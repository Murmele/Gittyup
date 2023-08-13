#ifndef TRACE_H
#define TRACE_H

#include <chrono>
#include <QString>
#include "Debug.h"

#define DEBUG_TRACING 1

#if DEBUG_TRACING == 1
#define DebugTracing(x)                                                               \
do {                                                                         \
    if (Debug::isLogging())                                                    \
      qDebug() << Q_FUNC_INFO << QStringLiteral(": ") << x;                                                           \
} while (false)


// copied from labplot project
// Thank you Alex ;)
class PerfTracer {
public:
  explicit PerfTracer(QString m): start(std::chrono::high_resolution_clock::now()), msg(m) {
    init();
  }
  PerfTracer(): start(std::chrono::high_resolution_clock::now()) {
    init();
  }
  void init() {
    if (Debug::isLogging())
      qDebug() << createTabs() << "<p>BEGIN " << msg;
    depth ++;
  }
  ~PerfTracer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto diff =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
    depth --;
    if (Debug::isLogging())
      qDebug() << createTabs() << "END " << msg << QStringLiteral(": ") << diff << QStringLiteral(" ms") << "\n\t" << QStringLiteral("</p>");
  }

private:
  std::chrono::high_resolution_clock::time_point start;
  QString msg;
};

#else
#define Debug(x)
#endif

#define PERFTRACE_ENABLED

#ifdef PERFTRACE_ENABLED
#define PERFTRACE(msg) PerfTracer tracer(qPrintable(Q_FUNC_INFO) + QStringLiteral(": ") + msg)
#else
#define PERFTRACE(msg)
#endif

#define PERFTRACE_DETAILVIEW 0

#endif // TRACE_H
