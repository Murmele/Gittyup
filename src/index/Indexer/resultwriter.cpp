#include "resultwriter.h"
#include "common.h"

void ResultWriter::run() {
  std::optional<Index::PostingMap> resultOpt = mResults.dequeue();
  while (!canceled && resultOpt.has_value()) {
    auto result = resultOpt.value();
    // Write to disk.
    log("start write");
    if (mIndex.write(result) && mNotify)
      QTextStream(stdout) << "write" << Qt::endl;
    log("end write");

    // Grab next value
    resultOpt = mResults.dequeue();
  }
}
