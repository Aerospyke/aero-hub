#pragma once

#include <memory>

#include <QObject>
#include <QString>
#include <QStringList>

#include "ah_msgs/srv/set_yolo_profile.hpp"
#include "rclcpp/rclcpp.hpp"

/// QML client for `ah/yolo/set_profile` — runtime tank | coco80 switch.
class AhYoloController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ Busy NOTIFY BusyChanged)
  Q_PROPERTY(QString lastMessage READ LastMessage NOTIFY LastMessageChanged)
  Q_PROPERTY(bool lastSuccess READ LastSuccess NOTIFY LastSuccessChanged)
  /// Combo labels (display).
  Q_PROPERTY(QStringList profileLabels READ ProfileLabels CONSTANT)
  /// Combo values passed to the service (parallel to profileLabels).
  Q_PROPERTY(QStringList profileIds READ ProfileIds CONSTANT)
  Q_PROPERTY(QString activeProfile READ ActiveProfile NOTIFY profileChanged)
  Q_PROPERTY(QString activeWeights READ ActiveWeights NOTIFY profileChanged)
  Q_PROPERTY(int selectedListIndex READ SelectedListIndex NOTIFY profileChanged)

 public:
  explicit AhYoloController(rclcpp::Node::SharedPtr node, QObject* parent = nullptr);

  [[nodiscard]] bool Busy() const { return busy_; }
  [[nodiscard]] QString LastMessage() const { return last_message_; }
  [[nodiscard]] bool LastSuccess() const { return last_success_; }
  [[nodiscard]] QStringList ProfileLabels() const { return profile_labels_; }
  [[nodiscard]] QStringList ProfileIds() const { return profile_ids_; }
  [[nodiscard]] QString ActiveProfile() const { return active_profile_; }
  [[nodiscard]] QString ActiveWeights() const { return active_weights_; }
  [[nodiscard]] int SelectedListIndex() const { return selected_list_index_; }

 public slots:
  /// Apply profile id ("tank" | "coco80"). Optional weights path override (usually empty).
  void SetProfile(const QString& profile_id, const QString& weights_path = QString());
  /// Combo index into profileIds.
  void SetProfileAt(int list_index);
  /// Sync combo selection from detections stream / known active profile string.
  void SyncFromProfileName(const QString& profile);

 signals:
  void BusyChanged();
  void LastMessageChanged();
  void LastSuccessChanged();
  void profileChanged();
  void CommandFinished(bool success, const QString& message);

 private:
  void SetBusy(bool busy);
  void SetResult(bool success, const QString& message);
  void FinishOnUiThread(bool success, const QString& message);
  void ApplyActive(const QString& profile, const QString& weights);
  int IndexForProfile(const QString& profile) const;

  rclcpp::Node::SharedPtr node_;
  rclcpp::Client<ah_msgs::srv::SetYoloProfile>::SharedPtr set_profile_client_;

  bool busy_ = false;
  bool last_success_ = false;
  QString last_message_ = QStringLiteral("Select a YOLO profile (ah_yolo must be running)");

  QStringList profile_labels_;
  QStringList profile_ids_;
  QString active_profile_;
  QString active_weights_;
  int selected_list_index_ = 0;
};
