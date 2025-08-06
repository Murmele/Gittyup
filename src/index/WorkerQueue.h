//
//          Copyright (c) 2025
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
//

#ifndef WORKERQUEUE_H
#define WORKERQUEUE_H

#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QWaitCondition>

template <class T> class WorkerQueue : public QObject {
private:
  const qsizetype mMaxSize;
  QQueue<T> mQueue;
  QMutex mMutex;
  QWaitCondition mNotEmpty;
  QWaitCondition mNotFull;
  QWaitCondition mEmpty;
  volatile bool mStop = false;

public:
  WorkerQueue(QObject *parent = nullptr)
      : QObject(parent), mMaxSize(std::numeric_limits<qsizetype>::max()) {}
  WorkerQueue(qsizetype maxSize, QObject *parent = nullptr)
      : QObject(parent), mMaxSize(maxSize) {}

  void stop() {
    mStop = true;
    mNotEmpty.wakeAll();
    mEmpty.wakeAll();
  }
  void enqueue(T &&item) {
    QMutexLocker lock(&mMutex);
    while (mQueue.size() == mMaxSize && !mStop)
      mNotFull.wait(&mMutex);
    mQueue.enqueue(std::move(item));
    mNotEmpty.wakeOne();
  }

  void awaitEmpty() {
    QMutexLocker lock(&mMutex);
    while (mQueue.empty() == false && !mStop)
      mEmpty.wait(&mMutex);
  }

  std::optional<T> dequeue() {
    QMutexLocker lock(&mMutex);
    while (mQueue.empty() && !mStop)
      mNotEmpty.wait(&mMutex);
    if (mStop)
      return std::nullopt;
    else {
      T item = std::move(mQueue.dequeue());
      mNotFull.wakeOne();
      if (mQueue.size() == 0)
        mEmpty.wakeAll();
      return item;
    }
  }
};

#endif