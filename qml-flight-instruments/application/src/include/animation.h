#pragma once

#include <QTimer>

#include "primary_flight_data.h"

class Animation : public QObject {
  Q_OBJECT
 public:
  explicit Animation(QObject* parent = nullptr);

  void setPfd(PrimaryFlightData* newPfd);

 public slots:
  void update();
  void init();

 private:
  PrimaryFlightData* mPfd;
  QTimer mTimer;
  double mPlayTime;
  quint64 mPreviousTime;
};