// ah_core_node — AeroHub core
// - /ah/system/status       : JSON in std_msgs/String  (ros2-interface-map §3.1)
// - /ah/video/compressed    : synthetic JPEG frames    (ros2-interface-map §5)
// - /ah/tracking/start|stop|cancel : track services    (ros2-interface-map §4)
// - /ah/camera/list|select  : device enum + select     (Task_30; no hard-coded IDs)

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "ah_msgs/srv/list_cameras.hpp"
#include "ah_msgs/srv/select_camera.hpp"
#include "ah_msgs/srv/start_tracking.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"

#if defined(__linux__)
#include <dirent.h>
#include <sys/stat.h>
#endif

using namespace std::chrono_literals;

namespace
{
constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;
constexpr int kJpegQuality = 80;
constexpr int kPublishHz = 10;
// Keep the probe short: macOS often only has indices 0–1; long probes spam
// OpenCV ("out device of bound"). Stop after one consecutive open failure.
constexpr int kMaxCameraProbeIndex = 6;
constexpr int kMaxConsecutiveOpenFails = 1;

struct CameraDevice
{
  int id{-1};
  std::string path;
  std::string name;
  std::string backend;
};

struct CameraSelection
{
  std::string video_source{"synthetic"};  // synthetic | camera
  int device_id{-1};
  std::string device_path;
  std::string backend;
};

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
  bool smart_mode_active,
  bool following_active,
  const CameraSelection & cam)
{
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss.precision(3);
  oss << '{'
      << "\"smart_mode_active\":" << (smart_mode_active ? "true" : "false") << ','
      << "\"tracking_started\":" << (tracking_started ? "true" : "false") << ','
      << "\"segmentation_active\":" << (segmentation_active ? "true" : "false") << ','
      << "\"following_active\":" << (following_active ? "true" : "false") << ','
      << "\"video_status\":\"" << video_status << "\","
      << "\"tracker_type\":\"stub\","
      << "\"follower_mode\":\"none\","
      << "\"video_source\":\"" << JsonEscape(cam.video_source) << "\","
      << "\"camera_device_id\":" << cam.device_id << ','
      << "\"camera_device_path\":\"" << JsonEscape(cam.device_path) << "\","
      << "\"camera_backend\":\"" << JsonEscape(cam.backend) << "\","
      << "\"stamp\":" << stamp_sec
      << '}';
  return oss.str();
}

