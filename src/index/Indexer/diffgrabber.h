#ifndef DIFFGRABBER_H
#define DIFFGRABBER_H

#include "../WorkerQueue.h"
#include "git/Commit.h"
#include "git/Diff.h"

#include <QRunnable>

/// @brief Runnable grabbing the git diff from libgit2
/// @note This is somewhat expensive and easily hits multi-threading limits in
/// libgit2. It should be run in a 1:1 ratio with Map runnables
class DiffGrabber : public QRunnable {
public:
  DiffGrabber(WorkerQueue<git::Commit> &commits,
              WorkerQueue<QPair<git::Commit, git::Diff>> &diffedCommits)
      : mCommits(commits), mDiffedCommits(diffedCommits) {}

private:
  WorkerQueue<git::Commit> &mCommits;
  WorkerQueue<QPair<git::Commit, git::Diff>> &mDiffedCommits;

  void run() override;
};

#endif // DIFFGRABBER_H
