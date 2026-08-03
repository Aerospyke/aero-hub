#include "ah_yolo_controller.h"

#include "ah_ros_names.h"

#include <QFileInfo>
#include <QMetaObject>

#include <utility>

AhYoloController::AhYoloController(rclcpp::Node::SharedPtr node, QObject* parent)
    : QObject(parent), node_(std::move(node)) {
  profile_labels_ = {QStringLiteral("COCO-80 (general)"), QStringLiteral("Mini tank")};
  profile_ids_ = {QStringLiteral("coco80"), QStringLiteral("tank")};
  active_profile_ = QStringLiteral("coco80");
  selected_list_index_ = 0;

  set_profile_client_ =
      node_->create_client<ah_msgs::srv::SetYoloProfile>(ah_ros_names::YoloSetProfileService);
}

int AhYoloController::IndexForProfile(const QString& profile) const {
  const QString p = profile.trimmed().toLower();
  if (p == QLatin1String("tank") || p == QLatin1String("mini_tank") ||
      p == QLatin1String("minitank")) {
    return profile_ids_.indexOf(QStringLiteral("tank"));
  }
  if (p == QLatin1String("coco80") || p == QLatin1String("coco") ||
      p == QLatin1String("coco-80")) {
    return profile_ids_.indexOf(QStringLiteral("coco80"));
  }
  return profile_ids_.indexOf(p);
}

void AhYoloController::ApplyActive(const QString& profile, const QString& weights) {
  const QString norm = profile.trimmed().toLower();
  QString canonical = norm;
  if (norm == QLatin1String("coco") || norm == QLatin1String("coco-80")) {
    canonical = QStringLiteral("coco80");
  } else if (norm == QLatin1String("mini_tank") || norm == QLatin1String("minitank")) {
    canonical = QStringLiteral("tank");
  }

  bool changed = false;
  if (canonical != active_profile_ && !canonical.isEmpty()) {
    active_profile_ = canonical;
    changed = true;
  }
  if (weights != active_weights_) {
    active_weights_ = weights;
    changed = true;
  }
  const int idx = IndexForProfile(active_profile_);
  if (idx >= 0 && idx != selected_list_index_) {
    selected_list_index_ = idx;
    changed = true;
  }
  if (changed) {
    emit profileChanged();
  }
}

void AhYoloController::SyncFromProfileName(const QString& profile) {
  if (busy_ || profile.trimmed().isEmpty()) {
    return;
  }
  ApplyActive(profile, active_weights_);
}

void AhYoloController::SetBusy(bool busy) {
  if (busy_ == busy) {
    return;
  }
  busy_ = busy;
  emit BusyChanged();
}

void AhYoloController::SetResult(bool success, const QString& message) {
  if (last_success_ != success) {
    last_success_ = success;
    emit LastSuccessChanged();
  }
  if (last_message_ != message) {
    last_message_ = message;
    emit LastMessageChanged();
  }
}

void AhYoloController::FinishOnUiThread(bool success, const QString& message) {
  QMetaObject::invokeMethod(
      this,
      [this, success, message]() {
        SetBusy(false);
        SetResult(success, message);
        emit CommandFinished(success, message);
      },
      Qt::QueuedConnection);
}

void AhYoloController::SetProfileAt(int list_index) {
  if (list_index < 0 || list_index >= profile_ids_.size()) {
    SetResult(false, QStringLiteral("Invalid profile index"));
    emit CommandFinished(false, last_message_);
    return;
  }
  SetProfile(profile_ids_.at(list_index), QString());
}

void AhYoloController::SetProfile(const QString& profile_id, const QString& weights_path) {
  if (busy_) {
    return;
  }
  const QString id = profile_id.trimmed().toLower();
  if (id.isEmpty()) {
    SetResult(false, QStringLiteral("Profile id is empty"));
    emit CommandFinished(false, last_message_);
    return;
  }
  if (!set_profile_client_->service_is_ready()) {
    SetResult(false, QStringLiteral("Service ah/yolo/set_profile not available (is ah_yolo running?)"));
    emit CommandFinished(false, last_message_);
    return;
  }

  SetBusy(true);
  auto request = std::make_shared<ah_msgs::srv::SetYoloProfile::Request>();
  request->profile = id.toStdString();
  request->weights_path = weights_path.trimmed().toStdString();

  set_profile_client_->async_send_request(
      request, [this](rclcpp::Client<ah_msgs::srv::SetYoloProfile>::SharedFuture future) {
        try {
          const auto response = future.get();
          const bool ok = response->success;
          const QString msg = QString::fromStdString(response->message);
          const QString profile = QString::fromStdString(response->active_profile);
          const QString weights = QString::fromStdString(response->weights_path);
          QMetaObject::invokeMethod(
              this,
              [this, ok, msg, profile, weights]() {
                if (ok) {
                  ApplyActive(profile, weights);
                  // Short display path for the footer line
                  const QString base = QFileInfo(weights).fileName();
                  SetResult(true, base.isEmpty() ? msg : QStringLiteral("%1 · %2").arg(profile, base));
                } else {
                  SetResult(false, msg);
                }
                SetBusy(false);
                emit CommandFinished(ok, last_message_);
              },
              Qt::QueuedConnection);
        } catch (const std::exception& ex) {
          FinishOnUiThread(false, QStringLiteral("Set YOLO profile failed: %1").arg(ex.what()));
        }
      });
}
