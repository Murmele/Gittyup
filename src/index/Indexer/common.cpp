#include "common.h"

#include <QTime>
#include <QFile>
#include <QString>

bool canceled = false;

namespace {
    QFile* file = nullptr;
    QMutex mutex = QMutex();
}

void setLogFile(QFile* f) {
    mutex.lock();
    file = f;
    mutex.unlock();
}

void log(const QString &fmt, const git::Id &id) {
    log(fmt.arg(id.toString()));
}

void log(const QString &text) {
    if (!file)
        return;
    mutex.lock();
    // Drop QTextStream to flush the complete text
    {
      QString time = QTime::currentTime().toString(Qt::ISODateWithMs);
      QTextStream(file) << time << " - " << text << Qt::endl;
    }
    mutex.unlock();
}
