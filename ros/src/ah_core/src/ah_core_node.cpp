// AhCoreNode implementation
// - /ah/system/status       : JSON in std_msgs/String  (ros2-interface-map §3.1)
// - /ah/video/compressed    : live or synthetic JPEG   (ros2-interface-map §5)
// - /ah/tracking/start|stop|cancel : track services    (ros2-interface-map §4)
// - /ah/camera/list|select  : device enum + select     (Task_30 / Task_31)

#include "ah_core/ah_core_node.hpp"

#include "ah_core/camera_devices.hpp"
#include "ah_core/ros_runtime_settings.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

using namespace std::chrono_literals;

namespace ah_core
{
namespace
{
constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;
constexpr int kJpegQuality = 80;
constexpr int kPublishHz = 10;

std::string JsonEscape(const std::string & s)
{
  std::string out;
  out.reserve(s.size());
  for (const char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

std::string CreateStatusJson(
  double stamp_sec,
  const char * video_status,
  bool tracking_started,
  bool segmentation_active,
  bool ai_tracking_active,
  bool following_active,
  const char * tracker_type,
  const CameraSelection & cam,
  float track_x, float track_y, float track_w, float track_h,
  int locked_track_id)
{
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss.precision(3);
  oss << '{'
      << "\"ai_tracking_active\":" << (ai_tracking_active ? "true" : "false") << ','
      << "\"tracking_started\":" << (tracking_started ? "true" : "false") << ','
      << "\"segmentation_active\":" << (segmentation_active ? "true" : "false") << ','
      << "\"following_active\":" << (following_active ? "true" : "false") << ','
      << "\"video_status\":\"" << video_status << "\","
      << "\"tracker_type\":\"" << JsonEscape(tracker_type ? tracker_type : "stub") << "\","
      << "\"follower_mode\":\"none\","
      << "\"video_source\":\"" << JsonEscape(cam.video_source) << "\","
      << "\"camera_device_id\":" << cam.device_id << ','
      << "\"camera_device_path\":\"" << JsonEscape(cam.device_path) << "\","
      << "\"camera_backend\":\"" << JsonEscape(cam.backend) << "\","
      << "\"locked_track_id\":" << locked_track_id << ','
      << "\"tracking_bbox_x\":" << track_x << ','
      << "\"tracking_bbox_y\":" << track_y << ','
      << "\"tracking_bbox_w\":" << track_w << ','
      << "\"tracking_bbox_h\":" << track_h << ','
      << "\"stamp\":" << stamp_sec
      << '}';
  return oss.str();
}

// Minimal parse of ah_yolo JSON detections (no JSON library in ah_core).
float ParseJsonFloatAt(const std::string & s, size_t key_pos, size_t key_len, bool * ok)
{
  *ok = false;
  size_t i = key_pos + key_len;
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == ':')) {
    ++i;
  }
  // skip optional colon already handled; also skip leftover ':'
  if (i < s.size() && s[i] == ':') {
    ++i;
  }
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
    ++i;
  }
  try {
    size_t consumed = 0;
    const float v = std::stof(s.substr(i), &consumed);
    *ok = consumed > 0;
    return v;
  } catch (...) {
    return 0.f;
  }
}

/// First occurrence of "key": after @p from (must be before @p until if until != npos).
float ParseJsonFloatAfterKey(
  const std::string & s, size_t from, size_t until, const char * key, bool * ok)
{
  *ok = false;
  const std::string needle = std::string("\"") + key + "\"";
  const size_t k = s.find(needle, from);
  if (k == std::string::npos || (until != std::string::npos && k >= until)) {
    return 0.f;
  }
  return ParseJsonFloatAt(s, k, needle.size(), ok);
}

/// Last occurrence of "key": in [from, until) — used for track_id before bbox_normalized.
float ParseJsonFloatLastKeyBefore(
  const std::string & s, size_t from, size_t until, const char * key, bool * ok)
{
  *ok = false;
  const std::string needle = std::string("\"") + key + "\"";
  size_t last = std::string::npos;
  for (size_t p = from; p < until; ) {
    const size_t found = s.find(needle, p);
    if (found == std::string::npos || found >= until) {
      break;
    }
    last = found;
    p = found + 1;
  }
  if (last == std::string::npos) {
    return 0.f;
  }
  return ParseJsonFloatAt(s, last, needle.size(), ok);
}

cv::Mat CreateSyntheticImage(const int frame_id, bool tracking_started)
{
  cv::Mat frame(kFrameHeight, kFrameWidth, CV_8UC3, cv::Scalar(20, 24, 32));

  const int band_h = 40;
  for (int i = 0; i < 6; ++i) {
    const cv::Scalar colors[] = {
      {0, 0, 200}, {0, 200, 200}, {0, 200, 0},
      {200, 200, 0}, {200, 0, 0}, {200, 0, 200},
    };
    cv::rectangle(
      frame,
      cv::Rect(0, i * band_h, kFrameWidth, band_h),
      colors[i],
      cv::FILLED);
  }

  const int box = 48;
  const int span_x = kFrameWidth - box;
  const int span_y = kFrameHeight - box - 6 * band_h;
  const int x = (frame_id * 3) % (span_x > 0 ? span_x : 1);
  const int y = 6 * band_h + ((frame_id * 2) % (span_y > 0 ? span_y : 1));
  cv::rectangle(frame, cv::Rect(x, y, box, box), cv::Scalar(255, 255, 255), cv::FILLED);
  cv::rectangle(frame, cv::Rect(x, y, box, box), cv::Scalar(0, 140, 255), 2);

  cv::putText(
    frame,
    tracking_started ? "AeroHub ah_core TRACKING" : "AeroHub ah_core synthetic",
    cv::Point(12, kFrameHeight - 36),
    cv::FONT_HERSHEY_SIMPLEX,
    0.7,
    tracking_started ? cv::Scalar(80, 255, 120) : cv::Scalar(230, 230, 230),
    2);
  cv::putText(
    frame,
    "frame " + std::to_string(frame_id),
    cv::Point(12, kFrameHeight - 12),
    cv::FONT_HERSHEY_SIMPLEX,
    0.6,
    cv::Scalar(180, 200, 220),
    1);

  return frame;
}

void OverlayTrackingHint(cv::Mat & bgr, bool tracking_started)
{
  if (bgr.empty()) {
    return;
  }
  cv::putText(
    bgr,
    tracking_started ? "TRACKING" : "LIVE",
    cv::Point(12, 28),
    cv::FONT_HERSHEY_SIMPLEX,
    0.7,
    tracking_started ? cv::Scalar(80, 255, 120) : cv::Scalar(230, 230, 230),
    2);
}

bool IsTrackingBoundingBoxNormalized(float x, float y, float width, float height)
{
  return x >= 0.0f && y >= 0.0f && width > 0.0f && height > 0.0f &&
         x <= 1.0f && y <= 1.0f && width <= 1.0f && height <= 1.0f &&
         (x + width) <= 1.0001f && (y + height) <= 1.0001f;
}
}  // namespace

