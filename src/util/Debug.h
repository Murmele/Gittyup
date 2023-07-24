#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>

namespace Debug {

void setLogging(bool enable);
bool isLogging();
qint64 runTime();
}; // namespace Debug

#ifdef DEBUG_OUTPUT_GENERAL
#define Debug(x)                                                               \
  do {                                                                         \
    if (Debug::isLogging()) {                                                  \
      qDebug() << QString::number(runTime()) << ": " << x;                     \
	}                                                                          \
  } while (false)
#else
#define Debug(x)
#endif

#ifdef DEBUG_OUTPUT_REFRESH
#define DebugRefresh(x)                                                        \
  do {                                                                         \
    if (Debug::isLogging())                                                    \
      qDebug() << Q_FUNC_INFO << QStringLiteral(": ") << x;                    \
  } while (false)
#else
#define DebugRefresh(x)
#endif
#endif // DEBUG_H
