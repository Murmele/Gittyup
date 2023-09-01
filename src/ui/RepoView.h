//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REPOVIEW_H
#define REPOVIEW_H

#include "dialogs/ConfigDialog.h"
#include "git/AnnotatedCommit.h"
#include "git/Blob.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Reference.h"
#include "git/Remote.h"
#include "git/Repository.h"
#include "git/Submodule.h"
#include "git/Rebase.h"
#include "host/Account.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QProcess>
#include <QSplitter>
#include <QTimer>
#include <functional>

class CommitList;
class DetailView;
class EditorWindow;
class History;
class Index;
class Location;
class LogEntry;
class LogView;
class MainWindow;
class PathspecWidget;
class ReferenceWidget;
class RemoteCallbacks;
class ToolBar;
struct ContributorInfo;

namespace git {
class Result;
}

class RepoView : public QSplitter {
  Q_OBJECT

public:
  enum ViewMode {
    DoubleTree,
    // Diff,
    Tree,
  };

  enum MergeFlag {
    Default = 0x0,
    Merge = 0x1,
    Rebase = 0x2,
    FastForward = 0x4,   // equivalent to --ff-only
    NoFastForward = 0x8, // equivalent to --no-ff
    NoCommit = 0x10,     // equivalent to --no-commit
    Squash = 0x20
  };

  Q_DECLARE_FLAGS(MergeFlags, MergeFlag);

  RepoView(const git::Repository &repo, MainWindow *parent);
  virtual ~RepoView();

  // clean
  void clean(const QStringList &untracked);

  // Select head reference in ref list.
  void selectHead();
  void selectFirstCommit();

  git::Repository repo() const { return mRepo; }
  History *history() const { return mHistory; }
  Index *index() const { return mIndex; }

  Repository *remoteRepo();

  // LFS
  void lfsInitialize();
  void lfsDeinitialize();
  bool lfsSetLocked(const QStringList &paths, bool locked);

  // commit
  void commit(bool force = false);
  bool isCommitEnabled() const;

  // stage / unstage
  void stage();
  bool isStageEnabled() const;
  void unstage();
  bool isUnstageEnabled() const;

  // mode
  ViewMode viewMode() const;
  void setViewMode(ViewMode mode);

  // workdir
  bool isWorkingDirectoryDirty() const;

  // current reference
  git::Reference reference() const;
  void selectReference(const git::Reference &ref);

  // current selection
  QList<git::Commit> commits() const;
  git::Diff diff() const;
  git::Tree tree() const;

  // background tasks
  void cancelRemoteTransfer();
  void cancelBackgroundTasks();

  // links
  void visitLink(const QString &link);

  // history location
  Location location() const;
  void setLocation(const Location &location);

  // history
  void find();
  void findNext();
  void findPrevious();

  // search index
  void startIndexing();
  void cancelIndexing();

  // log window
  bool isLogVisible() const;
  void setLogVisible(bool visible);

  /*!
   * \brief addLogEntry
   * \param text The text which is shown after the title
   * \param title The text which is shown in bold
   * \param parent
   * \return
   */
  LogEntry *addLogEntry(const QString &text, const QString &title,
                        LogEntry *parent = nullptr);

  /*!
   * \brief error
   * \param parent
   * \param action The Action text
   * \param name
   * \param defaultError
   *
   * Example:
   * Unable to Action 'Name' - the path
   * \return
   */
  LogEntry *error(LogEntry *parent, const QString &action,
                  const QString &name = QString(),
                  const QString &defaultError = QString());

  // automatic fetch
  void startFetchTimer();

  // fetch
  void fetchAll();
  QFuture<git::Result> fetch(const git::Remote &remote = git::Remote(),
                             bool tags = false, bool interactive = true,
                             LogEntry *parent = nullptr,
                             QStringList *submodules = nullptr);
  QFuture<git::Result> fetch(const git::Remote &remote, bool tags,
                             bool interactive, LogEntry *parent,
                             QStringList *submodules, bool prune);

  // pull
  void pull(MergeFlags flags = Default,
            const git::Remote &remote = git::Remote(), bool tags = false,
            bool prune = false);
  void merge(MergeFlags flags, const git::Reference &ref = git::Reference(),
             const git::AnnotatedCommit &commit = git::AnnotatedCommit(),
             LogEntry *parent = nullptr,
             const std::function<void()> &callback = std::function<void()>());

