#ifndef COMMON_H
#define COMMON_H

#include "git/Id.h"

#include <QMap>
#include <QHash>

class QFile;
class QString;

// global cancel flag
extern bool canceled;

struct Intermediate {
  using TermMap = QHash<QByteArray, QVector<quint32>>;
  using FieldMap = QMap<quint8, TermMap>;

  git::Id id;
  FieldMap fields;
};

void log(QFile *out, const QString &text);
void log(QFile *out, const QString &fmt, const git::Id &id);

#endif // COMMON_H
