// AhCoreNode — ROS orchestration only.
// - /ah/system/status       : JSON via messages.hpp
// - /ah/video/compressed    : frames via video_pipeline.hpp
// - /ah/tracking/*          : TrackingController
// - /ah/camera/list|select  : device enum + select
// - /ah/ai_tracking/*       : TrackingController
// - /ah/detections          : TrackingController (parse via messages.hpp)

#include "ah_core/ah_core_node.hpp"

#include "ah_core/camera_devices.hpp"
#include "ah_core/messages.hpp"
#include "ah_core/ros_runtime_settings.hpp"
#include "ah_core/video_pipeline.hpp"

#include <chrono>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace ah_core
{

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
    [this](const std::shared_ptr<ah_msgs::srv::StartTracking::Request>& request,
                                  const std::shared_ptr<ah_msgs::srv::StartTracking::Response>& response) {
      OnStartTracking(request, response);
    });

  stop_tracking_srv_ = create_service<std_srvs::srv::Trigger>(
    "ah/tracking/stop",
    [this](const std::shared_ptr<std_srvs::srv::Trigger::Request>& request,
             const std::shared_ptr<std_srvs::srv::Trigger::Response>& response) {
      OnStopTracking(request, response);
    });

  cancel_tracking_srv_ = create_service<std_srvs::srv::Trigger>(
    "ah/tracking/cancel",
    [this](const std::shared_ptr<std_srvs::srv::Trigger::Request>& request,
                                   const std::shared_ptr<std_srvs::srv::Trigger::Response>& response) {
      OnCancelTracking(request, response);
    });

  camera_cb_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

  list_cameras_srv_ = create_service<ah_msgs::srv::ListCameras>(
    "ah/camera/list",
    [this](const std::shared_ptr<ah_msgs::srv::ListCameras::Request>& request,
             const std::shared_ptr<ah_msgs::srv::ListCameras::Response>& response) {
      OnListCameras(request, response);
    },
    rclcpp::ServicesQoS(),
    camera_cb_group_);

  select_camera_srv_ = create_service<ah_msgs::srv::SelectCamera>(
    "ah/camera/select",
    [this](const std::shared_ptr<ah_msgs::srv::SelectCamera::Request> &request,
           const std::shared_ptr<ah_msgs::srv::SelectCamera::Response> &response) {
      OnSelectCamera(request, response);
    },
    rclcpp::ServicesQoS(),
    camera_cb_group_);

  ai_tracking_toggle_srv_ = create_service<std_srvs::srv::SetBool>(
    "ah/ai_tracking/toggle",
    [this](const std::shared_ptr<std_srvs::srv::SetBool::Request> &request,
           const std::shared_ptr<std_srvs::srv::SetBool::Response> &response) {
      OnAiTrackingToggle(request, response);
    });

  ai_tracking_click_srv_ = create_service<ah_msgs::srv::AiTrackingClick>(
    "ah/ai_tracking/click",
    [this](const std::shared_ptr<ah_msgs::srv::AiTrackingClick::Request> &request,
           const std::shared_ptr<ah_msgs::srv::AiTrackingClick::Response> &response) {
      OnAiTrackingClick(request, response);
    });

  rclcpp::QoS det_qos(rclcpp::KeepLast(1));
  det_qos.best_effort();
  detections_sub_ = create_subscription<std_msgs::msg::String>(
    "ah/detections", det_qos,
    [this](const std_msgs::msg::String::SharedPtr msg) {
      OnDetections(msg);
    });

  const auto period = std::chrono::milliseconds(1000 / kVideoPublishHz);
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
        const int held = camera_capture_.IsOpen() ? camera_capture_.DeviceId() : -999;
        camera_cache_ = EnumerateCameraDevices(camera_.backend, held);
        n = camera_cache_.size();
        camera_probe_done_ = true;

        if (camera_.video_source == "camera" && camera_.device_id >= 0) {
          if (!camera_capture_.IsOpen() || camera_capture_.DeviceId() != camera_.device_id) {
            camera_capture_.Close();
            if (camera_capture_.Open(camera_, &open_err)) {
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

  const char * ros_domain_id = std::getenv("ROS_DOMAIN_ID");
  RCLCPP_INFO(
    get_logger(),
    "ah_core ready: fqn=%s ROS_DOMAIN_ID=%s status+video @ %d Hz; "
    "services tracking + camera + ai_tracking/toggle|click; "
    "video.source=%s camera.id=%d path=%s (hardware probe + capture deferred)",
    get_fully_qualified_name(),
    ros_domain_id && ros_domain_id[0] ? ros_domain_id : "(default 0)",
    kVideoPublishHz,
    camera_.video_source.c_str(),
    camera_.device_id,
    camera_.device_path.c_str());
  RCLCPP_INFO(
    get_logger(),
    "CLI clients: source ./init_ah_ros_in_terminal.sh "
    "(exports ROS_DOMAIN_ID from aerohub_settings.ini via AhCommon)");
}

AhCoreNode::~AhCoreNode()
{
  std::lock_guard<std::mutex> lock(camera_mutex_);
  camera_capture_.Close();
}

void AhCoreNode::ApplySelectionToParams()
{
  set_parameter(rclcpp::Parameter("video.source", camera_.video_source));
  set_parameter(rclcpp::Parameter("camera.device_id", camera_.device_id));
  set_parameter(rclcpp::Parameter("camera.device_path", camera_.device_path));
  set_parameter(rclcpp::Parameter("camera.backend", camera_.backend));
}

void AhCoreNode::PersistSelectionToSettingsFile() const {
  std::string err;
  if (!PersistCameraSelectionToSettingsFile(settings_path_, camera_, err) &&
    !settings_path_.empty())
  {
    RCLCPP_WARN(get_logger(), "%s", err.c_str());
  }
}

bool AhCoreNode::SyncCaptureToSelection(std::string * error_out)
{
  std::lock_guard<std::mutex> lock(camera_mutex_);

  if (camera_.video_source != "camera" || camera_.device_id < 0) {
    camera_capture_.Close();
    last_video_status_ = "connected";
    capture_open_fail_log_count_ = 0;
    return true;
  }

  if (camera_capture_.IsOpen() && camera_capture_.DeviceId() == camera_.device_id) {
    return true;
  }

  camera_capture_.Close();
  if (!camera_capture_.Open(camera_, error_out)) {
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

  VideoGrabResult grab;
  {
    std::lock_guard<std::mutex> lock(camera_mutex_);
    grab = GrabPublishableFrame(
      camera_capture_, camera_, camera_probe_done_, tracking_.WasTrackingStarted(), frame_id_);

    if (!grab.capture_open_error.empty()) {
      last_video_status_ = grab.video_status;
      if (capture_open_fail_log_count_ < 3) {
        RCLCPP_WARN(
          get_logger(), "capture open failed: %s", grab.capture_open_error.c_str());
        ++capture_open_fail_log_count_;
      }
    } else {
      last_video_status_ = grab.video_status;
    }

    if (grab.live_grab_failed) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 3000,
        "camera frame grab failed (status=%s id=%d) — publishing synthetic fallback",
        grab.video_status.c_str(), camera_.device_id);
    }
  }

  std_msgs::msg::String status_msg;
  status_msg.data = CreateStatusJson(
    now.seconds(),
    grab.video_status.c_str(),
    tracking_.WasTrackingStarted(),
    tracking_.IsSegmentationActive(),
    tracking_.IsAiTrackingActive(),
    tracking_.IsFollowingActive(),
    tracking_.TrackerType().c_str(),
    camera_,
    tracking_.BboxX(),
    tracking_.BboxY(),
    tracking_.BboxW(),
    tracking_.BboxH(),
    tracking_.LockedTrackId());
  status_pub_->publish(status_msg);

  std::vector<uint8_t> jpeg;
  if (!EncodeJpeg(grab.bgr, &jpeg, kVideoJpegQuality)) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "JPEG encode failed");
    return;
  }

  sensor_msgs::msg::CompressedImage video_msg;
  video_msg.header.stamp = now;
  video_msg.header.frame_id = grab.frame_id;
  video_msg.format = "jpeg";
  video_msg.data = std::move(jpeg);
  video_pub_->publish(video_msg);
}

