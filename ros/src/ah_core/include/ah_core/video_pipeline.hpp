#pragma once

// Synthetic frames, live grab orchestration, and JPEG encode for /ah/video/compressed.

#include "ah_core/camera_devices.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace ah_core
{

constexpr int kVideoFrameWidth = 640;
constexpr int kVideoFrameHeight = 480;
constexpr int kVideoJpegQuality = 80;
constexpr int kVideoPublishHz = 10;

/// Result of one publish tick: BGR frame + status strings for status JSON / ROS headers.
struct VideoGrabResult
{
  cv::Mat bgr;
  std::string frame_id{"ah_camera_stub"};
  std::string video_status{"connected"};  // connected | degraded | unavailable
  /// Non-empty when Open() just failed (caller may log, rate-limited).
  std::string capture_open_error;
  /// True when a live camera was expected but grab failed (caller may warn).
  bool live_grab_failed{false};
};

cv::Mat CreateSyntheticImage(int frame_id, bool tracking_started);

void OverlayTrackingHint(cv::Mat & bgr, bool tracking_started);

/// Grab one publishable frame from capture or synthetic fallback.
/// Caller must hold any mutex that guards @p capture and @p camera if shared.
VideoGrabResult GrabPublishableFrame(
  CameraCapture & capture,
  const CameraSelection & camera,
  bool camera_probe_done,
  bool tracking_started,
  int frame_id);

/// Encode BGR to JPEG bytes. Returns false on failure.
bool EncodeJpeg(
  const cv::Mat & bgr,
  std::vector<uint8_t> * out_jpeg,
  int quality = kVideoJpegQuality);

}  // namespace ah_core