  // fast-forward
  void
  fastForward(const git::Reference &ref, const git::AnnotatedCommit &upstream,
              LogEntry *parent,
              const std::function<void()> &callback = std::function<void()>());

  // merge
  void merge(MergeFlags flags, const git::AnnotatedCommit &upstream,
             LogEntry *parent,
             const std::function<void()> &callback = std::function<void()>());
  void mergeAbort(LogEntry *parent = nullptr);

  // rebase
  void rebase(const git::AnnotatedCommit &upstream, LogEntry *parent);

  // Aborting the current ongoing rebase
  void abortRebase();

  // Continuouing the current ongoing rebase
  void continueRebase();

  // squash
  void squash(const git::AnnotatedCommit &upstream, LogEntry *parent);

  // revert
  void revert(const git::Commit &commit);

  // cherry-pick
  void cherryPick(const git::Commit &commit);

  // push
  void promptToForcePush(const git::Remote &remote = git::Remote(),
                         const git::Reference &src = git::Reference());

  void push(const git::Remote &remote = git::Remote(),
            const git::Reference &src = git::Reference(),
            const QString &dst = QString(), bool setUpstream = false,
            bool force = false, bool tags = false);

  // commit
  bool commit(const git::Signature &author, const git::Signature &commiter,
              const QString &message,
              const git::AnnotatedCommit &upstream = git::AnnotatedCommit(),
              LogEntry *parent = nullptr, bool force = false,
              bool fakeSignature = false);
  bool commit(const QString &message,
              const git::AnnotatedCommit &upstream = git::AnnotatedCommit(),
              LogEntry *parent = nullptr, bool force = false);
  void amendCommit();

  // checkout
  void promptToCheckout();
  void checkout(const git::Commit &commit, const QStringList &paths);
  void checkout(const git::Reference &ref, bool detach = false);
  void checkout(const git::Commit &commit,
                const git::Reference &ref = git::Reference(),
                bool detach = true);

  // branches
  void promptToCreateBranch(const git::Commit &commit = git::Commit());
  git::Branch createBranch(const QString &name, const git::Commit &target,
                           const git::Branch &upstream = git::Branch(),
                           bool checkout = false, bool force = false);
  void promptToDeleteBranch(const git::Reference &ref);
  void promptToRenameBranch(const git::Branch &branch);

  // stash
  void promptToStash();
  void stash(const QString &message = QString());
  void applyStash(int index = 0);
  void dropStash(int index = 0);
  void popStash(int index = 0);

  // tag
  void promptToAddTag(const git::Commit &commit);
  void promptToDeleteTag(const git::Reference &ref);

  void promptToAmend(const git::Commit &commit);
  void amend(const git::Commit &commit, const git::Signature &author,
             const git::Signature &committer, const QString &commitMessage);

  // reset
  void promptToReset(const git::Commit &commit, git_reset_t type);
  void reset(const git::Commit &commit, git_reset_t type,
             const git::Commit &commitToAmend = git::Commit());

  // submodule
  void resetSubmodules(const QList<git::Submodule> &submodules, bool recursive,
                       git_reset_t type, LogEntry *parent);
  void updateSubmodules(
      const QList<git::Submodule> &submodules = QList<git::Submodule>(),
      bool recursive = true, bool init = false, bool checkout_force = false,
      LogEntry *parent = nullptr, bool restoreSelection = true);
  bool openSubmodule(const git::Submodule &submodule);

  // config
  ConfigDialog *
  configureSettings(ConfigDialog::Index index = ConfigDialog::General);

  // terminal
  void openTerminal();
  // file manager
  void openFileManager();

  // ignore
  void ignore(const QString &name);

  // editor
  EditorWindow *newEditor();
  bool edit(const QString &path, int line = -1);
  EditorWindow *openEditor(const QString &path, int line = -1,
                           const git::Blob &blob = git::Blob(),
                           const git::Commit &commit = git::Commit());

  // refresh
  void refresh();
  void refresh(bool restoreSelection);

  // pathspec search filter
  void setPathspec(const QString &path);

