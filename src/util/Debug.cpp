#include <QDebug>
#include <QSettings>
#include <QDateTime>

namespace {
const QString kLogKey = "debug/log";
bool readSettings = false;
bool logging = false;
qint64 startupTime{0};
} // namespace

namespace Debug {

void setLogging(bool enable) {
  logging = enable;
  QSettings().setValue(kLogKey, enable);
}

qint64 runTime() {
	return QDateTime::currentMSecsSinceEpoch() - startupTime;
}

bool isLogging() {
  if (!readSettings) {
	startupTime = QDateTime::currentMSecsSinceEpoch();
    readSettings = true;
    logging = QSettings().value(kLogKey).toBool();
  }
  return logging;
}
}; // namespace Debug