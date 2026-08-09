// Classic + AI tracking state machine.

#include "ah_core/tracking_controller.hpp"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace ah_core
{

bool TrackingController::IsBoundingBoxNormalized(float x, float y, float width, float height)
{
  return x >= 0.0f && y >= 0.0f && width > 0.0f && height > 0.0f &&
         x <= 1.0f && y <= 1.0f && width <= 1.0f && height <= 1.0f &&
         (x + width) <= 1.0001f && (y + height) <= 1.0001f;
}

TrackingOpResult TrackingController::StartClassic(
  float x, float y, float width, float height)
{
  TrackingOpResult result;
  if (!IsBoundingBoxNormalized(x, y, width, height)) {
    result.success = false;
    result.message =
      "tracking bounding box must be normalized in [0,1] with positive width/height (interface map §4)";
    return result;
  }

  was_tracking_started_ = true;
  tracker_type_ = "classic";
  // Classic bbox must not keep following a previous AI track_id.
  currently_locked_object_id_ = -1;
  current_lock_num_missed_frames_ = 0;
  tracking_bounding_box_x_ = x;
  tracking_bounding_box_y_ = y;
  tracking_bounding_box_width_ = width;
  tracking_bounding_box_height_ = height;

  result.success = true;
  result.message = "tracking started (classic bbox)";
  result.publish_status = true;
  return result;
}

TrackingOpResult TrackingController::Stop()
{
  TrackingOpResult result;
  const bool was = was_tracking_started_;
  was_tracking_started_ = false;
  currently_locked_object_id_ = -1;
  current_lock_num_missed_frames_ = 0;
  if (!is_ai_tracking_active_) {
    tracker_type_ = "stub";
  }
  result.success = true;
  result.message = was ? "tracking stopped" : "tracking already idle";
  return result;
}

TrackingOpResult TrackingController::Cancel()
{
  TrackingOpResult result;
  was_tracking_started_ = false;
  is_segmentation_active_ = false;
  currently_locked_object_id_ = -1;
  current_lock_num_missed_frames_ = 0;
  if (!is_ai_tracking_active_) {
    tracker_type_ = "stub";
  } else {
    tracker_type_ = "ai_tracking";
  }
  result.success = true;
  result.message = "tracking cancelled (hard reset)";
  return result;
}

TrackingOpResult TrackingController::SetAiTrackingActive(bool active)
{
  TrackingOpResult result;
  is_ai_tracking_active_ = active;

  if (active) {
    // Enter AI tracking: clear classic framing; lock only after click-on-detection.
    was_tracking_started_ = false;
    currently_locked_object_id_ = -1;
    current_lock_num_missed_frames_ = 0;
    tracking_bounding_box_x_ = 0.f;
    tracking_bounding_box_y_ = 0.f;
    tracking_bounding_box_width_ = 0.f;
    tracking_bounding_box_height_ = 0.f;
    tracker_type_ = "ai_tracking";
  } else {
    // Leave AI tracking: stop any lock; classic drag box returns on the UI.
    was_tracking_started_ = false;
    currently_locked_object_id_ = -1;
    current_lock_num_missed_frames_ = 0;
    tracker_type_ = "stub";
    tracking_bounding_box_x_ = 0.35f;
    tracking_bounding_box_y_ = 0.35f;
    tracking_bounding_box_width_ = 0.30f;
    tracking_bounding_box_height_ = 0.30f;
  }

  result.success = true;
  result.message = is_ai_tracking_active_ ? "ai tracking ON (click detections only)"
                                          : "ai tracking OFF (classic drag)";
  result.publish_status = true;
  return result;
}

TrackingOpResult TrackingController::AiClick(float x, float y)
{
  TrackingOpResult result;

  if (!is_ai_tracking_active_) {
    result.success = false;
    result.message = "ai tracking is OFF — enable ah/ai_tracking/toggle first";
    return result;
  }
  if (x < 0.f || x > 1.f || y < 0.f || y > 1.f) {
    result.success = false;
    result.message = "click must be normalized in [0,1]";
    return result;
  }

  float bx = 0.f, by = 0.f, bw = 0.f, bh = 0.f;
  int track_id = -1;
  std::string label;
  if (!ResolveAiClickLock(x, y, &bx, &by, &bw, &bh, &track_id, &label)) {
    // Miss: do not start tracking and do not invent a free-point box.
    was_tracking_started_ = false;
    currently_locked_object_id_ = -1;
    current_lock_num_missed_frames_ = 0;
    tracking_bounding_box_x_ = 0.f;
    tracking_bounding_box_y_ = 0.f;
    tracking_bounding_box_width_ = 0.f;
    tracking_bounding_box_height_ = 0.f;
    result.success = false;
    result.message = "Missed click, reason: " + label;
    result.publish_status = true;
    return result;
  }

  if (track_id < 0) {
    result.success = false;
    result.message =
      "detection has no track_id — is ah_yolo using model.track(persist=True)?";
    return result;
  }

  // Explicitly replace any previous lock (do not leave old track_id sticky).
  was_tracking_started_ = true;
  tracker_type_ = "ai_tracking";
  currently_locked_object_id_ = track_id;
  current_lock_num_missed_frames_ = 0;
  tracking_bounding_box_x_ = bx;
  tracking_bounding_box_y_ = by;
  tracking_bounding_box_width_ = bw;
  tracking_bounding_box_height_ = bh;

  result.success = true;
  result.lock_x = bx;
  result.lock_y = by;
  result.lock_width = bw;
  result.lock_height = bh;
  result.message = "ai tracking lock on [" + label + "] track_id=" +
                   std::to_string(track_id);
  result.publish_status = true;
  return result;
}

TrackingIngestResult TrackingController::IngestDetections(const std::string & json)
{
  TrackingIngestResult result;
  auto parsed = ParseDetectionsJson(json);
  {
    std::lock_guard<std::mutex> lock(detections_mutex_);
    last_detections_ = std::move(parsed);
  }
  int lost_id = -1;
  int missed = 0;
  if (UpdateLockFromTrackedId(&lost_id, &missed)) {
    result.lock_lost = true;
    result.lost_track_id = lost_id;
    result.missed_frames = missed;
  }
  return result;
}

bool TrackingController::UpdateLockFromTrackedId(int * out_lost_id, int * out_missed)
{
  if (!was_tracking_started_ || currently_locked_object_id_ < 0) {
    return false;
  }

  std::vector<DetectionBox> detected_objects;
  {
    std::lock_guard<std::mutex> lock(detections_mutex_);
    detected_objects = last_detections_;
  }

  for (const auto & object_bounding_box : detected_objects) {
    if (object_bounding_box.track_id == currently_locked_object_id_) {
      tracking_bounding_box_x_ = object_bounding_box.x;
      tracking_bounding_box_y_ = object_bounding_box.y;
      tracking_bounding_box_width_ = object_bounding_box.w;
      tracking_bounding_box_height_ = object_bounding_box.h;
      current_lock_num_missed_frames_ = 0;
      return false;
    }
  }

  // Track ID missing this frame — keep last box for a short grace period.
  current_lock_num_missed_frames_++;
  if (current_lock_num_missed_frames_ >= kMaxMissedTrackingFrames) {
    if (out_lost_id) {
      *out_lost_id = currently_locked_object_id_;
    }
    if (out_missed) {
      *out_missed = current_lock_num_missed_frames_;
    }
    was_tracking_started_ = false;
    currently_locked_object_id_ = -1;
    current_lock_num_missed_frames_ = 0;
    tracking_bounding_box_x_ = 0.f;
    tracking_bounding_box_y_ = 0.f;
    tracking_bounding_box_width_ = 0.f;
    tracking_bounding_box_height_ = 0.f;
    return true;
  }
  return false;
}

bool TrackingController::ResolveAiClickLock(
  float click_x, float click_y,
  float * out_x, float * out_y, float * out_w, float * out_h,
  int * out_track_id,
  std::string * out_label) const
{
  // AI tracking lock only on a detection that contains the click (no free-point box).
  // When several boxes contain the point (overlap / nested), pick the one whose
  // *center* is closest to the click — not merely the smallest area.
  std::vector<DetectionBox> detected_objects;
  {
    std::lock_guard<std::mutex> lock(detections_mutex_);
    detected_objects = last_detections_;
  }

  float best_dist2 = 1e30f;
  float best_area = 1e30f;
  DetectionBox best_matched_box;
  bool did_click_in_box = false;

  for (const auto & object_bounding_box : detected_objects) {
    const float x1 = object_bounding_box.x;
    const float y1 = object_bounding_box.y;
    const float x2 = object_bounding_box.x + object_bounding_box.w;
    const float y2 = object_bounding_box.y + object_bounding_box.h;
    if (click_x < x1 || click_x > x2 || click_y < y1 || click_y > y2) {
      continue;
    }
    const float cx = object_bounding_box.x + 0.5f * object_bounding_box.w;
    const float cy = object_bounding_box.y + 0.5f * object_bounding_box.h;
    const float dx = click_x - cx;
    const float dy = click_y - cy;
    const float dist2 = dx * dx + dy * dy;
    const float area = object_bounding_box.w * object_bounding_box.h;
    // Prefer closer center; break ties with smaller area.
    if (dist2 < best_dist2 - 1e-12f ||
        (std::fabs(dist2 - best_dist2) <= 1e-12f && area < best_area))
    {
      best_dist2 = dist2;
      best_area = area;
      best_matched_box = object_bounding_box;
      did_click_in_box = true;
    }
  }

  if (!did_click_in_box) {
    if (out_label) {
      *out_label = detected_objects.empty()
        ? "No Detections Available To Click"
        : "Did Not Click In A Detected Bounding Box";
    }
    if (out_track_id) {
      *out_track_id = -1;
    }
    return false;
  }

  *out_x = best_matched_box.x;
  *out_y = best_matched_box.y;
  *out_w = best_matched_box.w;
  *out_h = best_matched_box.h;
  if (out_track_id) {
    *out_track_id = best_matched_box.track_id;
  }
  if (out_label) {
    if (best_matched_box.track_id >= 0) {
      *out_label = (best_matched_box.class_name.empty() ? "obj" : best_matched_box.class_name) +
                   " id=" + std::to_string(best_matched_box.track_id);
    } else {
      *out_label = best_matched_box.class_name.empty()
        ? "detection (no track_id)"
        : best_matched_box.class_name;
    }
  }
  return true;
}

}  // namespace ah_core