AhCoreNode::AhCoreNode(
  const std::string & ros_namespace,
  CameraSelection initial_camera,
  std::string settings_path)
: Node("ah_core", ros_namespace),
  camera_(std::move(initial_camera)),
  settings_path_(std::move(settings_path))
{
  declare_parameter<std::string>("video.source", camera_.video_source);
  declare_parameter<int>("camera.device_id", camera_.device_id);
  declare_parameter<std::string>("camera.device_path", camera_.device_path);
  declare_parameter<std::string>("camera.backend", camera_.backend);

  set_parameter(rclcpp::Parameter("video.source", camera_.video_source));
  set_parameter(rclcpp::Parameter("camera.device_id", camera_.device_id));
  set_parameter(rclcpp::Parameter("camera.device_path", camera_.device_path));
  set_parameter(rclcpp::Parameter("camera.backend", camera_.backend));

  rclcpp::QoS status_qos(rclcpp::KeepLast(1));
  status_qos.reliable();

  rclcpp::QoS video_qos(rclcpp::KeepLast(1));
  video_qos.best_effort();

  status_pub_ = create_publisher<std_msgs::msg::String>("ah/system/status", status_qos);
  video_pub_ =
    create_publisher<sensor_msgs::msg::CompressedImage>("ah/video/compressed", video_qos);

  start_tracking_srv_ = create_service<ah_msgs::srv::StartTracking>(
    "ah/tracking/start",
    [this](const std::shared_ptr<ah_msgs::srv::StartTracking::Request> request,
           std::shared_ptr<ah_msgs::srv::StartTracking::Response> response) {
      OnStartTracking(request, response);
    });

  stop_tracking_srv_ = create_service<std_srvs::srv::Trigger>(
    "ah/tracking/stop",
    [this](const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
           std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
      OnStopTracking(request, response);
    });

  cancel_tracking_srv_ = create_service<std_srvs::srv::Trigger>(
    "ah/tracking/cancel",
    [this](const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
           std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
      OnCancelTracking(request, response);
    });

  camera_cb_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

  list_cameras_srv_ = create_service<ah_msgs::srv::ListCameras>(
    "ah/camera/list",
    [this](const std::shared_ptr<ah_msgs::srv::ListCameras::Request> request,
           std::shared_ptr<ah_msgs::srv::ListCameras::Response> response) {
      OnListCameras(request, response);
    },
    rclcpp::ServicesQoS(),
    camera_cb_group_);

  select_camera_srv_ = create_service<ah_msgs::srv::SelectCamera>(
    "ah/camera/select",
    [this](const std::shared_ptr<ah_msgs::srv::SelectCamera::Request> request,
           std::shared_ptr<ah_msgs::srv::SelectCamera::Response> response) {
      OnSelectCamera(request, response);
    },
    rclcpp::ServicesQoS(),
    camera_cb_group_);

  ai_tracking_toggle_srv_ = create_service<std_srvs::srv::SetBool>(
    "ah/ai_tracking/toggle",
    [this](const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
           std::shared_ptr<std_srvs::srv::SetBool::Response> response) {
      OnAiTrackingToggle(request, response);
    });

  ai_tracking_click_srv_ = create_service<ah_msgs::srv::AiTrackingClick>(
    "ah/ai_tracking/click",
    [this](const std::shared_ptr<ah_msgs::srv::AiTrackingClick::Request> request,
           std::shared_ptr<ah_msgs::srv::AiTrackingClick::Response> response) {
      OnAiTrackingClick(request, response);
    });

  rclcpp::QoS det_qos(rclcpp::KeepLast(1));
  det_qos.best_effort();
  detections_sub_ = create_subscription<std_msgs::msg::String>(
    "ah/detections", det_qos,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      OnDetections(msg);
    });

  const auto period = std::chrono::milliseconds(1000 / kPublishHz);
  timer_ = create_wall_timer(period, [this]() { OnTimer(); });

  // Do not probe cameras in the constructor — spin must start first.
  camera_cache_.clear();
  camera_cache_.push_back(MakeSyntheticDevice());

  probe_timer_ = create_wall_timer(
    200ms,
    [this]() {
      if (probe_timer_) {
        probe_timer_->cancel();
      }
      RCLCPP_INFO(get_logger(), "camera probe starting (async)…");
      // Hold camera_mutex_ for enumerate + open so OnTimer cannot race exclusive
      // device access mid-probe (macOS AVFoundation).
      size_t n = 0;
      std::string open_err;
      bool opened_live = false;
      {
        std::lock_guard<std::mutex> lock(camera_mutex_);
        const int held = capture_.IsOpen() ? capture_.DeviceId() : -999;
        camera_cache_ = EnumerateCameraDevices(camera_.backend, held);
        n = camera_cache_.size();
        camera_probe_done_ = true;

        if (camera_.video_source == "camera" && camera_.device_id >= 0) {
          if (!capture_.IsOpen() || capture_.DeviceId() != camera_.device_id) {
            capture_.Close();
            if (capture_.Open(camera_, &open_err)) {
              opened_live = true;
              last_video_status_ = "connected";
              capture_open_fail_log_count_ = 0;
            } else {
              last_video_status_ = "unavailable";
            }
          } else {
            opened_live = true;
          }
        }
      }
      RCLCPP_INFO(
        get_logger(),
        "camera probe done: %zu device(s) (incl. synthetic)",
        n);
      if (!open_err.empty() && !opened_live) {
        RCLCPP_WARN(get_logger(), "initial capture open failed: %s", open_err.c_str());
      } else if (opened_live) {
        RCLCPP_INFO(
          get_logger(),
          "live capture open: id=%d path=%s",
          camera_.device_id,
          camera_.device_path.c_str());
      }
    },
    camera_cb_group_);

  const char * domain = std::getenv("ROS_DOMAIN_ID");
  RCLCPP_INFO(
    get_logger(),
    "ah_core ready: fqn=%s ROS_DOMAIN_ID=%s status+video @ %d Hz; "
    "services tracking + camera + ai_tracking/toggle|click; "
    "video.source=%s camera.id=%d path=%s (hardware probe + capture deferred)",
    get_fully_qualified_name(),
    domain && domain[0] ? domain : "(default 0)",
    kPublishHz,
    camera_.video_source.c_str(),
    camera_.device_id,
    camera_.device_path.c_str());
  RCLCPP_INFO(
    get_logger(),
    "CLI clients must use the same ROS_DOMAIN_ID (export it or: source ./init_ah_ros_in_terminal.sh)");
}