// Simple synthetic pattern: dark background, color bar strip, bouncing box, frame id.
cv::Mat CreateSyntheticImage(const int frame_id, bool tracking_started)
{
  cv::Mat frame(kFrameHeight, kFrameWidth, CV_8UC3, cv::Scalar(20, 24, 32));

  // Horizontal color bands
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

  // Bouncing box
  const int box = 48;
  const int span_x = kFrameWidth - box;
  const int span_y = kFrameHeight - box - 6 * band_h;
  const int x = (frame_id * 3) % (span_x > 0 ? span_x : 1);
  const int y = 6 * band_h + ((frame_id * 2) % (span_y > 0 ? span_y : 1));
  cv::rectangle(frame, cv::Rect(x, y, box, box), cv::Scalar(255, 255, 255), cv::FILLED);
  cv::rectangle(frame, cv::Rect(x, y, box, box), cv::Scalar(0, 140, 255), 2);

  // Overlay text
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

bool IsTrackingBoundingBoxNormalized(float x, float y, float width, float height)
{
  return x >= 0.0f && y >= 0.0f && width > 0.0f && height > 0.0f &&
         x <= 1.0f && y <= 1.0f && width <= 1.0f && height <= 1.0f &&
         (x + width) <= 1.0001f && (y + height) <= 1.0001f;
}

std::string DefaultBackendName()
{
#if defined(__APPLE__)
  return "avfoundation";
#elif defined(__linux__)
  return "v4l2";
#else
  return "any";
#endif
}

int BackendApiPreference(const std::string & backend)
{
  if (backend.empty() || backend == "any") {
    return cv::CAP_ANY;
  }
#if defined(__APPLE__)
  if (backend == "avfoundation") {
    return cv::CAP_AVFOUNDATION;
  }
#endif
#if defined(__linux__)
  if (backend == "v4l2") {
    return cv::CAP_V4L2;
  }
#endif
  return cv::CAP_ANY;
}

bool TryOpenIndex(int index, const std::string & backend_hint, std::string * opened_backend)
{
  // Prefer platform backend only (skip CAP_ANY fallback) to avoid double OpenCV
  // error spam and long hangs on missing indices.
  const int api = BackendApiPreference(backend_hint);
  cv::VideoCapture cap;
  if (!cap.open(index, api)) {
    return false;
  }
  (void)cap.grab();
  if (opened_backend != nullptr) {
    *opened_backend = backend_hint.empty() || backend_hint == "any"
      ? DefaultBackendName()
      : backend_hint;
  }
  cap.release();
  return true;
}

// Parse path forms: "synthetic", "index:N", "/dev/videoN"
bool ParseDevicePath(const std::string & path, int * out_id)
{
  if (path.empty() || path == "synthetic") {
    if (out_id) {
      *out_id = -1;
    }
    return path == "synthetic";
  }
  if (path.rfind("index:", 0) == 0) {
    try {
      const int id = std::stoi(path.substr(6));
      if (out_id) {
        *out_id = id;
      }
      return id >= 0;
    } catch (...) {
      return false;
    }
  }
#if defined(__linux__)
  if (path.rfind("/dev/video", 0) == 0) {
    try {
      const int id = std::stoi(path.substr(std::string("/dev/video").size()));
      if (out_id) {
        *out_id = id;
      }
      return id >= 0;
    } catch (...) {
      return false;
    }
  }
#endif
  return false;
}

std::string PathForIndex(int index)
{
#if defined(__linux__)
  const std::string dev = "/dev/video" + std::to_string(index);
  struct stat st {};
  if (stat(dev.c_str(), &st) == 0) {
    return dev;
  }
#endif
  return "index:" + std::to_string(index);
}

std::vector<CameraDevice> EnumerateCameraDevices(const std::string & backend_hint)
{
  std::vector<CameraDevice> devices;

  // Always offer synthetic so operators never depend on a magic camera index.
  devices.push_back(CameraDevice{-1, "synthetic", "Synthetic test pattern", "none"});

  std::vector<int> indices_to_try;
#if defined(__linux__)
  if (DIR * dir = opendir("/dev")) {
    while (dirent * ent = readdir(dir)) {
      const std::string name = ent->d_name;
      if (name.rfind("video", 0) != 0) {
        continue;
      }
      try {
        const int id = std::stoi(name.substr(5));
        indices_to_try.push_back(id);
      } catch (...) {
      }
    }
    closedir(dir);
    std::sort(indices_to_try.begin(), indices_to_try.end());
    indices_to_try.erase(
      std::unique(indices_to_try.begin(), indices_to_try.end()), indices_to_try.end());
  }
#endif

  if (indices_to_try.empty()) {
    for (int i = 0; i <= kMaxCameraProbeIndex; ++i) {
      indices_to_try.push_back(i);
    }
  }

  int consecutive_fails = 0;
  for (const int index : indices_to_try) {
    std::string opened_backend;
    if (!TryOpenIndex(index, backend_hint, &opened_backend)) {
      ++consecutive_fails;
      // Sequential index holes mean "no more devices" on most platforms.
      if (consecutive_fails >= kMaxConsecutiveOpenFails) {
        break;
      }
      continue;
    }
    consecutive_fails = 0;
    CameraDevice d;
    d.id = index;
    d.path = PathForIndex(index);
    d.name = "Camera " + std::to_string(index);
    d.backend = opened_backend.empty() ? DefaultBackendName() : opened_backend;
    devices.push_back(std::move(d));
  }

  return devices;
}

std::string Trim(std::string s)
{
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '"')) {
    s.erase(s.begin());
  }
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '"')) {
    s.pop_back();
  }
  return s;
}
}  // namespace

