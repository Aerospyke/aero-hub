#pragma once

// Classic + AI tracking state machine for ah_core.
// Owns bbox lock, detections cache, and track_id follow / drop logic.
// ROS service wiring stays on AhCoreNode.

#include "ah_core/messages.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace ah_core
{

/// Result of a tracking command (start/stop/cancel/toggle/click).
struct TrackingOpResult
{
  bool success{true};
  std::string message;
  /// Caller should publish /ah/system/status immediately when true.
  bool publish_status{false};
  /// Filled by AiClick on success (and zeroed on miss).
  float lock_x{0.f};
  float lock_y{0.f};
  float lock_width{0.f};
  float lock_height{0.f};
};

/// Optional detail when IngestDetections clears a stuck AI lock.
struct TrackingIngestResult
{
  /// True when lock was dropped after MaxMissedTrackingFrames.
  bool lock_lost{false};
  int lost_track_id{-1};
  int missed_frames{0};
};

class TrackingController
{
public:
  static constexpr int kMaxMissedTrackingFrames = 15;

  TrackingController() = default;

  /// Classic bbox track (normalized [0,1]). Rejects invalid boxes.
  TrackingOpResult StartClassic(float x, float y, float width, float height);

  TrackingOpResult Stop();
  TrackingOpResult Cancel();

  /// Enter/leave AI tracking mode (clears or restores classic UI defaults).
  TrackingOpResult SetAiTrackingActive(bool active);

  /// Click-to-lock on a detection containing (x,y). Requires AI mode + track_id.
  TrackingOpResult AiClick(float x, float y);

  /// Parse detections JSON, store boxes, refresh lock from track_id.
  TrackingIngestResult IngestDetections(const std::string & json);

  // --- Status / video accessors ---
  [[nodiscard]] bool WasTrackingStarted() const { return was_tracking_started_; }
  [[nodiscard]] bool IsSegmentationActive() const { return is_segmentation_active_; }
  [[nodiscard]] bool IsAiTrackingActive() const { return is_ai_tracking_active_; }
  [[nodiscard]] bool IsFollowingActive() const { return is_following_active_; }
  [[nodiscard]] const std::string & TrackerType() const { return tracker_type_; }
  [[nodiscard]] float BboxX() const { return tracking_bounding_box_x_; }
  [[nodiscard]] float BboxY() const { return tracking_bounding_box_y_; }
  [[nodiscard]] float BboxW() const { return tracking_bounding_box_width_; }
  [[nodiscard]] float BboxH() const { return tracking_bounding_box_height_; }
  [[nodiscard]] int LockedTrackId() const { return currently_locked_object_id_; }

private:
  static bool IsBoundingBoxNormalized(float x, float y, float width, float height);

  /// Pick lock target from click + last detections (must hit a box).
  bool ResolveAiClickLock(
    float click_x, float click_y,
    float * out_x, float * out_y, float * out_w, float * out_h,
    int * out_track_id,
    std::string * out_label) const;

  /// If locked track_id is set, refresh lock bbox from latest detections.
  /// Returns true when lock was cleared after sustained miss.
  bool UpdateLockFromTrackedId(int * out_lost_id, int * out_missed);

  bool was_tracking_started_{false};
  bool is_segmentation_active_{false};
  bool is_ai_tracking_active_{false};
  bool is_following_active_{false};
  float tracking_bounding_box_x_{0.0f};
  float tracking_bounding_box_y_{0.0f};
  float tracking_bounding_box_width_{0.0f};
  float tracking_bounding_box_height_{0.0f};
  std::string tracker_type_{"stub"};

  mutable std::mutex detections_mutex_;
  std::vector<DetectionBox> last_detections_;
  /// When tracking under AI mode: follow this Ultralytics track_id frame-to-frame.
  int currently_locked_object_id_{-1};
  int current_lock_num_missed_frames_{0};
};

}  // namespace ah_core