AhCoreNode::~AhCoreNode()
{
  std::lock_guard<std::mutex> lock(camera_mutex_);
  capture_.Close();
}

void AhCoreNode::ApplySelectionToParams()
{
  set_parameter(rclcpp::Parameter("video.source", camera_.video_source));
  set_parameter(rclcpp::Parameter("camera.device_id", camera_.device_id));
  set_parameter(rclcpp::Parameter("camera.device_path", camera_.device_path));
  set_parameter(rclcpp::Parameter("camera.backend", camera_.backend));
}

void AhCoreNode::PersistSelectionToSettingsFile()
{
  std::string err;
  if (!PersistCameraSelectionToSettingsFile(settings_path_, camera_, &err) &&
    !settings_path_.empty())
  {
    RCLCPP_WARN(get_logger(), "%s", err.c_str());
  }
}

bool AhCoreNode::SyncCaptureToSelection(std::string * error_out)
{
  std::lock_guard<std::mutex> lock(camera_mutex_);

  if (camera_.video_source != "camera" || camera_.device_id < 0) {
    capture_.Close();
    last_video_status_ = "connected";
    capture_open_fail_log_count_ = 0;
    return true;
  }

  if (capture_.IsOpen() && capture_.DeviceId() == camera_.device_id) {
    return true;
  }

  capture_.Close();
  if (!capture_.Open(camera_, error_out)) {
    last_video_status_ = "unavailable";
    return false;
  }
  last_video_status_ = "connected";
  capture_open_fail_log_count_ = 0;
  return true;
}

