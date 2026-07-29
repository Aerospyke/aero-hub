#pragma once

#include <QByteArray>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QQuickImageProvider>
#include <QTimer>

/// Latest ROS video frame for QML (`image://ahvideo/...`).
/// ApplyJpeg is safe to invoke on the UI thread via QueuedConnection.
/// frameLive goes false if frames stop (ah_core killed) without clearing the last image.
class AhVideoFeed final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool hasFrame READ HasFrame NOTIFY HasFrameChanged)
  Q_PROPERTY(bool frameLive READ FrameLive NOTIFY FrameLiveChanged)
  Q_PROPERTY(int frameId READ FrameId NOTIFY FrameIdChanged)
  Q_PROPERTY(int frameWidth READ FrameWidth NOTIFY FrameSizeChanged)
  Q_PROPERTY(int frameHeight READ FrameHeight NOTIFY FrameSizeChanged)

 public:
  static constexpr int FrameStaleTimeoutMs = 1500;

  explicit AhVideoFeed(QObject* parent = nullptr);

  [[nodiscard]] bool HasFrame() const { return has_frame_; }
  [[nodiscard]] bool FrameLive() const { return frame_live_; }
  [[nodiscard]] int FrameId() const { return frame_id_; }
  [[nodiscard]] int FrameWidth() const { return frame_width_; }
  [[nodiscard]] int FrameHeight() const { return frame_height_; }

  /// Thread-safe copy of the latest decoded frame (for the image provider).
  [[nodiscard]] QImage CopyFrame() const;

 public slots:
  void ApplyJpeg(const QByteArray& jpeg);

 signals:
  void HasFrameChanged();
  void FrameLiveChanged();
  void FrameIdChanged();
  void FrameSizeChanged();

 private slots:
  void CheckFrameHealth();

 private:
  void SetFrameLive(bool live);

  mutable QMutex mutex_;
  QImage frame_;
  QTimer frame_watchdog_;
  qint64 last_frame_ms_ = 0;
  bool has_frame_ = false;
  bool frame_live_ = false;
  int frame_id_ = 0;
  int frame_width_ = 0;
  int frame_height_ = 0;
};

/// Serves frames to QML Image items.
class AhVideoImageProvider final : public QQuickImageProvider {
 public:
  explicit AhVideoImageProvider(AhVideoFeed* feed);

  QImage requestImage(const QString& id, QSize* size, const QSize& requested_size) override;

 private:
  AhVideoFeed* feed_ = nullptr;
};