void AhCoreNode::OnStartTracking(
  const std::shared_ptr<ah_msgs::srv::StartTracking::Request> request,
  std::shared_ptr<ah_msgs::srv::StartTracking::Response> response)
{
  const auto result = tracking_.StartClassic(
    request->x, request->y, request->width, request->height);
  response->success = result.success;
  response->message = result.message;
  if (!result.success) {
    RCLCPP_WARN(
      get_logger(),
      "start rejected: x=%.3f y=%.3f w=%.3f h=%.3f — %s",
      request->x, request->y, request->width, request->height,
      result.message.c_str());
    return;
  }
  RCLCPP_INFO(
    get_logger(),
    "tracking started (classic) bbox norm x=%.3f y=%.3f w=%.3f h=%.3f",
    tracking_.BboxX(), tracking_.BboxY(), tracking_.BboxW(), tracking_.BboxH());
  if (result.publish_status) {
    PublishStatusSnapshot(last_video_status_.c_str());
  }
}

void AhCoreNode::OnStopTracking(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const auto result = tracking_.Stop();
  response->success = result.success;
  response->message = result.message;
  RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
}

void AhCoreNode::OnCancelTracking(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  const auto result = tracking_.Cancel();
  response->success = result.success;
  response->message = result.message;
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
    const int held = camera_capture_.IsOpen() ? camera_capture_.DeviceId() : -999;
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
      camera_capture_.Close();
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
    camera_capture_.Close();
    if (!camera_capture_.Open(next, &open_err)) {
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
    tracking_.WasTrackingStarted(),
    tracking_.IsSegmentationActive(),
    tracking_.IsAiTrackingActive(),
    tracking_.IsFollowingActive(),
    tracking_.TrackerType().c_str(),
    camera_,
    tracking_.BboxX(),
    tracking_.BboxY(),
    tracking_.BboxW(),
    tracking_.BboxH(),
    tracking_.LockedTrackId());
  status_pub_->publish(status_msg);
}