void AhCoreNode::OnTimer()
{
  const auto now = get_clock()->now();
  ++frame_id_;

  cv::Mat bgr;
  std::string frame_id = "ah_camera_stub";
  const char * video_status = "connected";

  {
    std::lock_guard<std::mutex> lock(camera_mutex_);

    if (camera_.video_source == "camera" && camera_.device_id >= 0) {
      // Wait for async probe (or select) before fighting AVFoundation opens.
      if (!camera_probe_done_ && !capture_.IsOpen()) {
        bgr = CreateSyntheticImage(frame_id_, tracking_started_);
        video_status = "connected";
      } else {
        if (!capture_.IsOpen() || capture_.DeviceId() != camera_.device_id) {
          std::string err;
          if (!capture_.Open(camera_, &err)) {
            video_status = "unavailable";
            last_video_status_ = video_status;
            if (capture_open_fail_log_count_ < 3) {
              RCLCPP_WARN(get_logger(), "capture open failed: %s", err.c_str());
              ++capture_open_fail_log_count_;
            }
            bgr = CreateSyntheticImage(frame_id_, tracking_started_);
          }
        }
        if (capture_.IsOpen() && capture_.ReadBgr(&bgr) && !bgr.empty()) {
          OverlayTrackingHint(bgr, tracking_started_);
          frame_id = "ah_camera";
          video_status = "connected";
          last_video_status_ = video_status;
        } else if (bgr.empty()) {
          video_status = capture_.IsOpen() ? "degraded" : "unavailable";
          last_video_status_ = video_status;
          bgr = CreateSyntheticImage(frame_id_, tracking_started_);
          RCLCPP_WARN_THROTTLE(
            get_logger(), *get_clock(), 3000,
            "camera frame grab failed (status=%s id=%d) — publishing synthetic fallback",
            video_status, camera_.device_id);
        }
      }
    } else {
      if (capture_.IsOpen()) {
        capture_.Close();
      }
      bgr = CreateSyntheticImage(frame_id_, tracking_started_);
      video_status = "connected";
      last_video_status_ = video_status;
    }
  }

  std_msgs::msg::String status_msg;
  status_msg.data = CreateStatusJson(
    now.seconds(),
    video_status,
    tracking_started_,
    segmentation_active_,
    ai_tracking_active_,
    following_active_,
    tracker_type_.c_str(),
    camera_,
    tracking_bounding_box_x_,
    tracking_bounding_box_y_,
    tracking_bounding_box_width_,
    tracking_bounding_box_height_,
    locked_track_id_);
  status_pub_->publish(status_msg);

  std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, kJpegQuality};
  std::vector<uint8_t> jpeg;
  if (!cv::imencode(".jpg", bgr, jpeg, params)) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "JPEG encode failed");
    return;
  }

  sensor_msgs::msg::CompressedImage video_msg;
  video_msg.header.stamp = now;
  video_msg.header.frame_id = frame_id;
  video_msg.format = "jpeg";
  video_msg.data = std::move(jpeg);
  video_pub_->publish(video_msg);
}

void AhCoreNode::OnStartTracking(
  const std::shared_ptr<ah_msgs::srv::StartTracking::Request> request,
  std::shared_ptr<ah_msgs::srv::StartTracking::Response> response)
{
  if (!IsTrackingBoundingBoxNormalized(request->x, request->y, request->width, request->height)) {
    response->success = false;
    response->message =
      "tracking bounding box must be normalized in [0,1] with positive width/height (interface map §4)";
    RCLCPP_WARN(
      get_logger(),
      "start rejected: x=%.3f y=%.3f w=%.3f h=%.3f",
      request->x, request->y, request->width, request->height);
    return;
  }

  tracking_started_ = true;
  tracker_type_ = "classic";
  // Classic bbox must not keep following a previous AI track_id.
  locked_track_id_ = -1;
  locked_track_miss_frames_ = 0;
  tracking_bounding_box_x_ = request->x;
  tracking_bounding_box_y_ = request->y;
  tracking_bounding_box_width_ = request->width;
  tracking_bounding_box_height_ = request->height;

  response->success = true;
  response->message = "tracking started (classic bbox)";
  RCLCPP_INFO(
    get_logger(),
    "tracking started (classic) bbox norm x=%.3f y=%.3f w=%.3f h=%.3f",
    tracking_bounding_box_x_, tracking_bounding_box_y_, tracking_bounding_box_width_,
    tracking_bounding_box_height_);
  PublishStatusSnapshot(last_video_status_.c_str());
}

