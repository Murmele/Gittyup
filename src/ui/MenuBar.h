//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef MENUBAR
#define MENUBAR

#include <QMenuBar>

class History;
class RepoView;
class StateAction;

class MenuBar : public QMenuBar {
  Q_OBJECT

public:
  MenuBar(QWidget *parent = nullptr);

  void update();
  void updateFile();
  void updateSave();
  void updateUndoRedo();
  void updateCutCopyPaste();
  void updateSelectAll();
  void updateFind();
  void updateView();
  void updateRepository();
  void updateRemote();
  void updateBranch();
  void updateSubmodules();
  void updateStash();
  void updateRebase();
  void updateHistory();
  void updateWindow();

  static void setDebugMenuVisible(bool show);
  static bool isDebugMenuVisible();
  static MenuBar *instance(QWidget *widget);
  /*!
   * \brief isMaximized
   * Returns the checkstate of the mToggleMaximize Action
   * \return
   */
  bool isMaximized();

  void registerActions(QWidget *parent) const;

  static const QString donationUrlLiberapay;

private:
  QWidget *window() const;
  /*!
   * \brief view
   * Returns the current view
   * \return
   */
  RepoView *view() const;
  /*!
   * \brief views
   * Return all open views. Needed in the maximize feature
   * \return
   */
  QList<RepoView *> views() const;

  // File
  QAction *mClose;
  QAction *mSave;

  // Edit
  QAction *mUndo;
  QAction *mRedo;
  QAction *mCut;
  QAction *mCopy;
  QAction *mPaste;
  QAction *mSelectAll;

  QAction *mFind;
  QAction *mFindNext;
  QAction *mFindPrevious;
  QAction *mFindSelection;

  // View
  QAction *mRefresh;
  QAction *mToggleLog;
  QAction *mToggleView;
  QAction *mToggleMenuBar;
  StateAction *mToggleMaximize;

  // Repository
  QAction *mConfigureRepository;
  QAction *mStageAll;
  QAction *mUnstageAll;
  QAction *mCommit;
  QAction *mAmendCommit;
  QAction *mApplyPatch;
  QAction *mLfsUnlock;
  QAction *mLfsInitialize;

  // Remote
  QAction *mConfigureRemotes;
  QAction *mFetch;
  QAction *mFetchAll;
  QAction *mFetchFrom;
  QAction *mPull;
  QAction *mPullFrom;
  QAction *mPush;
  QAction *mPushTo;

  // Branch
  QAction *mConfigureBranches;
  QAction *mNewBranch;
  QAction *mCheckoutCurrent;
  QAction *mCheckout;
  QAction *mRenameBranch;
  QAction *mMerge;
  QAction *mRebase;
  QAction *mSquash;
  QAction *mAbort;

  // Submodule
  QAction *mConfigureSubmodules;
  QAction *mUpdateSubmodules;
  QAction *mInitSubmodules;
  QMenu *mOpenSubmodule;

  // Stash
  QAction *mShowStashes;
  QAction *mStash;
  QAction *mStashPop;

  // History
  QAction *mPrev;
  QAction *mNext;

  // Window
  QAction *mPrevTab;
  QAction *mNextTab;

  static bool sDebugMenuVisible;
};

#endif
