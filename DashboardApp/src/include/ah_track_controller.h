#pragma once

#include <memory>

#include <QObject>
#include <QString>

#include "ah_msgs/srv/smart_click.hpp"
#include "ah_msgs/srv/start_tracking.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/set_bool.hpp"
#include "std_srvs/srv/trigger.hpp"

/// QML client for tracking + smart mode (Task_19/20 + Task_35).
/// Service calls are async; responses are marshalled onto the Qt thread.
class AhTrackController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ Busy NOTIFY BusyChanged)
  Q_PROPERTY(QString lastMessage READ LastMessage NOTIFY LastMessageChanged)
  Q_PROPERTY(bool lastSuccess READ LastSuccess NOTIFY LastSuccessChanged)
  // NOTIFY camelCase so QML Connections (onTrackingBoundingBoxChanged) resolve.
  Q_PROPERTY(float trackingBoundingBoxX READ TrackingBoundingBoxX WRITE SetTrackingBoundingBoxX
                 NOTIFY trackingBoundingBoxChanged)
  Q_PROPERTY(float trackingBoundingBoxY READ TrackingBoundingBoxY WRITE SetTrackingBoundingBoxY
                 NOTIFY trackingBoundingBoxChanged)
  Q_PROPERTY(float trackingBoundingBoxWidth READ TrackingBoundingBoxWidth WRITE
                 SetTrackingBoundingBoxWidth NOTIFY trackingBoundingBoxChanged)
  Q_PROPERTY(float trackingBoundingBoxHeight READ TrackingBoundingBoxHeight WRITE
                 SetTrackingBoundingBoxHeight NOTIFY trackingBoundingBoxChanged)

 public:
  explicit AhTrackController(rclcpp::Node::SharedPtr node, QObject* parent = nullptr);

  [[nodiscard]] bool Busy() const { return busy_; }
  [[nodiscard]] QString LastMessage() const { return last_message_; }
  [[nodiscard]] bool LastSuccess() const { return last_success_; }
  [[nodiscard]] float TrackingBoundingBoxX() const { return tracking_bounding_box_x_; }
  [[nodiscard]] float TrackingBoundingBoxY() const { return tracking_bounding_box_y_; }
  [[nodiscard]] float TrackingBoundingBoxWidth() const { return tracking_bounding_box_width_; }
  [[nodiscard]] float TrackingBoundingBoxHeight() const { return tracking_bounding_box_height_; }

  void SetTrackingBoundingBoxX(float value);
  void SetTrackingBoundingBoxY(float value);
  void SetTrackingBoundingBoxWidth(float value);
  void SetTrackingBoundingBoxHeight(float value);

 public slots:
  /// Calls `/ah/tracking/start` with current normalized tracking bounding box [0,1].
  void StartTracking();
  /// Calls `/ah/tracking/stop`.
  void StopTracking();
  /// Calls `/ah/tracking/cancel` (hard reset).
  void CancelTracking();
  /// Reset tracking bounding box to a default center rectangle.
  void ResetTrackingBoundingBox();
  /// Task_35: `ah/smart/toggle` with SetBool data.
  void SetSmartMode(bool enabled);
  /// Task_35: `ah/smart/click` with normalized point [0,1].
  void SmartClick(float x, float y);

 signals:
  void BusyChanged();
  void LastMessageChanged();
  void LastSuccessChanged();
  void trackingBoundingBoxChanged();
  void CommandFinished(bool success, const QString& message);

 private:
  void SetBusy(bool busy);
  void SetResult(bool success, const QString& message);
  void FinishOnUiThread(bool success, const QString& message);

  rclcpp::Node::SharedPtr node_;
  rclcpp::Client<ah_msgs::srv::StartTracking>::SharedPtr start_client_;
  rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr stop_client_;
  rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr cancel_client_;
  rclcpp::Client<std_srvs::srv::SetBool>::SharedPtr smart_toggle_client_;
  rclcpp::Client<ah_msgs::srv::SmartClick>::SharedPtr smart_click_client_;

  bool busy_ = false;
  bool last_success_ = false;
  QString last_message_ = QStringLiteral("Ready");
  float tracking_bounding_box_x_ = 0.35f;
  float tracking_bounding_box_y_ = 0.35f;
  float tracking_bounding_box_width_ = 0.30f;
  float tracking_bounding_box_height_ = 0.30f;
};
