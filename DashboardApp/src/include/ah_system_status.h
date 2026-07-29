#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

/// Live snapshot of `/ah/system/status` for QML (interface map §3.1).
/// Updated from the ROS executor thread via ApplyJson (queued to the Qt thread).
/// If status messages stop arriving, linkState becomes "lost" without crashing the app.
class AhSystemStatus final : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool connected READ Connected NOTIFY ConnectedChanged)
  /// "waiting" (never seen core) | "live" | "lost" (had core, timed out)
  Q_PROPERTY(QString linkState READ LinkState NOTIFY LinkStateChanged)
  Q_PROPERTY(QString connectionMessage READ ConnectionMessage NOTIFY ConnectionMessageChanged)
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
  /// No status message for this long → treat link as lost (ah_core publishes ~10 Hz).
  static constexpr int LinkStaleTimeoutMs = 2000;

  explicit AhSystemStatus(QObject* parent = nullptr);

  [[nodiscard]] bool Connected() const { return connected_; }
  [[nodiscard]] QString LinkState() const { return link_state_; }
  [[nodiscard]] QString ConnectionMessage() const { return connection_message_; }
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
  void LinkStateChanged();
  void ConnectionMessageChanged();
  void SmartModeActiveChanged();
  void TrackingStartedChanged();
  void SegmentationActiveChanged();
  void FollowingActiveChanged();
  void VideoStatusChanged();
  void TrackerTypeChanged();
  void FollowerModeChanged();
  void StampChanged();
  void RawJsonChanged();

 private slots:
  void CheckLinkHealth();

 private:
  void SetLinkState(const QString& state);

  QTimer link_watchdog_;
  qint64 last_status_ms_ = 0;
  bool ever_connected_ = false;
  bool connected_ = false;
  QString link_state_ = QStringLiteral("waiting");
  QString connection_message_ =
      QStringLiteral("Waiting for ah_core (/ah/system/status) on this ROS domain…");
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
