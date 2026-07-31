#pragma once

#include <memory>

#include <QObject>
#include <QString>
#include <QStringList>

#include "ah_msgs/srv/list_cameras.hpp"
#include "ah_msgs/srv/select_camera.hpp"
#include "rclcpp/rclcpp.hpp"

/// QML client for `ah/camera/list` + `ah/camera/select` (Task_32).
/// Thin ROS client — device discovery stays in ah_core.
class AhCameraController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ Busy NOTIFY BusyChanged)
  Q_PROPERTY(QString lastMessage READ LastMessage NOTIFY LastMessageChanged)
  Q_PROPERTY(bool lastSuccess READ LastSuccess NOTIFY LastSuccessChanged)
  /// Display labels for ComboBox (name + path); parallel to device paths/ids.
  Q_PROPERTY(QStringList deviceLabels READ DeviceLabels NOTIFY DevicesChanged)
  Q_PROPERTY(int deviceCount READ DeviceCount NOTIFY DevicesChanged)
  Q_PROPERTY(int selectedListIndex READ SelectedListIndex NOTIFY SelectionChanged)
  Q_PROPERTY(QString currentVideoSource READ CurrentVideoSource NOTIFY SelectionChanged)
  Q_PROPERTY(int currentDeviceId READ CurrentDeviceId NOTIFY SelectionChanged)
  Q_PROPERTY(QString currentDevicePath READ CurrentDevicePath NOTIFY SelectionChanged)
  Q_PROPERTY(QString currentBackend READ CurrentBackend NOTIFY SelectionChanged)

 public:
  explicit AhCameraController(rclcpp::Node::SharedPtr node, QObject* parent = nullptr);

  [[nodiscard]] bool Busy() const { return busy_; }
  [[nodiscard]] QString LastMessage() const { return last_message_; }
  [[nodiscard]] bool LastSuccess() const { return last_success_; }
  [[nodiscard]] QStringList DeviceLabels() const { return device_labels_; }
  [[nodiscard]] int DeviceCount() const { return device_labels_.size(); }
  [[nodiscard]] int SelectedListIndex() const { return selected_list_index_; }
  [[nodiscard]] QString CurrentVideoSource() const { return current_video_source_; }
  [[nodiscard]] int CurrentDeviceId() const { return current_device_id_; }
  [[nodiscard]] QString CurrentDevicePath() const { return current_device_path_; }
  [[nodiscard]] QString CurrentBackend() const { return current_backend_; }

 public slots:
  /// Call ah/camera/list. refresh=true re-probes on core.
  void RefreshDevices(bool refresh = true);
  /// Select entry at index into the last list response (no hard-coded IDs).
  void SelectDeviceAt(int list_index);

 signals:
  void BusyChanged();
  void LastMessageChanged();
  void LastSuccessChanged();
  void DevicesChanged();
  void SelectionChanged();
  void CommandFinished(bool success, const QString& message);

 private:
  void SetBusy(bool busy);
  void SetResult(bool success, const QString& message);
  void FinishOnUiThread(bool success, const QString& message);
  void ApplyListResponse(const ah_msgs::srv::ListCameras::Response& response);
  void ApplySelectResponse(const ah_msgs::srv::SelectCamera::Response& response);
  void UpdateSelectedListIndexFromCurrent();

  rclcpp::Node::SharedPtr node_;
  rclcpp::Client<ah_msgs::srv::ListCameras>::SharedPtr list_client_;
  rclcpp::Client<ah_msgs::srv::SelectCamera>::SharedPtr select_client_;

  bool busy_ = false;
  bool last_success_ = false;
  QString last_message_ = QStringLiteral("Refresh to load cameras from ah_core");

  QStringList device_labels_;
  QList<int> device_ids_;
  QStringList device_paths_;
  QStringList device_backends_;

  int selected_list_index_ = -1;
  QString current_video_source_ = QStringLiteral("synthetic");
  int current_device_id_ = -1;
  QString current_device_path_ = QStringLiteral("synthetic");
  QString current_backend_;
};
