#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>

#define DEBUG 1
#define DEBUG_REFRESH 1

#if DEBUG == 0

#define Debug(x)                                                               \
  do {                                                                         \
    qDebug() << x;                                                             \
  } while (false)

#else

#define Debug(x)

#endif

#if DEBUG_REFRESH == 1

#define DebugRefresh(x)                                                        \
  do {                                                                         \
    qDebug() << Q_FUNC_INFO << QStringLiteral(": ") << x;                      \
  } while (false)

#else

#define DebugRefresh(x)

#endif

#endif // DEBUG_H