void AhCoreNode::OnStopTracking(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const bool was = tracking_started_;
  tracking_started_ = false;
  locked_track_id_ = -1;
  locked_track_miss_frames_ = 0;
  if (!ai_tracking_active_) {
    tracker_type_ = "stub";
  }
  response->success = true;
  response->message = was ? "tracking stopped" : "tracking already idle";
  RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
}

void AhCoreNode::OnCancelTracking(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  tracking_started_ = false;
  segmentation_active_ = false;
  locked_track_id_ = -1;
  locked_track_miss_frames_ = 0;
  if (!ai_tracking_active_) {
    tracker_type_ = "stub";
  } else {
    tracker_type_ = "ai_tracking";
  }
  response->success = true;
  response->message = "tracking cancelled (hard reset)";
  RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
}

void AhCoreNode::OnListCameras(
  const std::shared_ptr<ah_msgs::srv::ListCameras::Request> request,
  std::shared_ptr<ah_msgs::srv::ListCameras::Response> response)
{
  RCLCPP_INFO(
    get_logger(),
    "ah/camera/list called (refresh=%s)",
    request->refresh ? "true" : "false");

  std::vector<CameraDevice> snapshot;
  {
    std::lock_guard<std::mutex> lock(camera_mutex_);
    const int held = capture_.IsOpen() ? capture_.DeviceId() : -999;
    if (request->refresh || !camera_probe_done_) {
      camera_cache_ = EnumerateCameraDevices(camera_.backend, held);
      camera_probe_done_ = true;
    }
    snapshot = camera_cache_;
  }

  response->success = true;
  response->ids.clear();
  response->paths.clear();
  response->names.clear();
  response->backends.clear();
  std::ostringstream msg;
  msg << "ok count=" << snapshot.size();
  for (const auto & d : snapshot) {
    response->ids.push_back(static_cast<int32_t>(d.id));
    response->paths.push_back(d.path);
    response->names.push_back(d.name);
    response->backends.push_back(d.backend);
    msg << " | " << d.id << ":" << d.path;
  }
  response->message = msg.str();
  response->video_source = camera_.video_source;
  response->selected_id = camera_.device_id;
  response->selected_path = camera_.device_path;
  response->selected_backend = camera_.backend;

  RCLCPP_INFO(
    get_logger(),
    "camera list response: %zu device(s) ids_size=%zu msg=%s",
    snapshot.size(),
    response->ids.size(),
    response->message.c_str());
}

