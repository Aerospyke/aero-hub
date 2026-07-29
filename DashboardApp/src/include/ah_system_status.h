#pragma once

#include <QObject>
#include <QString>

/// Live snapshot of `/ah/system/status` for QML (interface map §3.1).
/// Updated from the ROS executor thread via ApplyJson (queued to the Qt thread).
class AhSystemStatus final : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool connected READ Connected NOTIFY ConnectedChanged)
  Q_PROPERTY(bool smartModeActive READ SmartModeActive NOTIFY SmartModeActiveChanged)
  Q_PROPERTY(bool trackingStarted READ TrackingStarted NOTIFY TrackingStartedChanged)
  Q_PROPERTY(bool segmentationActive READ SegmentationActive NOTIFY SegmentationActiveChanged)
  Q_PROPERTY(bool followingActive READ FollowingActive NOTIFY FollowingActiveChanged)
  Q_PROPERTY(QString videoStatus READ VideoStatus NOTIFY VideoStatusChanged)
  Q_PROPERTY(QString trackerType READ TrackerType NOTIFY TrackerTypeChanged)
  Q_PROPERTY(QString followerMode READ FollowerMode NOTIFY FollowerModeChanged)
  Q_PROPERTY(double stamp READ Stamp NOTIFY StampChanged)
  Q_PROPERTY(QString rawJson READ RawJson NOTIFY RawJsonChanged)

 public:
  explicit AhSystemStatus(QObject* parent = nullptr);

  [[nodiscard]] bool Connected() const { return connected_; }
  [[nodiscard]] bool SmartModeActive() const { return smart_mode_active_; }
  [[nodiscard]] bool TrackingStarted() const { return tracking_started_; }
  [[nodiscard]] bool SegmentationActive() const { return segmentation_active_; }
  [[nodiscard]] bool FollowingActive() const { return following_active_; }
  [[nodiscard]] QString VideoStatus() const { return video_status_; }
  [[nodiscard]] QString TrackerType() const { return tracker_type_; }
  [[nodiscard]] QString FollowerMode() const { return follower_mode_; }
  [[nodiscard]] double Stamp() const { return stamp_; }
  [[nodiscard]] QString RawJson() const { return raw_json_; }

 public slots:
  /// Parse JSON status payload (std_msgs/String data). Safe to call via
  /// QMetaObject::invokeMethod(..., Qt::QueuedConnection) from the ROS thread.
  void ApplyJson(const QString& json);

 signals:
  void ConnectedChanged();
  void SmartModeActiveChanged();
  void TrackingStartedChanged();
  void SegmentationActiveChanged();
  void FollowingActiveChanged();
  void VideoStatusChanged();
  void TrackerTypeChanged();
  void FollowerModeChanged();
  void StampChanged();
  void RawJsonChanged();

 private:
  bool connected_ = false;
  bool smart_mode_active_ = false;
  bool tracking_started_ = false;
  bool segmentation_active_ = false;
  bool following_active_ = false;
  QString video_status_ = QStringLiteral("unknown");
  QString tracker_type_ = QStringLiteral("—");
  QString follower_mode_ = QStringLiteral("—");
  double stamp_ = 0.0;
  QString raw_json_;
};
