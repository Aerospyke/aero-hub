// Camera/video processing for ah_core (synthetic pattern, live grab, JPEG).

#include "ah_core/video_pipeline.hpp"

#include <string>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace ah_core
{

cv::Mat CreateSyntheticImage(const int frame_id, bool tracking_started)
{
  cv::Mat frame(kVideoFrameHeight, kVideoFrameWidth, CV_8UC3, cv::Scalar(20, 24, 32));

  const int band_h = 40;
  for (int i = 0; i < 6; ++i) {
    const cv::Scalar colors[] = {
      {0, 0, 200}, {0, 200, 200}, {0, 200, 0},
      {200, 200, 0}, {200, 0, 0}, {200, 0, 200},
    };
    cv::rectangle(
      frame,
      cv::Rect(0, i * band_h, kVideoFrameWidth, band_h),
      colors[i],
      cv::FILLED);
  }

  const int box = 48;
  const int span_x = kVideoFrameWidth - box;
  const int span_y = kVideoFrameHeight - box - 6 * band_h;
  const int x = (frame_id * 3) % (span_x > 0 ? span_x : 1);
  const int y = 6 * band_h + ((frame_id * 2) % (span_y > 0 ? span_y : 1));
  cv::rectangle(frame, cv::Rect(x, y, box, box), cv::Scalar(255, 255, 255), cv::FILLED);
  cv::rectangle(frame, cv::Rect(x, y, box, box), cv::Scalar(0, 140, 255), 2);

  cv::putText(
    frame,
    tracking_started ? "AeroHub ah_core TRACKING" : "AeroHub ah_core synthetic",
    cv::Point(12, kVideoFrameHeight - 36),
    cv::FONT_HERSHEY_SIMPLEX,
    0.7,
    tracking_started ? cv::Scalar(80, 255, 120) : cv::Scalar(230, 230, 230),
    2);
  cv::putText(
    frame,
    "frame " + std::to_string(frame_id),
    cv::Point(12, kVideoFrameHeight - 12),
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

VideoGrabResult GrabPublishableFrame(
  CameraCapture & capture,
  const CameraSelection & camera,
  bool camera_probe_done,
  bool tracking_started,
  int frame_id)
{
  VideoGrabResult result;

  if (camera.video_source == "camera" && camera.device_id >= 0) {
    // Wait for async probe (or select) before fighting AVFoundation opens.
    if (!camera_probe_done && !capture.IsOpen()) {
      result.bgr = CreateSyntheticImage(frame_id, tracking_started);
      result.video_status = "connected";
    } else {
      if (!capture.IsOpen() || capture.DeviceId() != camera.device_id) {
        std::string err;
        if (!capture.Open(camera, &err)) {
          result.video_status = "unavailable";
          result.capture_open_error = err;
          result.bgr = CreateSyntheticImage(frame_id, tracking_started);
        }
      }
      if (capture.IsOpen() && capture.ReadBgr(&result.bgr) && !result.bgr.empty()) {
        OverlayTrackingHint(result.bgr, tracking_started);
        result.frame_id = "ah_camera";
        result.video_status = "connected";
      } else if (result.bgr.empty()) {
        result.video_status = capture.IsOpen() ? "degraded" : "unavailable";
        result.live_grab_failed = true;
        result.bgr = CreateSyntheticImage(frame_id, tracking_started);
      }
    }
  } else {
    if (capture.IsOpen()) {
      capture.Close();
    }
    result.bgr = CreateSyntheticImage(frame_id, tracking_started);
    result.video_status = "connected";
  }

  return result;
}

bool EncodeJpeg(
  const cv::Mat & bgr,
  std::vector<uint8_t> * out_jpeg,
  int quality)
{
  if (!out_jpeg || bgr.empty()) {
    return false;
  }
  const std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};
  return cv::imencode(".jpg", bgr, *out_jpeg, params);
}

}  // namespace ah_core
