#include "ah_detections_model.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>
#include <utility>

AhDetectionsModel::AhDetectionsModel(QObject* parent) : QAbstractListModel(parent) {
  stale_watchdog_.setInterval(250);
  connect(&stale_watchdog_, &QTimer::timeout, this, &AhDetectionsModel::CheckStale);
  stale_watchdog_.start();
}

int AhDetectionsModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return items_.size();
}

QVariant AhDetectionsModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const Det& d = items_.at(index.row());
  switch (role) {
    case XRole:
      return d.x;
    case YRole:
      return d.y;
    case WRole:
      return d.w;
    case HRole:
      return d.h;
    case LabelRole:
      return d.label;
    case ConfidenceRole:
      return d.confidence;
    case TrackIdRole:
      return d.track_id;
    default:
      return {};
  }
}

QHash<int, QByteArray> AhDetectionsModel::roleNames() const {
  return {
      {XRole, "nx"},
      {YRole, "ny"},
      {WRole, "nw"},
      {HRole, "nh"},
      {LabelRole, "label"},
      {ConfidenceRole, "confidence"},
      {TrackIdRole, "trackId"},
  };
}

QString AhDetectionsModel::Summary() const {
  if (!live_ || items_.isEmpty()) {
    if (!live_) {
      return QStringLiteral("no detections stream");
    }
    return QStringLiteral("0 boxes");
  }
  return QStringLiteral("%1 box%2 · %3")
      .arg(items_.size())
      .arg(items_.size() == 1 ? QString() : QStringLiteral("es"))
      .arg(profile_.isEmpty() ? QStringLiteral("?") : profile_);
}

void AhDetectionsModel::SetLive(bool live) {
  if (live_ == live) {
    return;
  }
  live_ = live;
  emit liveChanged();
  emit countChanged();
}

void AhDetectionsModel::Clear() {
  if (items_.isEmpty() && profile_.isEmpty()) {
    SetLive(false);
    return;
  }
  beginResetModel();
  items_.clear();
  endResetModel();
  profile_.clear();
  emit metaChanged();
  emit countChanged();
  SetLive(false);
}

void AhDetectionsModel::CheckStale() {
  if (!live_) {
    return;
  }
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (last_msg_ms_ > 0 && (now - last_msg_ms_) > DetectionsStaleTimeoutMs) {
    Clear();
  }
}

void AhDetectionsModel::ApplyJson(const QString& json) {
  const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
  if (!doc.isObject()) {
    return;
  }
  const QJsonObject root = doc.object();
  const QString profile = root.value(QStringLiteral("profile")).toString();
  const QJsonArray arr = root.value(QStringLiteral("detections")).toArray();

  QVector<Det> next;
  next.reserve(arr.size());
  for (const QJsonValue& v : arr) {
    if (!v.isObject()) {
      continue;
    }
    const QJsonObject o = v.toObject();
    const QJsonObject bb = o.value(QStringLiteral("bbox_normalized")).toObject();
    Det d;
    d.x = static_cast<float>(bb.value(QStringLiteral("x")).toDouble(0.0));
    d.y = static_cast<float>(bb.value(QStringLiteral("y")).toDouble(0.0));
    d.w = static_cast<float>(bb.value(QStringLiteral("w")).toDouble(0.0));
    d.h = static_cast<float>(bb.value(QStringLiteral("h")).toDouble(0.0));
    // Clamp to unit square
    d.x = qBound(0.f, d.x, 1.f);
    d.y = qBound(0.f, d.y, 1.f);
    d.w = qBound(0.f, d.w, 1.f - d.x);
    d.h = qBound(0.f, d.h, 1.f - d.y);
    if (d.w < 0.001f || d.h < 0.001f) {
      continue;
    }
    d.confidence = static_cast<float>(o.value(QStringLiteral("confidence")).toDouble(0.0));
    d.track_id = o.value(QStringLiteral("track_id")).toInt(-1);
    d.label = o.value(QStringLiteral("class_name")).toString();
    if (d.label.isEmpty()) {
      d.label = QString::number(o.value(QStringLiteral("class_id")).toInt());
    }
    // Short label for HUD (include track id when present)
    QString conf_s;
    if (d.confidence > 0.f) {
      conf_s = QStringLiteral(" %1%").arg(
          QString::number(static_cast<int>(d.confidence * 100.f + 0.5f)));
    }
    if (d.track_id >= 0) {
      d.label = QStringLiteral("%1#%2%3").arg(d.label).arg(d.track_id).arg(conf_s);
    } else if (!conf_s.isEmpty()) {
      d.label = d.label + conf_s;
    }
    next.push_back(d);
  }

  beginResetModel();
  items_ = std::move(next);
  endResetModel();

  if (profile_ != profile) {
    profile_ = profile;
    emit metaChanged();
  }
  last_msg_ms_ = QDateTime::currentMSecsSinceEpoch();
  SetLive(true);
  emit countChanged();
}
