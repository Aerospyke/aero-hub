#include "ah_track_controller.h"

#include "ah_ros_names.h"

#include <QMetaObject>
#include <QtGlobal>

#include <chrono>
#include <utility>

using namespace std::chrono_literals;

namespace {

bool IsTrackingBoundingBoxNormalized(float x, float y, float width, float height) {
  return x >= 0.0f && y >= 0.0f && width > 0.0f && height > 0.0f && x <= 1.0f && y <= 1.0f &&
         width <= 1.0f && height <= 1.0f && (x + width) <= 1.0001f && (y + height) <= 1.0001f;
}

}  // namespace

AhTrackController::AhTrackController(rclcpp::Node::SharedPtr node, QObject* parent)
    : QObject(parent), node_(std::move(node)) {
  // Relative names → same node namespace as status/video (multi-vehicle ready).
  start_client_ =
      node_->create_client<ah_msgs::srv::StartTracking>(ah_ros_names::TrackingStartService);
  stop_client_ = node_->create_client<std_srvs::srv::Trigger>(ah_ros_names::TrackingStopService);
  cancel_client_ = node_->create_client<std_srvs::srv::Trigger>(ah_ros_names::TrackingCancelService);
  smart_toggle_client_ =
      node_->create_client<std_srvs::srv::SetBool>(ah_ros_names::SmartToggleService);
  smart_click_client_ =
      node_->create_client<ah_msgs::srv::SmartClick>(ah_ros_names::SmartClickService);
}

void AhTrackController::SetTrackingBoundingBoxX(float value) {
  if (qFuzzyCompare(tracking_bounding_box_x_ + 1.0f, value + 1.0f)) {
    return;
  }
  tracking_bounding_box_x_ = value;
  emit trackingBoundingBoxChanged();
}

void AhTrackController::SetTrackingBoundingBoxY(float value) {
  if (qFuzzyCompare(tracking_bounding_box_y_ + 1.0f, value + 1.0f)) {
    return;
  }
  tracking_bounding_box_y_ = value;
  emit trackingBoundingBoxChanged();
}

void AhTrackController::SetTrackingBoundingBoxWidth(float value) {
  if (qFuzzyCompare(tracking_bounding_box_width_ + 1.0f, value + 1.0f)) {
    return;
  }
  tracking_bounding_box_width_ = value;
  emit trackingBoundingBoxChanged();
}

void AhTrackController::SetTrackingBoundingBoxHeight(float value) {
  if (qFuzzyCompare(tracking_bounding_box_height_ + 1.0f, value + 1.0f)) {
    return;
  }
  tracking_bounding_box_height_ = value;
  emit trackingBoundingBoxChanged();
}

void AhTrackController::ResetTrackingBoundingBox() {
  tracking_bounding_box_x_ = 0.35f;
  tracking_bounding_box_y_ = 0.35f;
  tracking_bounding_box_width_ = 0.30f;
  tracking_bounding_box_height_ = 0.30f;
  emit trackingBoundingBoxChanged();
}

void AhTrackController::SetBusy(bool busy) {
  if (busy_ == busy) {
    return;
  }
  busy_ = busy;
  emit BusyChanged();
}

void AhTrackController::SetResult(bool success, const QString& message) {
  if (last_success_ != success) {
    last_success_ = success;
    emit LastSuccessChanged();
  }
  if (last_message_ != message) {
    last_message_ = message;
    emit LastMessageChanged();
  }
}

void AhTrackController::FinishOnUiThread(bool success, const QString& message) {
  QMetaObject::invokeMethod(
      this,
      [this, success, message]() {
        SetBusy(false);
        SetResult(success, message);
        emit CommandFinished(success, message);
      },
      Qt::QueuedConnection);
}

void AhTrackController::StartTracking() {
  if (busy_) {
    return;
  }
  if (!IsTrackingBoundingBoxNormalized(tracking_bounding_box_x_, tracking_bounding_box_y_,
                                       tracking_bounding_box_width_, tracking_bounding_box_height_)) {
    SetResult(false, QStringLiteral(
                         "Tracking bounding box must be normalized in [0,1] with positive size"));
    emit CommandFinished(false, last_message_);
    return;
  }
  if (!start_client_->service_is_ready()) {
    SetResult(false, QStringLiteral("Service /ah/tracking/start not available (is ah_core running?)"));
    emit CommandFinished(false, last_message_);
    return;
  }

  SetBusy(true);
  auto request = std::make_shared<ah_msgs::srv::StartTracking::Request>();
  request->x = tracking_bounding_box_x_;
  request->y = tracking_bounding_box_y_;
  request->width = tracking_bounding_box_width_;
  request->height = tracking_bounding_box_height_;

  start_client_->async_send_request(
      request, [this](rclcpp::Client<ah_msgs::srv::StartTracking>::SharedFuture future) {
        try {
          const auto response = future.get();
          FinishOnUiThread(response->success, QString::fromStdString(response->message));
        } catch (const std::exception& ex) {
          FinishOnUiThread(false, QStringLiteral("Start failed: %1").arg(ex.what()));
        }
      });
}

