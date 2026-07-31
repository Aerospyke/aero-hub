#include "ah_system_status.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

AhSystemStatus::AhSystemStatus(QObject* parent) : QObject(parent) {
  link_watchdog_.setInterval(250);
  QObject::connect(&link_watchdog_, &QTimer::timeout, this, &AhSystemStatus::CheckLinkHealth);
  link_watchdog_.start();
}

void AhSystemStatus::SetLinkState(const QString& state) {
  if (link_state_ == state) {
    return;
  }
  link_state_ = state;
  emit LinkStateChanged();

  QString message;
  if (state == QLatin1String("live")) {
    message = QStringLiteral("Connected to ah_core");
  } else if (state == QLatin1String("lost")) {
    message = QStringLiteral(
        "Lost contact with ah_core — dashboard still running; will recover when status returns");
  } else {
    message = QStringLiteral("Waiting for ah_core (/ah/system/status) on this ROS domain…");
  }
  if (connection_message_ != message) {
    connection_message_ = message;
    emit ConnectionMessageChanged();
  }

  const bool connected = (state == QLatin1String("live"));
  if (connected_ != connected) {
    connected_ = connected;
    emit ConnectedChanged();
  }
}

void AhSystemStatus::CheckLinkHealth() {
  if (!ever_connected_) {
    return;
  }
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (now - last_status_ms_ > LinkStaleTimeoutMs) {
    SetLinkState(QStringLiteral("lost"));
  }
}

void AhSystemStatus::ApplyJson(const QString& json) {
  const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
  if (!doc.isObject()) {
    qWarning("AhSystemStatus: ignoring non-object JSON status payload");
    return;
  }

  const QJsonObject obj = doc.object();

  last_status_ms_ = QDateTime::currentMSecsSinceEpoch();
  ever_connected_ = true;
  SetLinkState(QStringLiteral("live"));

  if (raw_json_ != json) {
    raw_json_ = json;
    emit RawJsonChanged();
  }

  const bool ai_tracking = obj.value(QStringLiteral("ai_tracking_active")).toBool(ai_tracking_active_);
  if (ai_tracking != ai_tracking_active_) {
    ai_tracking_active_ = ai_tracking;
    emit aiTrackingActiveChanged();
  }

  const bool tracking = obj.value(QStringLiteral("tracking_started")).toBool(tracking_started_);
  if (tracking != tracking_started_) {
    tracking_started_ = tracking;
    emit TrackingStartedChanged();
  }

  const bool segmentation = obj.value(QStringLiteral("segmentation_active")).toBool(segmentation_active_);
  if (segmentation != segmentation_active_) {
    segmentation_active_ = segmentation;
    emit SegmentationActiveChanged();
  }

  const bool following = obj.value(QStringLiteral("following_active")).toBool(following_active_);
  if (following != following_active_) {
    following_active_ = following;
    emit FollowingActiveChanged();
  }

  const QString video = obj.value(QStringLiteral("video_status")).toString(video_status_);
  if (video != video_status_) {
    video_status_ = video;
    emit VideoStatusChanged();
  }

  const QString tracker = obj.value(QStringLiteral("tracker_type")).toString(tracker_type_);
  if (tracker != tracker_type_) {
    tracker_type_ = tracker;
    emit TrackerTypeChanged();
  }

  const QString follower = obj.value(QStringLiteral("follower_mode")).toString(follower_mode_);
  if (follower != follower_mode_) {
    follower_mode_ = follower;
    emit FollowerModeChanged();
  }

  const double stamp = obj.value(QStringLiteral("stamp")).toDouble(stamp_);
  if (!qFuzzyCompare(stamp + 1.0, stamp_ + 1.0)) {
    stamp_ = stamp;
    emit StampChanged();
  }

  const float bx = static_cast<float>(obj.value(QStringLiteral("tracking_bbox_x")).toDouble(tracking_bbox_x_));
  const float by = static_cast<float>(obj.value(QStringLiteral("tracking_bbox_y")).toDouble(tracking_bbox_y_));
  const float bw = static_cast<float>(obj.value(QStringLiteral("tracking_bbox_w")).toDouble(tracking_bbox_w_));
  const float bh = static_cast<float>(obj.value(QStringLiteral("tracking_bbox_h")).toDouble(tracking_bbox_h_));
  const int tid = obj.value(QStringLiteral("locked_track_id")).toInt(locked_track_id_);
  if (!qFuzzyCompare(bx + 1.f, tracking_bbox_x_ + 1.f) ||
      !qFuzzyCompare(by + 1.f, tracking_bbox_y_ + 1.f) ||
      !qFuzzyCompare(bw + 1.f, tracking_bbox_w_ + 1.f) ||
      !qFuzzyCompare(bh + 1.f, tracking_bbox_h_ + 1.f) ||
      tid != locked_track_id_) {
    tracking_bbox_x_ = bx;
    tracking_bbox_y_ = by;
    tracking_bbox_w_ = bw;
    tracking_bbox_h_ = bh;
    locked_track_id_ = tid;
    emit trackingBboxChanged();
  }
}
