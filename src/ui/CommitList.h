//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef COMMITLIST_H
#define COMMITLIST_H

#include "git/Reference.h"
#include <QListView>
#include <QTimer>

class Index;

namespace git {
class Commit;
class Diff;
} // namespace git

class CommitList : public QListView {
  Q_OBJECT

public:
  enum Role { DiffRole = Qt::UserRole, CommitRole, GraphRole, GraphColorRole };
  enum class RefsFilter {
    AllRefs,
    SelectedRef,
    SelectedRefIgnoreMerge,
  };

  CommitList(Index *index, QWidget *parent = nullptr);

  // Get the status diff item.
  git::Diff status() const;

  // Get the current selection.
  QString selectedRange() const;
  git::Diff selectedDiff() const;
  QList<git::Commit> selectedCommits() const;

  // Cancel background status diff.
  void cancelStatus();

  void setReference(const git::Reference &ref);
  void setFilter(const QString &filter);
  void setPathspec(const QString &pathspec, bool index = false);
  void setCommits(const QList<git::Commit> &commits);

  void selectReference(const git::Reference &ref);
  void resetSelection(bool spontaneous = false);
  void selectFirstCommit(bool spontaneous = false);
  void selectCommitRelative(int offset);
  bool selectRange(const QString &range, const QString &file = QString(),
                   bool spontaneous = false);
  void suppressResetWalker(bool suppress);
  bool isResetWalkerSuppressed();

  void resetSettings();
  void resetReference(const git::Reference &ref);

  void setModel(QAbstractItemModel *model) override;

  // Whether a status check and/or walker/row rebuild is currently in
  // flight. See the loadingChanged() signal for a way to wait on this
  // instead of polling it.
  bool isLoading() const { return mLoading; }

signals:
  void statusChanged(bool dirty);
  void diffSelected(const git::Diff diff, const QString &file = QString(),
                    bool spontaneous = false);

  // Emitted just before a (potentially slow) diff is being computation. This
  // can be used to clear GUI and enable loading indicators whilst waiting
  void diffLoading();

  // Emitted whenever isLoading() changes.
  void loadingChanged(bool loading);

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *) override;
  void paintEvent(QPaintEvent *event) override;

private:
  void storeSelection();
  void restoreSelection();
  void updateModel();
  void setLoading(bool loading);

  QModelIndexList sortedIndexes() const;

  QModelIndex findCommit(const git::Commit &commit);
  void selectIndexes(const QItemSelection &selection,
                     const QString &file = QString(), bool spontaneous = false);

  void notifySelectionChanged();
  void dispatchSelectedDiff(const QString &file, bool spontaneous);

  bool isDecoration(const QModelIndex &index, const QPoint &pos);
  bool isStar(const QModelIndex &index, const QPoint &pos);

  QString mFile;
  QModelIndex mStar;
  QModelIndex mCancel;
  bool mSpontaneous = true;

  Index *mIndex;
  QString mFilter;

  QAbstractListModel *mList;
  QAbstractListModel *mModel;

  bool mRestoreSelection{true};

  QString mSelectedRange;

  // Whether the current selection is just the automatic fallback rather
  // than a deliberate user pick
  bool mSelectionIsDefault{false};

  // Whether the loading indicator should be shown
  bool mLoading{false};
  float mLoadingFadein = 0;
  int mProgress{0};
  QTimer mTimer;

  // Incremented on every selection-driven diff request. This is a hack used to
  // discard diffs that arrive before the last one
  // (Yes, we should have a proper cancel pathway here)
  int mDiffRequest = 0;
};

#endif
