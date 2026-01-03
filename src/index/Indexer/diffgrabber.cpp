#include "diffgrabber.h"
#include "common.h"

void DiffGrabber::run() {
  std::optional<git::Commit> commitOpt = mCommits.dequeue();
  while (!canceled && commitOpt.has_value()) {
    auto commit = commitOpt.value();
    git::Diff diff = commit.diff(git::Commit(), true);
    mDiffedCommits.enqueue(QPair<git::Commit, git::Diff>(commit, diff));
    commitOpt = mCommits.dequeue();
  }
}
