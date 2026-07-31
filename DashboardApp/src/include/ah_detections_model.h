#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVector>

/// Latest YOLO detections from `/ah/detections` (JSON) for QML overlay (Task_34).
/// ApplyJson is safe via QueuedConnection from the ROS thread.
class AhDetectionsModel final : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY CountChanged)
  Q_PROPERTY(QString profile READ Profile NOTIFY MetaChanged)
  Q_PROPERTY(bool live READ Live NOTIFY LiveChanged)
  Q_PROPERTY(QString summary READ Summary NOTIFY CountChanged)

 public:
  enum Roles {
    XRole = Qt::UserRole + 1,
    YRole,
    WRole,
    HRole,
    LabelRole,
    ConfidenceRole,
  };

  /// No detection message for this long → clear boxes (ah_yolo may publish ~few Hz).
  static constexpr int DetectionsStaleTimeoutMs = 1500;

  explicit AhDetectionsModel(QObject* parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] QString Profile() const { return profile_; }
  [[nodiscard]] bool Live() const { return live_; }
  [[nodiscard]] QString Summary() const;

 public slots:
  /// Parse ah_yolo JSON payload (std_msgs/String data).
  void ApplyJson(const QString& json);
  void Clear();

 signals:
  void CountChanged();
  void MetaChanged();
  void LiveChanged();

 private slots:
  void CheckStale();

 private:
  struct Det {
    float x = 0.f;
    float y = 0.f;
    float w = 0.f;
    float h = 0.f;
    QString label;
    float confidence = 0.f;
  };

  void SetLive(bool live);

  QVector<Det> items_;
  QString profile_;
  QTimer stale_watchdog_;
  qint64 last_msg_ms_ = 0;
  bool live_ = false;
};
