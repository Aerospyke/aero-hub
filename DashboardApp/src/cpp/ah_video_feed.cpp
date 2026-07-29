#include "ah_video_feed.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QtGlobal>

AhVideoFeed::AhVideoFeed(QObject* parent) : QObject(parent) {
  frame_watchdog_.setInterval(250);
  QObject::connect(&frame_watchdog_, &QTimer::timeout, this, &AhVideoFeed::CheckFrameHealth);
  frame_watchdog_.start();
}

QImage AhVideoFeed::CopyFrame() const {
  QMutexLocker lock(&mutex_);
  return frame_;
}

void AhVideoFeed::SetFrameLive(bool live) {
  if (frame_live_ == live) {
    return;
  }
  frame_live_ = live;
  emit FrameLiveChanged();
}

void AhVideoFeed::CheckFrameHealth() {
  if (!has_frame_) {
    return;
  }
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (now - last_frame_ms_ > FrameStaleTimeoutMs) {
    SetFrameLive(false);
  }
}

void AhVideoFeed::ApplyJpeg(const QByteArray& jpeg) {
  if (jpeg.isEmpty()) {
    return;
  }

  QImage decoded;
  if (!decoded.loadFromData(reinterpret_cast<const uchar*>(jpeg.constData()), jpeg.size(), "JPG")) {
    qWarning("AhVideoFeed: failed to decode JPEG (%d bytes)", static_cast<int>(jpeg.size()));
    return;
  }

  const int w = decoded.width();
  const int h = decoded.height();
  {
    QMutexLocker lock(&mutex_);
    frame_ = std::move(decoded);
  }

  last_frame_ms_ = QDateTime::currentMSecsSinceEpoch();
  SetFrameLive(true);

  if (!has_frame_) {
    has_frame_ = true;
    emit HasFrameChanged();
  }

  if (w != frame_width_ || h != frame_height_) {
    frame_width_ = w;
    frame_height_ = h;
    emit FrameSizeChanged();
  }

  ++frame_id_;
  emit FrameIdChanged();
}

AhVideoImageProvider::AhVideoImageProvider(AhVideoFeed* feed)
    : QQuickImageProvider(QQuickImageProvider::Image), feed_(feed) {}

QImage AhVideoImageProvider::requestImage(const QString& /*id*/, QSize* size,
                                          const QSize& requested_size) {
  if (feed_ == nullptr) {
    return {};
  }

  QImage img = feed_->CopyFrame();
  if (img.isNull()) {
    return {};
  }

  if (size != nullptr) {
    *size = img.size();
  }

  if (requested_size.isValid() && !requested_size.isEmpty() && requested_size != img.size()) {
    return img.scaled(requested_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }
  return img;
}
