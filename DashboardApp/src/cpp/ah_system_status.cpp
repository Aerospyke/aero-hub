#include "ah_system_status.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

AhSystemStatus::AhSystemStatus(QObject* parent) : QObject(parent) {}

void AhSystemStatus::ApplyJson(const QString& json) {
  const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
  if (!doc.isObject()) {
    qWarning("AhSystemStatus: ignoring non-object JSON status payload");
    return;
  }

  const QJsonObject obj = doc.object();

  if (!connected_) {
    connected_ = true;
    emit ConnectedChanged();
  }

  if (raw_json_ != json) {
    raw_json_ = json;
    emit RawJsonChanged();
  }

  const bool smart = obj.value(QStringLiteral("smart_mode_active")).toBool(smart_mode_active_);
  if (smart != smart_mode_active_) {
    smart_mode_active_ = smart;
    emit SmartModeActiveChanged();
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
}