class AhCoreNode : public rclcpp::Node
{
public:
  // Relative names (no leading /) so node namespace prefixes them for multi-vehicle.
  // Empty namespace → /ah/... ; namespace "uav1" → /uav1/ah/...
  explicit AhCoreNode(
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

    // Sync ROS params with selection loaded from settings (if any).
    set_parameter(rclcpp::Parameter("video.source", camera_.video_source));
    set_parameter(rclcpp::Parameter("camera.device_id", camera_.device_id));
    set_parameter(rclcpp::Parameter("camera.device_path", camera_.device_path));
    set_parameter(rclcpp::Parameter("camera.backend", camera_.backend));

    rclcpp::QoS status_qos(rclcpp::KeepLast(1));
    status_qos.reliable();

    // Video: latest-frame only, best effort (interface map §5).
    rclcpp::QoS video_qos(rclcpp::KeepLast(1));
    video_qos.best_effort();

    status_pub_ = create_publisher<std_msgs::msg::String>("ah/system/status", status_qos);
    video_pub_ =
      create_publisher<sensor_msgs::msg::CompressedImage>("ah/video/compressed", video_qos);

    start_tracking_srv_ = create_service<ah_msgs::srv::StartTracking>(
      "ah/tracking/start",
      std::bind(&AhCoreNode::OnStartTracking, this, std::placeholders::_1, std::placeholders::_2));

    stop_tracking_srv_ = create_service<std_srvs::srv::Trigger>(
      "ah/tracking/stop",
      std::bind(&AhCoreNode::OnStopTracking, this, std::placeholders::_1, std::placeholders::_2));

    cancel_tracking_srv_ = create_service<std_srvs::srv::Trigger>(
      "ah/tracking/cancel",
      std::bind(&AhCoreNode::OnCancelTracking, this, std::placeholders::_1, std::placeholders::_2));

    // Camera open/probe can block for seconds. Keep it off the default group so
    // status/video timers still run (MultiThreadedExecutor in main).
    camera_cb_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

    list_cameras_srv_ = create_service<ah_msgs::srv::ListCameras>(
      "ah/camera/list",
      std::bind(&AhCoreNode::OnListCameras, this, std::placeholders::_1, std::placeholders::_2),
      rclcpp::ServicesQoS(),
      camera_cb_group_);

    select_camera_srv_ = create_service<ah_msgs::srv::SelectCamera>(
      "ah/camera/select",
      std::bind(&AhCoreNode::OnSelectCamera, this, std::placeholders::_1, std::placeholders::_2),
      rclcpp::ServicesQoS(),
      camera_cb_group_);

    const auto period = std::chrono::milliseconds(1000 / kPublishHz);
    timer_ = create_wall_timer(period, std::bind(&AhCoreNode::OnTimer, this));

    // Do NOT probe cameras in the constructor. create_service() advertises names
    // on the graph immediately, but the node cannot handle calls until spin().
    // A multi-second OpenCV probe here made `ros2 service list` show
    // /ah/camera/list while `ros2 service call` hung on wait_for_service.
    camera_cache_.clear();
    camera_cache_.push_back(
      CameraDevice{-1, "synthetic", "Synthetic test pattern", "none"});

    // Probe hardware after the executor is spinning.
    probe_timer_ = create_wall_timer(
      200ms,
      [this]() {
        if (probe_timer_) {
          probe_timer_->cancel();
        }
        RCLCPP_INFO(get_logger(), "camera probe starting (async)…");
        auto devices = EnumerateCameraDevices(camera_.backend);
        const size_t n = devices.size();
        {
          std::lock_guard<std::mutex> lock(camera_mutex_);
          camera_cache_ = std::move(devices);
          camera_probe_done_ = true;
        }
        RCLCPP_INFO(
          get_logger(),
          "camera probe done: %zu device(s) (incl. synthetic)",
          n);
      },
      camera_cb_group_);

    const char * domain = std::getenv("ROS_DOMAIN_ID");
    RCLCPP_INFO(
      get_logger(),
      "ah_core ready: fqn=%s ROS_DOMAIN_ID=%s status+video @ %d Hz; "
      "services ah/tracking/* + ah/camera/list|select; "
      "video.source=%s camera.id=%d path=%s (hardware probe deferred)",
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

private:
  void ApplySelectionToParams()
  {
    set_parameter(rclcpp::Parameter("video.source", camera_.video_source));
    set_parameter(rclcpp::Parameter("camera.device_id", camera_.device_id));
    set_parameter(rclcpp::Parameter("camera.device_path", camera_.device_path));
    set_parameter(rclcpp::Parameter("camera.backend", camera_.backend));
  }

  void PersistSelectionToSettingsFile()
  {
    if (settings_path_.empty()) {
      return;
    }
    // Read existing file, rewrite [Camera] section keys; preserve other sections.
    std::ifstream in(settings_path_);
    if (!in) {
      // Create minimal file with Camera section only.
      std::ofstream out(settings_path_, std::ios::app);
      if (!out) {
        RCLCPP_WARN(get_logger(), "cannot write camera settings to %s", settings_path_.c_str());
        return;
      }
      out << "\n[Camera]\n"
          << "video_source=" << camera_.video_source << '\n'
          << "device_id=" << camera_.device_id << '\n'
          << "device_path=" << camera_.device_path << '\n'
          << "backend=" << camera_.backend << '\n';
      return;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      lines.push_back(line);
    }
    in.close();

    std::vector<std::string> out_lines;
    bool in_camera = false;
    bool wrote_camera = false;
    auto write_camera_block = [&]() {
      out_lines.push_back("[Camera]");
      out_lines.push_back("video_source=" + camera_.video_source);
      out_lines.push_back("device_id=" + std::to_string(camera_.device_id));
      out_lines.push_back("device_path=" + camera_.device_path);
      out_lines.push_back("backend=" + camera_.backend);
      wrote_camera = true;
    };

    for (const auto & l : lines) {
      if (!l.empty() && l.front() == '[') {
        if (in_camera) {
          in_camera = false;
        }
        if (l == "[Camera]") {
          in_camera = true;
          write_camera_block();
          continue;
        }
        out_lines.push_back(l);
        continue;
      }
      if (in_camera) {
        // Skip old Camera keys; block already rewritten.
        continue;
      }
      out_lines.push_back(l);
    }
    if (!wrote_camera) {
      if (!out_lines.empty() && !out_lines.back().empty()) {
        out_lines.push_back("");
      }
      write_camera_block();
    }

    std::ofstream out(settings_path_);
    if (!out) {
      RCLCPP_WARN(get_logger(), "cannot rewrite settings %s", settings_path_.c_str());
      return;
    }
    for (size_t i = 0; i < out_lines.size(); ++i) {
      out << out_lines[i];
      if (i + 1 < out_lines.size()) {
        out << '\n';
      }
    }
    out << '\n';
  }

  void OnTimer()
  {
    const auto now = get_clock()->now();
    ++frame_id_;

    // Task_30: selection is live in params/status; frames still synthetic until Task_31.
    const char * video_status = "connected";

    std_msgs::msg::String status_msg;
    status_msg.data = CreateStatusJson(
      now.seconds(),
      video_status,
      tracking_started_,
      segmentation_active_,
      smart_mode_active_,
      following_active_,
      camera_);
    status_pub_->publish(status_msg);

    cv::Mat bgr = CreateSyntheticImage(frame_id_, tracking_started_);
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, kJpegQuality};
    std::vector<uint8_t> jpeg;
    if (!cv::imencode(".jpg", bgr, jpeg, params)) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "JPEG encode failed");
      return;
    }

