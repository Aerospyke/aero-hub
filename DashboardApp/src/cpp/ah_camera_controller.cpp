#include "ah_camera_controller.h"

#include "ah_ros_names.h"

#include <QMetaObject>

#include <utility>

AhCameraController::AhCameraController(rclcpp::Node::SharedPtr node, QObject* parent)
    : QObject(parent), node_(std::move(node)) {
  list_client_ = node_->create_client<ah_msgs::srv::ListCameras>(ah_ros_names::CameraListService);
  select_client_ =
      node_->create_client<ah_msgs::srv::SelectCamera>(ah_ros_names::CameraSelectService);
}

void AhCameraController::SetBusy(bool busy) {
  if (busy_ == busy) {
    return;
  }
  busy_ = busy;
  emit BusyChanged();
}

void AhCameraController::SetResult(bool success, const QString& message) {
  if (last_success_ != success) {
    last_success_ = success;
    emit LastSuccessChanged();
  }
  if (last_message_ != message) {
    last_message_ = message;
    emit LastMessageChanged();
  }
}

void AhCameraController::FinishOnUiThread(bool success, const QString& message) {
  QMetaObject::invokeMethod(
      this,
      [this, success, message]() {
        SetBusy(false);
        SetResult(success, message);
        emit CommandFinished(success, message);
      },
      Qt::QueuedConnection);
}

void AhCameraController::UpdateSelectedListIndexFromCurrent() {
  int idx = -1;
  for (int i = 0; i < device_paths_.size(); ++i) {
    if (device_paths_[i] == current_device_path_ ||
        (current_device_id_ >= 0 && device_ids_[i] == current_device_id_) ||
        (current_video_source_ == QStringLiteral("synthetic") && device_ids_[i] < 0)) {
      idx = i;
      break;
    }
  }
  if (selected_list_index_ != idx) {
    selected_list_index_ = idx;
  }
}

void AhCameraController::ApplyListResponse(const ah_msgs::srv::ListCameras::Response& response) {
  device_labels_.clear();
  device_ids_.clear();
  device_paths_.clear();
  device_backends_.clear();

  const int n = static_cast<int>(response.ids.size());
  for (int i = 0; i < n; ++i) {
    const int id = response.ids[static_cast<size_t>(i)];
    const QString path = i < static_cast<int>(response.paths.size())
                             ? QString::fromStdString(response.paths[static_cast<size_t>(i)])
                             : QString();
    const QString name = i < static_cast<int>(response.names.size())
                             ? QString::fromStdString(response.names[static_cast<size_t>(i)])
                             : QStringLiteral("Camera %1").arg(id);
    const QString backend =
        i < static_cast<int>(response.backends.size())
            ? QString::fromStdString(response.backends[static_cast<size_t>(i)])
            : QString();

    device_ids_.push_back(id);
    device_paths_.push_back(path);
    device_backends_.push_back(backend);
    // Label from core only — never invent hard-coded indices in UI logic.
    device_labels_.push_back(QStringLiteral("%1  [%2]").arg(name, path));
  }

  current_video_source_ = QString::fromStdString(response.video_source);
  current_device_id_ = response.selected_id;
  current_device_path_ = QString::fromStdString(response.selected_path);
  current_backend_ = QString::fromStdString(response.selected_backend);
  UpdateSelectedListIndexFromCurrent();

  emit DevicesChanged();
  emit SelectionChanged();
}

void AhCameraController::ApplySelectResponse(const ah_msgs::srv::SelectCamera::Response& response) {
  current_video_source_ = QString::fromStdString(response.video_source);
  current_device_id_ = response.device_id;
  current_device_path_ = QString::fromStdString(response.device_path);
  current_backend_ = QString::fromStdString(response.backend);
  UpdateSelectedListIndexFromCurrent();
  emit SelectionChanged();
}

void AhCameraController::RefreshDevices(bool refresh) {
  if (busy_) {
    return;
  }
  if (!list_client_->service_is_ready()) {
    SetResult(false, QStringLiteral("Service ah/camera/list not available (is ah_core running?)"));
    emit CommandFinished(false, last_message_);
    return;
  }

  SetBusy(true);
  auto request = std::make_shared<ah_msgs::srv::ListCameras::Request>();
  request->refresh = refresh;

  list_client_->async_send_request(
      request, [this](rclcpp::Client<ah_msgs::srv::ListCameras>::SharedFuture future) {
        try {
          const auto response = future.get();
          const bool ok = response->success;
          const QString msg = QString::fromStdString(response->message);
          QMetaObject::invokeMethod(
              this,
              [this, ok, msg, response]() {
                if (ok) {
                  ApplyListResponse(*response);
                }
                SetBusy(false);
                SetResult(ok, msg);
                emit CommandFinished(ok, msg);
              },
              Qt::QueuedConnection);
        } catch (const std::exception& ex) {
          FinishOnUiThread(false, QStringLiteral("List failed: %1").arg(ex.what()));
        }
      });
}

void AhCameraController::SelectDeviceAt(int list_index) {
  if (busy_) {
    return;
  }
  if (list_index < 0 || list_index >= device_ids_.size()) {
    SetResult(false, QStringLiteral("Invalid camera list index (refresh first)"));
    emit CommandFinished(false, last_message_);
    return;
  }
  if (!select_client_->service_is_ready()) {
    SetResult(false, QStringLiteral("Service ah/camera/select not available (is ah_core running?)"));
    emit CommandFinished(false, last_message_);
    return;
  }

  SetBusy(true);
  auto request = std::make_shared<ah_msgs::srv::SelectCamera::Request>();
  const int id = device_ids_[list_index];
  const QString path = device_paths_[list_index];
  const QString backend = list_index < device_backends_.size() ? device_backends_[list_index]
                                                               : QString();

  if (id < 0 || path == QStringLiteral("synthetic")) {
    request->video_source = "synthetic";
    request->device_id = -1;
    request->device_path = "synthetic";
    request->backend = "";
  } else {
    request->video_source = "camera";
    request->device_id = id;
    request->device_path = path.toStdString();
    request->backend = backend == QStringLiteral("none") ? "" : backend.toStdString();
  }

  select_client_->async_send_request(
      request, [this](rclcpp::Client<ah_msgs::srv::SelectCamera>::SharedFuture future) {
        try {
          const auto response = future.get();
          const bool ok = response->success;
          const QString msg = QString::fromStdString(response->message);
          QMetaObject::invokeMethod(
              this,
              [this, ok, msg, response]() {
                if (ok) {
                  ApplySelectResponse(*response);
                }
                SetBusy(false);
                SetResult(ok, msg);
                emit CommandFinished(ok, msg);
              },
              Qt::QueuedConnection);
        } catch (const std::exception& ex) {
          FinishOnUiThread(false, QStringLiteral("Select failed: %1").arg(ex.what()));
        }
      });
}
