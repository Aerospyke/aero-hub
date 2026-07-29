#include "ah_video_feed.h"

#include <QMutexLocker>
#include <QtGlobal>

AhVideoFeed::AhVideoFeed(QObject* parent) : QObject(parent) {}

QImage AhVideoFeed::CopyFrame() const {
  QMutexLocker lock(&mutex_);
  return frame_;
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

  if (requested_size.isValid() && !requested_size.isEmpty() &&
      requested_size != img.size()) {
    return img.scaled(requested_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }
  return img;
}
