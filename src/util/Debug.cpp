#include <QDebug>
#include <QSettings>

namespace {
const QString kLogKey = "debug/log";
bool readSettings = false;
bool logging = false;
} // namespace

namespace Debug {

void setLogging(bool enable) {
  logging = enable;
  QSettings().setValue(kLogKey, enable);
}

bool isLogging() {
  if (!readSettings) {
    readSettings = true;
    logging = QSettings().value(kLogKey).toBool();
  }
  return logging;
}
}; // namespace Debug

int depth = 0;
QString createTabs() {
  QString s;
  for (int i=0; i < depth; i++)
    s += "\t";
  return s;
}
