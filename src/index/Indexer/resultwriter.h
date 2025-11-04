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
  ResultWriter(Index &index,
               WorkerQueue<Index::PostingMap> &results, bool notify)
      : mNotify(notify), mIndex(index), mResults(results) {}

private:
  bool mNotify;
  Index &mIndex;
  WorkerQueue<Index::PostingMap> &mResults;

  void run() override;
};
#endif // RESULTWRITER_H