void AhCoreNode::OnSelectCamera(
  const std::shared_ptr<ah_msgs::srv::SelectCamera::Request> request,
  std::shared_ptr<ah_msgs::srv::SelectCamera::Response> response)
{
  std::string source = Trim(request->video_source, TrimIniChars);
  int device_id = request->device_id;
  std::string path = Trim(request->device_path, TrimIniChars);
  std::string backend = Trim(request->backend, TrimIniChars);

  if (source.empty()) {
    if ((!path.empty() && path != "synthetic") || device_id >= 0) {
      source = "camera";
    } else if (path == "synthetic" || device_id < 0) {
      source = "synthetic";
    } else {
      source = camera_.video_source;
    }
  }

  if (source != "synthetic" && source != "camera") {
    response->success = false;
    response->message = "video_source must be \"synthetic\" or \"camera\"";
    FillSelectResponse(response);
    return;
  }

  if (source == "synthetic") {
    {
      std::lock_guard<std::mutex> lock(camera_mutex_);
      camera_.video_source = "synthetic";
      camera_.device_id = -1;
      camera_.device_path = "synthetic";
      if (!backend.empty()) {
        camera_.backend = backend;
      }
      capture_.Close();
      last_video_status_ = "connected";
    }
    ApplySelectionToParams();
    PersistSelectionToSettingsFile();
    response->success = true;
    response->message = "selected synthetic test pattern";
    FillSelectResponse(response);
    RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
    return;
  }

  if (!path.empty()) {
    int parsed = -1;
    if (!ParseDevicePath(path, &parsed) && path.rfind("index:", 0) != 0 &&
      path.find("/dev/video") == std::string::npos)
    {
      if (device_id < 0) {
        response->success = false;
        response->message =
          "device_path not recognized; provide device_id or use index:N / synthetic";
        FillSelectResponse(response);
        return;
      }
    } else if (parsed >= 0) {
      device_id = parsed;
    }
  } else if (device_id >= 0) {
    path = PathForIndex(device_id);
  } else {
    response->success = false;
    response->message = "camera selection requires device_path or device_id >= 0";
    FillSelectResponse(response);
    return;
  }

  if (device_id < 0) {
    response->success = false;
    response->message = "could not resolve device_id for selection";
    FillSelectResponse(response);
    return;
  }

  // Build tentative selection, open long-lived capture (validates device).
  CameraSelection next = camera_;
  next.video_source = "camera";
  next.device_id = device_id;
  next.device_path = path.empty() ? PathForIndex(device_id) : path;
  next.backend = backend.empty() ? camera_.backend : backend;
  if (next.backend.empty()) {
    next.backend = DefaultBackendName();
  }

  std::string open_err;
  {
    std::lock_guard<std::mutex> lock(camera_mutex_);
    capture_.Close();
    if (!capture_.Open(next, &open_err)) {
      response->success = false;
      response->message = open_err;
      FillSelectResponse(response);
      RCLCPP_WARN(get_logger(), "select failed: %s", open_err.c_str());
      return;
    }
    camera_ = next;
    last_video_status_ = "connected";
    capture_open_fail_log_count_ = 0;
  }

  ApplySelectionToParams();
  PersistSelectionToSettingsFile();

  response->success = true;
  response->message = "selected camera id=" + std::to_string(camera_.device_id) +
                      " path=" + camera_.device_path + " (live capture open)";
  FillSelectResponse(response);
  RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
}

void AhCoreNode::FillSelectResponse(
  std::shared_ptr<ah_msgs::srv::SelectCamera::Response> response) const
{
  response->video_source = camera_.video_source;
  response->device_id = camera_.device_id;
  response->device_path = camera_.device_path;
  response->backend = camera_.backend;
}

void AhCoreNode::PublishStatusSnapshot(const char * video_status)
{
  const auto now = get_clock()->now();
  std_msgs::msg::String status_msg;
  status_msg.data = CreateStatusJson(
    now.seconds(),
    video_status ? video_status : last_video_status_.c_str(),
    tracking_started_,
    segmentation_active_,
    ai_tracking_active_,
    following_active_,
    tracker_type_.c_str(),
    camera_,
    tracking_bounding_box_x_,
    tracking_bounding_box_y_,
    tracking_bounding_box_width_,
    tracking_bounding_box_height_,
    locked_track_id_);
  status_pub_->publish(status_msg);
}

void AhCoreNode::OnDetections(const std_msgs::msg::String::SharedPtr msg)
{
  if (!msg) {
    return;
  }
  const std::string & json = msg->data;

  // Pair track_id[i] with bbox_normalized[i] by document order (ah_yolo always emits
  // track_id before bbox in each detection object). This avoids sticky wrong IDs from
  // windowed key search when multiple detections share one JSON payload.
  std::vector<int> track_ids;
  {
    const std::string tid_key = "\"track_id\"";
    size_t p = 0;
    while (true) {
      const size_t k = json.find(tid_key, p);
      if (k == std::string::npos) {
        break;
      }
      bool okt = false;
      const float tid_f = ParseJsonFloatAt(json, k, tid_key.size(), &okt);
      track_ids.push_back(okt ? static_cast<int>(tid_f) : -1);
      p = k + tid_key.size();
    }
  }

  std::vector<DetectionBox> parsed;
  size_t pos = 0;
  size_t det_index = 0;
  while (true) {
    const size_t bb = json.find("\"bbox_normalized\"", pos);
    if (bb == std::string::npos) {
      break;
    }
    // Scope x,y,w,h to this bbox object only (avoid leaking into later objects).
    const size_t brace = json.find('{', bb);
    size_t bb_end = std::string::npos;
    if (brace != std::string::npos) {
      bb_end = json.find('}', brace);
      if (bb_end != std::string::npos) {
        ++bb_end;
      }
    }
    bool okx = false, oky = false, okw = false, okh = false, okc = false;
    DetectionBox d;
    d.x = ParseJsonFloatAfterKey(json, bb, bb_end, "x", &okx);
    d.y = ParseJsonFloatAfterKey(json, bb, bb_end, "y", &oky);
    d.w = ParseJsonFloatAfterKey(json, bb, bb_end, "w", &okw);
    d.h = ParseJsonFloatAfterKey(json, bb, bb_end, "h", &okh);
    // confidence lives in the same detection object, just before bbox_normalized.
    const size_t conf_from = (det_index == 0) ? 0 : pos;
    d.confidence = ParseJsonFloatLastKeyBefore(json, conf_from, bb, "confidence", &okc);
    (void)okc;
    d.track_id = (det_index < track_ids.size()) ? track_ids[det_index] : -1;
    if (okx && oky && okw && okh && d.w > 0.f && d.h > 0.f) {
      parsed.push_back(d);
    }
    ++det_index;
    pos = bb + 17;  // length of "bbox_normalized" including quotes
  }
  {
    std::lock_guard<std::mutex> lock(detections_mutex_);
    last_detections_ = std::move(parsed);
  }
  // Follow locked track across frames when AI tracking lock is active.
  UpdateLockFromTrackedId();
}

