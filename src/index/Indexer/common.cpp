#include "common.h"

#include <QTime>
#include <QFile>
#include <QString>

bool canceled = false;

void log(QFile *out, const QString &text) {
  if (!out)
    return;

  QString time = QTime::currentTime().toString(Qt::ISODateWithMs);
  QTextStream(out) << time << " - " << text << Qt::endl;
}

void log(QFile *out, const QString &fmt, const git::Id &id) {
  if (!out)
    return;

  log(out, fmt.arg(id.toString()));
}