  git::Commit nextRevision(const QString &path) const;
  git::Commit previousRevision(const QString &path) const;

  void selectCommit(const git::Commit &commit, const QString &file);

  static RepoView *parentView(const QWidget *widget);

  enum class DetailSplitterWidgets {
    NotDefined,
    SideBar,    // commit list, header, ...
    DetailView, // DiffView, TreeView
  };
  /*!
   * \brief match
   * Check recursively if the searched object \p search is one of the childs of
   * \p parent The \p parent is not checked. This must be done manually \param
   * search Object which should match \param parent Parent of the childs which
   * should be checked \return true if one of the childs is the searched one,
   * else false
   */
  static bool match(QObject *search, QObject *parent);
  /*!
   * \brief detailsMaximized
   * Returns if the details are maximized or not
   * \return
   */
  bool detailsMaximized();
  /*!
   * \brief detailSplitterMaximize
   *
   * \param maximized
   */
  DetailSplitterWidgets detailSplitterMaximize(
      bool maximized,
      DetailSplitterWidgets maximizeWidget = DetailSplitterWidgets::NotDefined);

public slots:
  void diffSelected(const git::Diff diff, const QString &file,
                    bool spontaneous);

private slots:
  void rebaseInitError();
  void rebaseCommitInvalid(const git::Rebase rebase);
  void rebaseAboutToRebase(const git::Rebase rebase, const git::Commit before,
                           int currIndex);
  void rebaseFinished(const git::Rebase rebase);
  void rebaseCommitSuccess(const git::Rebase rebase, const git::Commit before,
                           const git::Commit after, int currIndex);
  void rebaseConflict(const git::Rebase rebase);

signals:
  void statusChanged(bool dirty);

protected:
  void showEvent(QShowEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private:
  struct SubmoduleInfo {
    git::Submodule submodule;
    git::Repository repo;
    LogEntry *entry;
  };

  ToolBar *toolBar() const;
  CommitList *commitList() const;

  void notifyReferenceUpdated(const QString &name);

  void startLogTimer();
  bool suspendLogTimer();
  void resumeLogTimer(bool suspended = true);

  QList<SubmoduleInfo>
  submoduleUpdateInfoList(const git::Repository &repo,
                          const QList<git::Submodule> &submodules, bool init,
                          bool checkout_force, LogEntry *parent);
  void updateSubmodulesAsync(const QList<SubmoduleInfo> &submodules,
                             bool recursive = true, bool init = false,
                             bool checkout_force = false,
                             bool restoreSelection = true);

  QList<SubmoduleInfo>
  submoduleResetInfoList(const git::Repository &repo,
                         const QList<git::Submodule> &submodules,
                         LogEntry *parent);
  void resetSubmodulesAsync(const QList<SubmoduleInfo> &submodules,
                            bool recursive, git_reset_t type);

  bool checkForConflicts(LogEntry *parent, const QString &action);

  git::Signature getSignature(const ContributorInfo &info);

  git::Repository mRepo;

  Index *mIndex;
  QProcess mIndexer;
  bool mRestartIndexer = false;

  History *mHistory;

  Repository *mRemoteRepo;
  bool mRemoteRepoCached = false;

  ReferenceWidget *mRefs;
  PathspecWidget *mPathspec;
  CommitList *mCommits;
  DetailView *mDetails;
  QWidget *mSideBar;

  LogEntry *mLogRoot;
  LogEntry *mRebase{nullptr};
  LogView *mLogView;
  QTimer mLogTimer;
  bool mIsLogVisible = false;

  QTimer mFetchTimer;
  RemoteCallbacks *mCallbacks = nullptr;
  QFutureWatcher<git::Result> *mWatcher = nullptr;

  QList<QWidget *> mTrackedWindows;

  bool mShown = false;

  friend class MenuBar;

  /*!
   * \brief mDetailSplitter
   * Splits the history list and the detailview (diffView, TreeView)
   */
  QSplitter *mDetailSplitter;
  /*!
   * \brief mMaximized
   * Maximizes the widgets in the mDetailSplitter
   * true: single widget is visible and the others are invisible
   * false: all widgets are sized normaly and visible
   */
  bool mMaximized{false};
};

#endif