    sensor_msgs::msg::CompressedImage video_msg;
    video_msg.header.stamp = now;
    video_msg.header.frame_id = "ah_camera_stub";
    video_msg.format = "jpeg";
    video_msg.data = std::move(jpeg);
    video_pub_->publish(video_msg);
  }

  void OnStartTracking(
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
    tracking_bounding_box_x_ = request->x;
    tracking_bounding_box_y_ = request->y;
    tracking_bounding_box_width_ = request->width;
    tracking_bounding_box_height_ = request->height;

    response->success = true;
    response->message = "tracking started (stub)";
    RCLCPP_INFO(
      get_logger(),
      "tracking started (stub) tracking bounding box norm x=%.3f y=%.3f w=%.3f h=%.3f",
      tracking_bounding_box_x_, tracking_bounding_box_y_, tracking_bounding_box_width_,
      tracking_bounding_box_height_);
  }

  void OnStopTracking(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    const bool was = tracking_started_;
    tracking_started_ = false;
    response->success = true;
    response->message = was ? "tracking stopped" : "tracking already idle";
    RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
  }

  void OnCancelTracking(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    tracking_started_ = false;
    segmentation_active_ = false;
    response->success = true;
    response->message = "tracking cancelled (stub hard reset)";
    RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
  }

  void OnListCameras(
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
      // refresh=true forces re-probe; otherwise wait for async startup probe, or
      // probe once here if the caller races ahead of the timer.
      if (request->refresh || !camera_probe_done_) {
        camera_cache_ = EnumerateCameraDevices(camera_.backend);
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

  void OnSelectCamera(
    const std::shared_ptr<ah_msgs::srv::SelectCamera::Request> request,
    std::shared_ptr<ah_msgs::srv::SelectCamera::Response> response)
  {
    std::string source = Trim(request->video_source);
    int device_id = request->device_id;
    std::string path = Trim(request->device_path);
    std::string backend = Trim(request->backend);

    // Infer source when client only sends id/path.
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
      camera_.video_source = "synthetic";
      camera_.device_id = -1;
      camera_.device_path = "synthetic";
      if (!backend.empty()) {
        camera_.backend = backend;
      }
      ApplySelectionToParams();
      PersistSelectionToSettingsFile();
      response->success = true;
      response->message = "selected synthetic test pattern";
      FillSelectResponse(response);
      RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
      return;
    }

    // Prefer path when non-empty (stable selection across reordering when path is real).
    if (!path.empty()) {
      int parsed = -1;
      if (!ParseDevicePath(path, &parsed) && path.rfind("index:", 0) != 0 &&
        path.find("/dev/video") == std::string::npos)
      {
        // Allow opaque paths: keep path, require device_id if provided.
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

    // Validate device can open (avoids silent dead selection).
    std::string opened_backend;
    const std::string hint = backend.empty() ? camera_.backend : backend;
    if (!TryOpenIndex(device_id, hint, &opened_backend)) {
      response->success = false;
      response->message = "failed to open camera id=" + std::to_string(device_id) +
                         " path=" + path + " (is it connected / permission granted?)";
      FillSelectResponse(response);
      RCLCPP_WARN(get_logger(), "%s", response->message.c_str());
      return;
    }

    camera_.video_source = "camera";
    camera_.device_id = device_id;
    camera_.device_path = path.empty() ? PathForIndex(device_id) : path;
    camera_.backend = backend.empty() ? opened_backend : backend;
    if (camera_.backend.empty()) {
      camera_.backend = DefaultBackendName();
    }

    ApplySelectionToParams();
    PersistSelectionToSettingsFile();

    response->success = true;
    response->message = "selected camera id=" + std::to_string(camera_.device_id) +
                        " path=" + camera_.device_path;
    FillSelectResponse(response);
    RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
  }

  void FillSelectResponse(std::shared_ptr<ah_msgs::srv::SelectCamera::Response> response) const
  {
    response->video_source = camera_.video_source;
    response->device_id = camera_.device_id;
    response->device_path = camera_.device_path;
    response->backend = camera_.backend;
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr video_pub_;
  rclcpp::Service<ah_msgs::srv::StartTracking>::SharedPtr start_tracking_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr stop_tracking_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr cancel_tracking_srv_;
  rclcpp::Service<ah_msgs::srv::ListCameras>::SharedPtr list_cameras_srv_;
  rclcpp::Service<ah_msgs::srv::SelectCamera>::SharedPtr select_camera_srv_;
  rclcpp::CallbackGroup::SharedPtr camera_cb_group_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr probe_timer_;
  int frame_id_{0};

  bool tracking_started_{false};
  bool segmentation_active_{false};
  bool smart_mode_active_{false};
  bool following_active_{false};
  float tracking_bounding_box_x_{0.0f};
  float tracking_bounding_box_y_{0.0f};
  float tracking_bounding_box_width_{0.0f};
  float tracking_bounding_box_height_{0.0f};

  CameraSelection camera_;
  std::string settings_path_;
  std::mutex camera_mutex_;
  std::vector<CameraDevice> camera_cache_;
  bool camera_probe_done_{false};
};

namespace
{
struct RosRuntimeSettings
{
  std::string ros_namespace;  // empty = root
  std::string settings_path;
  CameraSelection camera;
};

std::string SanitizeNamespace(std::string ns)
{
  while (!ns.empty() && (ns.front() == '/' || ns.front() == ' ' || ns.front() == '\t')) {
    ns.erase(ns.begin());
  }
  while (!ns.empty() && (ns.back() == '/' || ns.back() == ' ' || ns.back() == '\t')) {
    ns.pop_back();
  }
  if (ns.find("//") != std::string::npos) {
    return {};
  }
  return ns;
}

// Same Qt-style INI as aero-hub/aerohub_settings.ini:
// [ROS] domain_id=N, namespace=...
// [Camera] video_source, device_id, device_path, backend
// Env ROS_DOMAIN_ID wins over file for domain. Namespace: env AERO_HUB_ROS_NAMESPACE or file.
RosRuntimeSettings LoadRosRuntimeSettings()
{
  RosRuntimeSettings out;

  if (const char * ns_env = std::getenv("AERO_HUB_ROS_NAMESPACE");
    ns_env != nullptr)
  {
    out.ros_namespace = SanitizeNamespace(ns_env);
  }

  const char * candidates[] = {
    std::getenv("AERO_HUB_SETTINGS"),
    "aerohub_settings.ini",
    "../aerohub_settings.ini",
    "../../aerohub_settings.ini",
    "/aero-hub/aerohub_settings.ini",
  };

  bool domain_from_env = (std::getenv("ROS_DOMAIN_ID") != nullptr &&
    std::getenv("ROS_DOMAIN_ID")[0] != '\0');
  bool namespace_from_env = (std::getenv("AERO_HUB_ROS_NAMESPACE") != nullptr);

  for (const char * path : candidates) {
    if (path == nullptr || path[0] == '\0') {
      continue;
    }
    std::ifstream in(path);
    if (!in) {
      continue;
    }
    out.settings_path = path;
    std::string line;
    bool in_ros = false;
    bool in_camera = false;
    while (std::getline(in, line)) {
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      if (line.empty() || line[0] == ';' || line[0] == '#') {
        continue;
      }
      if (line.front() == '[') {
        in_ros = (line == "[ROS]");
        in_camera = (line == "[Camera]");
        continue;
      }
      const auto eq = line.find('=');
      if (eq == std::string::npos) {
        continue;
      }
      const std::string key = Trim(line.substr(0, eq));
      std::string val = Trim(line.substr(eq + 1));
      if (in_ros) {
        if (key == "domain_id" && !val.empty() && !domain_from_env) {
          setenv("ROS_DOMAIN_ID", val.c_str(), 1);
        }
        if (key == "namespace" && !namespace_from_env) {
          out.ros_namespace = SanitizeNamespace(val);
        }
      }
      if (in_camera) {
        if (key == "video_source" && !val.empty()) {
          out.camera.video_source = val;
        }
        if (key == "device_id" && !val.empty()) {
          try {
            out.camera.device_id = std::stoi(val);
          } catch (...) {
          }
        }
        if (key == "device_path") {
          out.camera.device_path = val;
        }
        if (key == "backend") {
          out.camera.backend = val;
        }
      }
    }
    break;  // first readable settings file wins
  }

  // Normalize synthetic selection.
  if (out.camera.video_source == "synthetic" || out.camera.device_path == "synthetic") {
    out.camera.video_source = "synthetic";
    out.camera.device_id = -1;
    if (out.camera.device_path.empty()) {
      out.camera.device_path = "synthetic";
    }
  }

  return out;
}
}  // namespace

int main(int argc, char ** argv)
{
  const RosRuntimeSettings runtime = LoadRosRuntimeSettings();
  rclcpp::init(argc, argv);
  // Multi-threaded so camera probe / list refresh cannot starve status+video
  // (camera callbacks use a dedicated callback group).
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<AhCoreNode>(
    runtime.ros_namespace, runtime.camera, runtime.settings_path);
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
