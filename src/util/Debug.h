#define DEBUG_REFRESH 1

#if DEBUG_REFRESH == 1

#define DebugRefresh(x)                                                        \
  do {                                                                         \
    qDebug() << Q_FUNC_INFO << QStringLiteral(": ") << x;                      \
  } while (false)

#else

#define DebugRefresh(x)

#endif
