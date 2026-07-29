#pragma once

#include <memory>

#include <QObject>
#include <QString>

#include "ah_msgs/srv/start_tracking.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/trigger.hpp"

/// QML-facing client for `/ah/tracking/{start,stop,cancel}` (Task_19 / Task_20).
/// Service calls are async; responses are marshalled onto the Qt thread.
class AhTrackController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ Busy NOTIFY BusyChanged)
  Q_PROPERTY(QString lastMessage READ LastMessage NOTIFY LastMessageChanged)
  Q_PROPERTY(bool lastSuccess READ LastSuccess NOTIFY LastSuccessChanged)
  // NOTIFY names are camelCase so QML Connections (onBboxChanged) resolve correctly.
  Q_PROPERTY(float bboxX READ BboxX WRITE SetBboxX NOTIFY bboxChanged)
  Q_PROPERTY(float bboxY READ BboxY WRITE SetBboxY NOTIFY bboxChanged)
  Q_PROPERTY(float bboxWidth READ BboxWidth WRITE SetBboxWidth NOTIFY bboxChanged)
  Q_PROPERTY(float bboxHeight READ BboxHeight WRITE SetBboxHeight NOTIFY bboxChanged)

 public:
  explicit AhTrackController(rclcpp::Node::SharedPtr node, QObject* parent = nullptr);

  [[nodiscard]] bool Busy() const { return busy_; }
  [[nodiscard]] QString LastMessage() const { return last_message_; }
  [[nodiscard]] bool LastSuccess() const { return last_success_; }
  [[nodiscard]] float BboxX() const { return bbox_x_; }
  [[nodiscard]] float BboxY() const { return bbox_y_; }
  [[nodiscard]] float BboxWidth() const { return bbox_width_; }
  [[nodiscard]] float BboxHeight() const { return bbox_height_; }

  void SetBboxX(float value);
  void SetBboxY(float value);
  void SetBboxWidth(float value);
  void SetBboxHeight(float value);

 public slots:
  /// Calls `/ah/tracking/start` with current normalized bbox [0,1].
  void StartTracking();
  /// Calls `/ah/tracking/stop`.
  void StopTracking();
  /// Calls `/ah/tracking/cancel` (hard reset).
  void CancelTracking();
  /// Reset bbox to a default center box.
  void ResetBbox();

 signals:
  void BusyChanged();
  void LastMessageChanged();
  void LastSuccessChanged();
  void bboxChanged();
  void CommandFinished(bool success, const QString& message);

 private:
  void SetBusy(bool busy);
  void SetResult(bool success, const QString& message);
  void FinishOnUiThread(bool success, const QString& message);

  rclcpp::Node::SharedPtr node_;
  rclcpp::Client<ah_msgs::srv::StartTracking>::SharedPtr start_client_;
  rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr stop_client_;
  rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr cancel_client_;

  bool busy_ = false;
  bool last_success_ = false;
  QString last_message_ = QStringLiteral("Ready");
  float bbox_x_ = 0.35f;
  float bbox_y_ = 0.35f;
  float bbox_width_ = 0.30f;
  float bbox_height_ = 0.30f;
};
