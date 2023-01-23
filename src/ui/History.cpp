//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "History.h"
#include "RepoView.h"
#include <QMenu>

namespace {

void addAction(QMenu *menu, const git::Repository &repo, const Location &loc,
               int index) {
  QFontMetrics fm = menu->fontMetrics();
  QString text = fm.elidedText(loc.toString(repo), Qt::ElideRight, 256);
  menu->addAction(text)->setData(index);
}

} // namespace

History::History(RepoView *view) : QObject(view), mView(view), mIndex(0) {}

bool History::hasNext() const { return (mIndex < mLocs.size()); }

bool History::hasPrev() const { return (mIndex > 0); }

void History::next() {
  Q_ASSERT(hasNext());

  ++mIndex;
  mView->setLocation(mIndex < mLocs.size() ? mLocs.at(mIndex) : mPendingLoc);

  emit changed();
}

void History::prev() {
  Q_ASSERT(hasPrev());

  mView->setLocation(mLocs.at(--mIndex));

  emit changed();
}

void History::setIndex(int index) {
  mIndex = index;
  mView->setLocation(index < mLocs.size() ? mLocs.at(index) : mPendingLoc);

  emit changed();
}

void History::update(const Location &location, bool spontaneous) {
  // Skip invalid locations where the pending entry already represents
  // uncommitted changes. This handles the case where the user refreshes
  // with the uncommitted item already selected.
  bool uncommitted = (!location.isValid() && mPendingLoc.isUncommitted());
  if (!spontaneous || !mPendingLoc.isValid() || uncommitted) {
    if (!mPendingLoc.isValid() && location.isValid())
      mPendingLoc = location;
    return;
  }

  // Truncate at current index.
  while (mLocs.size() > mIndex)
    mPendingLoc = mLocs.takeLast();

  mLocs.append(mPendingLoc);
  mIndex = mLocs.size();
  mPendingLoc = location;

  emit changed();
}

void History::updatePrevMenu(QMenu *menu) {
  menu->clear();
  git::Repository repo = mView->repo();
  for (int i = mIndex - 1; i >= 0; --i)
    addAction(menu, repo, mLocs.at(i), i);
}

void History::updateNextMenu(QMenu *menu) {
  menu->clear();
  git::Repository repo = mView->repo();
  for (int i = mIndex + 1; i < mLocs.size(); ++i)
    addAction(menu, repo, mLocs.at(i), i);

  if (mPendingLoc.isValid())
    addAction(menu, repo, mPendingLoc, mLocs.size());
}

void History::clean() {
  // Can't clean while navigating back through history.
  if (mIndex < mLocs.size())
    return;

  auto predicate = [](const Location &loc) { return loc.isUncommitted(); };
  auto it = std::remove_if(mLocs.begin(), mLocs.end(), predicate);
  if (it != mLocs.end()) {
    // Remove duplicated entries.
    it = std::unique(mLocs.begin(), it);

    mLocs.erase(it, mLocs.end());
    mIndex = mLocs.size();

    emit changed();
  }
}
