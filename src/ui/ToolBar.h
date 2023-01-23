//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QAction>
#include <QToolBar>

class History;
class MainWindow;
class RepoView;
class SearchField;
class QButtonGroup;
class QToolButton;

class ToolBar : public QToolBar {
  Q_OBJECT

public:
  ToolBar(MainWindow *parent);

  SearchField *searchField() const { return mSearchField; }

private:
  void updateButtons(int ahead, int behind);
  void updateRemote(int ahead, int behind);
  void updateHistory();
  void updateStash();
  void updateView();
  void updateSearch();

  RepoView *currentView() const;

  QToolButton *mPrevButton;
  QToolButton *mNextButton;

  QToolButton *mFetchButton;
  QToolButton *mPullButton;
  QToolButton *mPushButton;

  QToolButton *mCheckoutButton;

  QToolButton *mStashButton;
  QToolButton *mStashPopButton;

  QToolButton *mRefreshButton;

  QToolButton *mRebaseContinueButton;
  QToolButton *mRebaseAbortButton;

  QToolButton *mPullRequestButton = nullptr;

  QToolButton *mTerminalButton;
  QToolButton *mFileManagerButton;
  QToolButton *mLogButton;
  const QButtonGroup *mModeGroup;

  QToolButton *mStarButton;
  SearchField *mSearchField;

  QAction *mRepoConfigAction;

  friend class MainWindow;
  friend class RepoView;
};

#endif
