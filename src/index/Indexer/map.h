#ifndef MAP_H
#define MAP_H

#include <QRunnable>
#include "git/Repository.h"
#include "../WorkerQueue.h"
#include "common.h"

class LexerPool;

/// @brief Runnable mapping git commits and diffs into Intermediate objects.
/// This includes running lexing on the diff and commit
class Map : public QRunnable {
public:
  typedef Intermediate result_type;

  Map(const git::Repository &repo, LexerPool &lexers, QFile *out,
      WorkerQueue<QPair<git::Commit, git::Diff>> &inQueue,
      WorkerQueue<Intermediate> &outQueue);

  void run() override;

private:
  LexerPool &mLexers;
  QFile *mOut;
  WorkerQueue<QPair<git::Commit, git::Diff>> &mInQueue;
  WorkerQueue<Intermediate> &mOutQueue;

  int mContextLines = 3;
  quint32 mTermLimit = 1000000;
};

#endif // MAP_H