void AhCoreNode::OnDetections(const std_msgs::msg::String::SharedPtr msg)
{
  if (!msg) {
    return;
  }
  const auto ingest = tracking_.IngestDetections(msg->data);
  if (ingest.lock_lost) {
    RCLCPP_WARN(
      get_logger(),
      "lost track_id=%d for %d frames — clearing AI lock",
      ingest.lost_track_id, ingest.missed_frames);
    PublishStatusSnapshot(last_video_status_.c_str());
  }
}

void AhCoreNode::OnAiTrackingToggle(
  const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
  std::shared_ptr<std_srvs::srv::SetBool::Response> response)
{
  const auto result = tracking_.SetAiTrackingActive(request->data);
  response->success = result.success;
  response->message = result.message;
  RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
  if (result.publish_status) {
    PublishStatusSnapshot(last_video_status_.c_str());
  }
}

void AhCoreNode::OnAiTrackingClick(
  const std::shared_ptr<ah_msgs::srv::AiTrackingClick::Request> request,
  std::shared_ptr<ah_msgs::srv::AiTrackingClick::Response> response)
{
  const int previous_lock = tracking_.LockedTrackId();
  const auto result = tracking_.AiClick(request->x, request->y);

  response->success = result.success;
  response->message = result.message;
  response->lock_x = result.lock_x;
  response->lock_y = result.lock_y;
  response->lock_width = result.lock_width;
  response->lock_height = result.lock_height;

  if (!result.success) {
    if (result.message.find("no track_id") != std::string::npos) {
      RCLCPP_WARN(
        get_logger(),
        "ai tracking click hit box without track_id at (%.3f,%.3f) — lock unchanged (was %d)",
        request->x, request->y, previous_lock);
    } else if (result.message.rfind("Missed click", 0) == 0) {
      RCLCPP_INFO(
        get_logger(), "ai tracking click MISS at (%.3f,%.3f): %s",
        request->x, request->y, result.message.c_str());
    }
    if (result.publish_status) {
      PublishStatusSnapshot(last_video_status_.c_str());
    }
    return;
  }

  RCLCPP_INFO(
    get_logger(),
    "ai tracking NEW lock track_id=%d (was %d) click=(%.3f,%.3f) bbox=[%.3f,%.3f,%.3f,%.3f]",
    tracking_.LockedTrackId(), previous_lock, request->x, request->y,
    result.lock_x, result.lock_y, result.lock_width, result.lock_height);

  // Push status immediately so the dashboard green box / lockedTrackId switch
  // without waiting for the next video timer tick.
  if (result.publish_status) {
    PublishStatusSnapshot(last_video_status_.c_str());
  }
}

}  // namespace ah_core