void AhTrackController::StopTracking() {
  if (busy_) {
    return;
  }
  if (!stop_client_->service_is_ready()) {
    SetResult(false, QStringLiteral("Service /ah/tracking/stop not available (is ah_core running?)"));
    emit CommandFinished(false, last_message_);
    return;
  }

  SetBusy(true);
  auto request = std::make_shared<std_srvs::srv::Trigger::Request>();
  stop_client_->async_send_request(
      request, [this](rclcpp::Client<std_srvs::srv::Trigger>::SharedFuture future) {
        try {
          const auto response = future.get();
          FinishOnUiThread(response->success, QString::fromStdString(response->message));
        } catch (const std::exception& ex) {
          FinishOnUiThread(false, QStringLiteral("Stop failed: %1").arg(ex.what()));
        }
      });
}

void AhTrackController::CancelTracking() {
  if (busy_) {
    return;
  }
  if (!cancel_client_->service_is_ready()) {
    SetResult(false, QStringLiteral("Service /ah/tracking/cancel not available (is ah_core running?)"));
    emit CommandFinished(false, last_message_);
    return;
  }

  SetBusy(true);
  auto request = std::make_shared<std_srvs::srv::Trigger::Request>();
  cancel_client_->async_send_request(
      request, [this](rclcpp::Client<std_srvs::srv::Trigger>::SharedFuture future) {
        try {
          const auto response = future.get();
          FinishOnUiThread(response->success, QString::fromStdString(response->message));
        } catch (const std::exception& ex) {
          FinishOnUiThread(false, QStringLiteral("Cancel failed: %1").arg(ex.what()));
        }
      });
}

void AhTrackController::SetSmartMode(bool enabled) {
  if (busy_) {
    return;
  }
  if (!smart_toggle_client_->service_is_ready()) {
    SetResult(false, QStringLiteral("Service ah/smart/toggle not available (is ah_core running?)"));
    emit CommandFinished(false, last_message_);
    return;
  }

  SetBusy(true);
  auto request = std::make_shared<std_srvs::srv::SetBool::Request>();
  request->data = enabled;
  smart_toggle_client_->async_send_request(
      request, [this](rclcpp::Client<std_srvs::srv::SetBool>::SharedFuture future) {
        try {
          const auto response = future.get();
          FinishOnUiThread(response->success, QString::fromStdString(response->message));
        } catch (const std::exception& ex) {
          FinishOnUiThread(false, QStringLiteral("Smart toggle failed: %1").arg(ex.what()));
        }
      });
}

void AhTrackController::SmartClick(float x, float y) {
  if (busy_) {
    return;
  }
  if (x < 0.f || x > 1.f || y < 0.f || y > 1.f) {
    SetResult(false, QStringLiteral("Smart click must be normalized in [0,1]"));
    emit CommandFinished(false, last_message_);
    return;
  }
  if (!smart_click_client_->service_is_ready()) {
    SetResult(false, QStringLiteral("Service ah/smart/click not available (is ah_core running?)"));
    emit CommandFinished(false, last_message_);
    return;
  }

  SetBusy(true);
  auto request = std::make_shared<ah_msgs::srv::SmartClick::Request>();
  request->x = x;
  request->y = y;
  smart_click_client_->async_send_request(
      request, [this](rclcpp::Client<ah_msgs::srv::SmartClick>::SharedFuture future) {
        try {
          const auto response = future.get();
          const bool ok = response->success;
          const QString msg = QString::fromStdString(response->message);
          const float lx = response->lock_x;
          const float ly = response->lock_y;
          const float lw = response->lock_width;
          const float lh = response->lock_height;
          QMetaObject::invokeMethod(
              this,
              [this, ok, msg, lx, ly, lw, lh]() {
                if (ok && lw > 0.f && lh > 0.f) {
                  tracking_bounding_box_x_ = lx;
                  tracking_bounding_box_y_ = ly;
                  tracking_bounding_box_width_ = lw;
                  tracking_bounding_box_height_ = lh;
                  emit trackingBoundingBoxChanged();
                }
                SetBusy(false);
                SetResult(ok, msg);
                emit CommandFinished(ok, msg);
              },
              Qt::QueuedConnection);
        } catch (const std::exception& ex) {
          FinishOnUiThread(false, QStringLiteral("Smart click failed: %1").arg(ex.what()));
        }
      });
}
