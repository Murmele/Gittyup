#ifndef RESULTWRITER_H
#define RESULTWRITER_H

#include "../WorkerQueue.h"
#include "index/Index.h"

#include <QThread>

class QFile;
class Index;

/// @brief Writes results to the index files
/// @note Only instantiate one instance of this class!
class ResultWriter : public QThread {
public:
  ResultWriter(QFile *out, Index &index,
               WorkerQueue<Index::PostingMap> &results, bool notify)
      : mOut(out), mNotify(notify), mIndex(index), mResults(results) {}

private:
  QFile *mOut;
  bool mNotify;
  Index &mIndex;
  WorkerQueue<Index::PostingMap> &mResults;

  void run() override;
};
#endif // RESULTWRITER_H
