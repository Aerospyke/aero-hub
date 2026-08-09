#pragma once

// Status JSON creation and ah/detections JSON parsing (hand-rolled; no JSON lib).

#include "ah_core/camera_devices.hpp"

#include <string>
#include <vector>

namespace ah_core
{

struct DetectionBox
{
  int track_id{-1};  // Ultralytics multi-object track id (-1 if unknown)
  float x{0.f};
  float y{0.f};
  float w{0.f};
  float h{0.f};
  float confidence{0.f};
  std::string class_name;
};

/// Escape a string for embedding in a JSON string value.
std::string JsonEscape(const std::string & s);

/// Build /ah/system/status payload (ros2-interface-map §3.1).
std::string CreateStatusJson(
  double stamp_sec,
  const char * video_status,
  bool tracking_started,
  bool segmentation_active,
  bool is_ai_tracking_active,
  bool following_active,
  const char * tracker_type,
  const CameraSelection & cam,
  float track_x, float track_y, float track_w, float track_h,
  int locked_track_id);

/// Parse ah_yolo detections JSON into boxes (pairs track_id[i] with bbox_normalized[i]).
std::vector<DetectionBox> ParseDetectionsJson(const std::string & json);

}  // namespace ah_core
