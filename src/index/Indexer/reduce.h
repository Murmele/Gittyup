#ifndef REDUCE_H
#define REDUCE_H

#include "../WorkerQueue.h"
#include "common.h"
#include "index/Index.h"

#include <QThread>

class QFile;
class IdStorage;
class Index;

/// @brief This will convert Intermediate into a posting map
/// @note This is extracted from explicitly single-thread code, thus only one
/// thread should be started!
class Reduce : public QThread {
public:
  Reduce(IdStorage &ids, WorkerQueue<Intermediate> &queue,
         WorkerQueue<Index::PostingMap> &results, Index &index,
         QObject *parent = nullptr)
      : QThread(parent), mIds(ids), mQueue(queue), mResults(results),
        mIndex(index) {}

private:
  void run() override;

private:
  IdStorage &mIds;
  WorkerQueue<Intermediate> &mQueue;
  WorkerQueue<Index::PostingMap> &mResults;
  Index &mIndex;
  quint16 mProcessedElemenets = 0;
};

#endif // REDUCE_H
