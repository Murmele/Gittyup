#ifndef TRACE_H
#define TRACE_H

#include <chrono>
#include <QString>
#include <iostream>

// copied from labplot project
// Thank you Alex ;)
class PerfTracer {
public:
  explicit PerfTracer(QString m) {
    msg = m.toStdString();
    start = std::chrono::high_resolution_clock::now();
  };
  ~PerfTracer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto diff =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
    std::cout << msg << ": " << diff << " ms" << std::endl;
  }

private:
  std::chrono::high_resolution_clock::time_point start;
  std::string msg;
};

#define PERFTRACE_ENABLED

#ifdef PERFTRACE_ENABLED
#define PERFTRACE(msg) PerfTracer tracer(QString(Q_FUNC_INFO) + msg)
#else
#define PERFTRACE(msg)
#endif

#define PERFTRACE_DETAILVIEW 0

#endif // TRACE_H