void AhCoreNode::UpdateLockFromTrackedId()
{
  if (!tracking_started_ || locked_track_id_ < 0) {
    return;
  }

  const int want_id = locked_track_id_;

  std::vector<DetectionBox> dets;
  {
    std::lock_guard<std::mutex> lock(detections_mutex_);
    dets = last_detections_;
  }

  for (const auto & d : dets) {
    if (d.track_id == want_id) {
      tracking_bounding_box_x_ = d.x;
      tracking_bounding_box_y_ = d.y;
      tracking_bounding_box_width_ = d.w;
      tracking_bounding_box_height_ = d.h;
      locked_track_miss_frames_ = 0;
      return;
    }
  }

  // Track ID missing this frame — keep last box for a short grace period.
  ++locked_track_miss_frames_;
  constexpr int kMaxMiss = 15;  // ~detections rate; clear after sustained loss
  if (locked_track_miss_frames_ >= kMaxMiss) {
    RCLCPP_WARN(
      get_logger(),
      "lost track_id=%d for %d frames — clearing AI lock",
      want_id, locked_track_miss_frames_);
    tracking_started_ = false;
    locked_track_id_ = -1;
    locked_track_miss_frames_ = 0;
    tracking_bounding_box_x_ = 0.f;
    tracking_bounding_box_y_ = 0.f;
    tracking_bounding_box_width_ = 0.f;
    tracking_bounding_box_height_ = 0.f;
    PublishStatusSnapshot(last_video_status_.c_str());
  }
}

bool AhCoreNode::ResolveAiTrackingClickLock(
  float click_x, float click_y,
  float * out_x, float * out_y, float * out_w, float * out_h,
  int * out_track_id,
  std::string * out_label) const
{
  // AI tracking lock only on a detection that contains the click (no free-point box).
  // When several boxes contain the point (overlap / nested), pick the one whose
  // *center* is closest to the click — not merely the smallest area. That makes
  // switching from a large locked object onto a nearby smaller one reliable.
  std::vector<DetectionBox> dets;
  {
    std::lock_guard<std::mutex> lock(detections_mutex_);
    dets = last_detections_;
  }

  int best = -1;
  float best_dist2 = 1e30f;
  float best_area = 1e30f;

  for (size_t i = 0; i < dets.size(); ++i) {
    const auto & d = dets[i];
    const float x1 = d.x;
    const float y1 = d.y;
    const float x2 = d.x + d.w;
    const float y2 = d.y + d.h;
    if (click_x < x1 || click_x > x2 || click_y < y1 || click_y > y2) {
      continue;
    }
    const float cx = d.x + 0.5f * d.w;
    const float cy = d.y + 0.5f * d.h;
    const float dx = click_x - cx;
    const float dy = click_y - cy;
    const float dist2 = dx * dx + dy * dy;
    const float area = d.w * d.h;
    // Prefer closer center; break ties with smaller area.
    if (dist2 < best_dist2 - 1e-12f ||
        (std::fabs(dist2 - best_dist2) <= 1e-12f && area < best_area))
    {
      best_dist2 = dist2;
      best_area = area;
      best = static_cast<int>(i);
    }
  }

  if (best < 0) {
    if (out_label) {
      *out_label = dets.empty() ? "no detections" : "miss (click a detection)";
    }
    if (out_track_id) {
      *out_track_id = -1;
    }
    return false;
  }

  const auto & d = dets[static_cast<size_t>(best)];
  *out_x = d.x;
  *out_y = d.y;
  *out_w = d.w;
  *out_h = d.h;
  if (out_track_id) {
    *out_track_id = d.track_id;
  }
  if (out_label) {
    if (d.track_id >= 0) {
      *out_label = (d.class_name.empty() ? "obj" : d.class_name) +
                   " id=" + std::to_string(d.track_id);
    } else {
      *out_label = d.class_name.empty() ? "detection (no track_id)" : d.class_name;
    }
  }
  return true;
}

void AhCoreNode::OnAiTrackingToggle(
  const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
  std::shared_ptr<std_srvs::srv::SetBool::Response> response)
{
  const bool turning_on = request->data;
  ai_tracking_active_ = turning_on;

  if (turning_on) {
    // Enter ai tracking: clear classic framing; lock only appears after click-on-detection.
    tracking_started_ = false;
    locked_track_id_ = -1;
    locked_track_miss_frames_ = 0;
    tracking_bounding_box_x_ = 0.f;
    tracking_bounding_box_y_ = 0.f;
    tracking_bounding_box_width_ = 0.f;
    tracking_bounding_box_height_ = 0.f;
    tracker_type_ = "ai_tracking";
  } else {
    // Leave ai tracking: stop any ai tracking lock; classic drag box returns on the UI.
    tracking_started_ = false;
    locked_track_id_ = -1;
    locked_track_miss_frames_ = 0;
    tracker_type_ = "stub";
    tracking_bounding_box_x_ = 0.35f;
    tracking_bounding_box_y_ = 0.35f;
    tracking_bounding_box_width_ = 0.30f;
    tracking_bounding_box_height_ = 0.30f;
  }

  response->success = true;
  response->message = ai_tracking_active_ ? "ai tracking ON (click detections only)"
                                         : "ai tracking OFF (classic drag)";
  RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
  PublishStatusSnapshot(last_video_status_.c_str());
}

void AhCoreNode::OnAiTrackingClick(
  const std::shared_ptr<ah_msgs::srv::AiTrackingClick::Request> request,
  std::shared_ptr<ah_msgs::srv::AiTrackingClick::Response> response)
{
  if (!ai_tracking_active_) {
    response->success = false;
    response->message = "ai tracking is OFF — enable ah/ai_tracking/toggle first";
    return;
  }
  if (request->x < 0.f || request->x > 1.f || request->y < 0.f || request->y > 1.f) {
    response->success = false;
    response->message = "click must be normalized in [0,1]";
    return;
  }

  const int previous_lock = locked_track_id_;

  float bx = 0.f, by = 0.f, bw = 0.f, bh = 0.f;
  int track_id = -1;
  std::string label;
  if (!ResolveAiTrackingClickLock(
        request->x, request->y, &bx, &by, &bw, &bh, &track_id, &label))
  {
    // Miss: do not start tracking and do not invent a free-point box.
    tracking_started_ = false;
    locked_track_id_ = -1;
    locked_track_miss_frames_ = 0;
    tracking_bounding_box_x_ = 0.f;
    tracking_bounding_box_y_ = 0.f;
    tracking_bounding_box_width_ = 0.f;
    tracking_bounding_box_height_ = 0.f;
    response->success = false;
    response->message = "MISS: click must hit a detection (" + label + ")";
    response->lock_x = 0.f;
    response->lock_y = 0.f;
    response->lock_width = 0.f;
    response->lock_height = 0.f;
    RCLCPP_INFO(
      get_logger(), "ai tracking click MISS at (%.3f,%.3f): %s",
      request->x, request->y, response->message.c_str());
    PublishStatusSnapshot(last_video_status_.c_str());
    return;
  }

  if (track_id < 0) {
    response->success = false;
    response->message =
      "detection has no track_id — is ah_yolo using model.track(persist=True)?";
    response->lock_x = 0.f;
    response->lock_y = 0.f;
    response->lock_width = 0.f;
    response->lock_height = 0.f;
    RCLCPP_WARN(
      get_logger(),
      "ai tracking click hit box without track_id at (%.3f,%.3f) — lock unchanged (was %d)",
      request->x, request->y, previous_lock);
    return;
  }

  // Explicitly replace any previous lock (do not leave old track_id sticky).
  tracking_started_ = true;
  tracker_type_ = "ai_tracking";
  locked_track_id_ = track_id;
  locked_track_miss_frames_ = 0;
  tracking_bounding_box_x_ = bx;
  tracking_bounding_box_y_ = by;
  tracking_bounding_box_width_ = bw;
  tracking_bounding_box_height_ = bh;

  response->success = true;
  response->lock_x = bx;
  response->lock_y = by;
  response->lock_width = bw;
  response->lock_height = bh;
  response->message = "ai tracking lock on [" + label + "] track_id=" +
                      std::to_string(track_id);
  RCLCPP_INFO(
    get_logger(),
    "ai tracking NEW lock track_id=%d (was %d) click=(%.3f,%.3f) bbox=[%.3f,%.3f,%.3f,%.3f]",
    track_id, previous_lock, request->x, request->y, bx, by, bw, bh);

  // Push status immediately so the dashboard green box / lockedTrackId switch
  // without waiting for the next video timer tick (which looked like "old re-selected").
  PublishStatusSnapshot(last_video_status_.c_str());
}

}  // namespace ah_core
