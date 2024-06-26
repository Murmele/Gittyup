//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "MenuBar.h"
#include "BlameEditor.h"
#include "CommitList.h"
#include "EditorWindow.h"
#include "FindWidget.h"
#include "History.h"
#include "HotkeyManager.h"
#include "MainWindow.h"
#include "RepoView.h"
#include "TabWidget.h"
#include "StateAction.h"
#include "app/Application.h"
#include "conf/RecentRepositories.h"
#include "conf/RecentRepository.h"
#include "conf/Settings.h"
#include "cred/CredentialHelper.h"
#include "dialogs/AboutDialog.h"
#include "dialogs/CloneDialog.h"
#include "dialogs/MergeDialog.h"
#include "dialogs/RemoteDialog.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/StartDialog.h"
#include "dialogs/UpdateSubmodulesDialog.h"
#include "editor/TextEditor.h"
#include "git/Reference.h"
#include "git/Remote.h"
#include "git/RevWalk.h"
#include "git/Submodule.h"
#include "index/Index.h"
#include "log/LogEntry.h"
#include "log/LogView.h"
#include "update/Updater.h"
#include "util/Debug.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextEdit>

namespace {

void openCloneDialog(CloneDialog::Kind kind) {
  CloneDialog *dialog = new CloneDialog(kind);
  QObject::connect(dialog, &CloneDialog::accepted, [dialog] {
    if (MainWindow *window = MainWindow::open(dialog->path())) {
      RepoView *view = window->currentView();
      if (view)
        view->addLogEntry(dialog->message(), dialog->messageTitle());
    }
  });

  dialog->open();
}

} // namespace

const QString MenuBar::donationUrlLiberapay =
    QStringLiteral("https://liberapay.com/Gittyup/donate");

bool MenuBar::sDebugMenuVisible = false;

static Hotkey newFileHotkey = HotkeyManager::registerHotkey(
    QKeySequence::New, "file/new", "File/New File");

static Hotkey newWinHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Meta+N", "file/newWindow", "File/New Window");

static Hotkey cloneHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+N", "file/clone", "File/Clone Repository");

static Hotkey initHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Alt+N", "file/init", "File/Initialize New Repository");

static Hotkey openHotkey = HotkeyManager::registerHotkey(
    QKeySequence::Open, "file/open", "File/Open Repository");

static Hotkey closeHotkey = HotkeyManager::registerHotkey(
    QKeySequence::Close, "file/close", "File/Close");

static Hotkey saveHotkey =
    HotkeyManager::registerHotkey(QKeySequence::Save, "file/save", "File/Save");

#ifndef Q_OS_MAC
static Hotkey quitHotkey =
    HotkeyManager::registerHotkey(QKeySequence::Quit, "file/quit", "File/Exit");
#endif

static Hotkey undoHotkey =
    HotkeyManager::registerHotkey(QKeySequence::Undo, "edit/undo", "Edit/Undo");

static Hotkey redoHotkey =
    HotkeyManager::registerHotkey(QKeySequence::Redo, "edit/redo", "Edit/Redo");

static Hotkey cutHotkey =
    HotkeyManager::registerHotkey(QKeySequence::Cut, "edit/cut", "Edit/Cut");

static Hotkey copyHotkey =
    HotkeyManager::registerHotkey(QKeySequence::Copy, "edit/copy", "Edit/Copy");

static Hotkey pasteHotkey = HotkeyManager::registerHotkey(
    QKeySequence::Paste, "edit/paste", "Edit/Paste");

static Hotkey selectAllHotkey = HotkeyManager::registerHotkey(
    QKeySequence::SelectAll, "edit/selectAll", "Edit/Select All");

static Hotkey findHotkey =
    HotkeyManager::registerHotkey(QKeySequence::Find, "edit/find", "Edit/Find");

static Hotkey findNextHotkey = HotkeyManager::registerHotkey(
    QKeySequence::FindNext, "edit/findNext", "Edit/Find Next");

static Hotkey findPreviousHotkey = HotkeyManager::registerHotkey(
    QKeySequence::FindPrevious, "edit/findPrevious", "Edit/Find Previous");

static Hotkey findSelectionHotkey = HotkeyManager::registerHotkey(
    "Ctrl+E", "edit/findSelection", "Edit/Use Selection for Find");

static Hotkey refreshHotkey = HotkeyManager::registerHotkey(
    QKeySequence::Refresh, "view/refresh", "View/Refresh");

static Hotkey toggleLogHotkey =
    HotkeyManager::registerHotkey(nullptr, "view/toggleLog", "View/Toggle Log");

static Hotkey toggleMaximizeHotkey = HotkeyManager::registerHotkey(
    "Ctrl+M", "view/toggleMaximize", "View/Toggle Maximize");

static Hotkey toggleViewHotkey = HotkeyManager::registerHotkey(
    nullptr, "view/toggleView", "View/Toggle Tree View");

static Hotkey toggleMenuBarHotkey = HotkeyManager::registerHotkey(
    "Ctrl+B", "view/toggleMenuBar", "View/Toggle Menu Bar");

static Hotkey configureRepositoryHotkey = HotkeyManager::registerHotkey(
    nullptr, "repository/configure", "Repository/Configure Repository");

static Hotkey stageAllHotkey = HotkeyManager::registerHotkey(
    "Ctrl++", "repository/stageAll", "Repository/Stage All");

static Hotkey unstageAllHotkey = HotkeyManager::registerHotkey(
    "Ctrl+-", "repository/unstageAll", "Repository/Unstage All");

static Hotkey commitHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+C", "repository/commit", "Repository/Commit");

static Hotkey amendCommitHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+A", "repository/amendCommit", "Repository/Amend Commit");

static Hotkey lfsUnlockHotkey = HotkeyManager::registerHotkey(
    nullptr, "repository/lfs/unlock", "Repository/Remove all LFS locks");

static Hotkey lfsInitializeHotkey = HotkeyManager::registerHotkey(
    nullptr, "repository/lfs/initialize", "Repository/Initialize LFS");

static Hotkey configureRemotesHotkey = HotkeyManager::registerHotkey(
    nullptr, "remote/configure", "Remote/Configure");

static Hotkey fetchHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+Alt+F", "remote/fetch", "Remote/Fetch");

static Hotkey fetchAllHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+Alt+A", "remote/fetchAll", "Remote/Fetch All");

static Hotkey fetchFromHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+F", "remote/fetchFrom", "Remote/Fetch From");

static Hotkey pullHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+Alt+L", "remote/pull", "Remote/Pull");

static Hotkey pullFromHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+L", "remote/pullFrom", "Remote/Pull From");

static Hotkey pushHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+Alt+P", "remote/push", "Remote/Push");

static Hotkey pushToHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+P", "remote/pushTo", "Remote/Push To");

static Hotkey configureBranchesHotkey = HotkeyManager::registerHotkey(
    nullptr, "branch/configure", "Branch/Configure");

static Hotkey newBranchHotkey =
    HotkeyManager::registerHotkey(nullptr, "branch/new", "Branch/New");

static Hotkey renameBranchHotkey =
    HotkeyManager::registerHotkey(nullptr, "branch/rename", "Branch/Rename");

static Hotkey checkoutCurrentHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+Alt+H", "branch/checkoutCurrent", "Branch/Checkout Current");

static Hotkey checkoutHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+H", "branch/checkout", "Branch/Checkout");

static Hotkey mergeHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+M", "branch/merge", "Branch/Merge");

static Hotkey rebaseHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+R", "branch/rebase", "Branch/Rebase");

static Hotkey abortHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+A", "branch/abort", "Branch/Abort Merge");

static Hotkey configureSubmodulesHotkey = HotkeyManager::registerHotkey(
    nullptr, "branch/configure", "Branch/Configure");

static Hotkey updateSubmodulesHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+Alt+U", "branch/update", "Branch/Update All");

static Hotkey initSubmodulesHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+U", "branch/init", "Branch/Update");

static Hotkey showStashesHotkey =
    HotkeyManager::registerHotkey(nullptr, "stash/show", "Stash/Show Stashes");

static Hotkey stashHotkey =
    HotkeyManager::registerHotkey("Ctrl+Shift+T", "stash/stash", "Stash/Stash");

static Hotkey stashPopHotkey =
    HotkeyManager::registerHotkey("Ctrl+Shift+Alt+T", "stash/pop", "Stash/Pop");

static Hotkey prevHotkey = HotkeyManager::registerHotkey(
    QKeySequence::Back, "history/prev", "History/Back");

static Hotkey nextHotkey = HotkeyManager::registerHotkey(
    QKeySequence::Forward, "history/next", "History/Forward");

static Hotkey prevTabHotkey = HotkeyManager::registerHotkey(
    QKeySequence::PreviousChild, "window/prevTab", "Window/Show Previous Tab");

static Hotkey nextTabHotkey = HotkeyManager::registerHotkey(
    QKeySequence::NextChild, "window/nextTab", "Window/Show Next Tab");

static Hotkey chooserHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+O", "window/chooser", "Window/Show Repository Chooser");

static Hotkey preferencesHotkey = HotkeyManager::registerHotkey(
    nullptr, "tools/preferences", "Tools/Options");

static Hotkey squashHotkey = HotkeyManager::registerHotkey(
    "Ctrl+Shift+Q", "tools/preferences", "Tools/Options");

MenuBar::MenuBar(QWidget *parent) : QMenuBar(parent) {

  Settings *settings = Settings::instance();

  setHidden(settings->value(Setting::Id::HideMenuBar).toBool());

  // File
  QMenu *file = addMenu(tr("File"));

  QAction *newFile = file->addAction(tr("New File"));
  newFileHotkey.use(newFile);
  connect(newFile, &QAction::triggered, [] {
    if (MainWindow *window = MainWindow::activeWindow()) {
      if (RepoView *view = window->currentView()) {
        view->newEditor();
        return;
      }
    }

    EditorWindow *window = new EditorWindow;
    window->show();
  });

  QAction *newWin = file->addAction(tr("New Window"));
  newWinHotkey.use(newWin);
  connect(newWin, &QAction::triggered, [] { MainWindow::open(); });

  QAction *clone = file->addAction(tr("Clone Repository..."));
  cloneHotkey.use(clone);
  connect(clone, &QAction::triggered,
          [] { openCloneDialog(CloneDialog::Clone); });

  QAction *init = file->addAction(tr("Initialize New Repository..."));
  initHotkey.use(init);
  connect(init, &QAction::triggered,
          [] { openCloneDialog(CloneDialog::Init); });

  file->addSeparator();

  QAction *open = file->addAction(tr("Open Repository..."));
  openHotkey.use(open);
  connect(open, &QAction::triggered, [] {
    // FIXME: Filter out non-git dirs.
    Settings *settings = Settings::instance();
    QString title = tr("Open Repository");
    QString path = QFileDialog::getExistingDirectory(
        nullptr, title, settings->lastPath(), QFileDialog::ShowDirsOnly);
    MainWindow::open(path);
    settings->setLastPath(path);
  });

  QMenu *openRecent = file->addMenu(tr("Open Recent"));
  connect(openRecent, &QMenu::aboutToShow, [openRecent] {
    openRecent->clear();
    RecentRepositories *repos = RecentRepositories::instance();
    for (int i = 0; i < repos->count(); ++i) {
      RecentRepository *repo = repos->repository(i);
      QAction *action = openRecent->addAction(repo->name());
      connect(action, &QAction::triggered,
              [repo] { MainWindow::open(repo->gitpath()); });
    }
  });

  file->addSeparator();

  mClose = file->addAction(tr("Close"));
  closeHotkey.use(mClose);
  connect(mClose, &QAction::triggered, [] {
    QWidget *window = QApplication::activeWindow();
    if (!window)
      return;

    if (MainWindow *win = qobject_cast<MainWindow *>(window)) {
      if (win->count() > 1) {
        if (auto c = win->currentView())
          c->close();
        return;
      }
    }

    window->close();
  });

  mSave = file->addAction(tr("Save"));
  saveHotkey.use(mSave);
  mSave->setEnabled(false);
  connect(mSave, &QAction::triggered,
          [this] { static_cast<EditorWindow *>(window())->widget()->save(); });

  file->addSeparator();

#ifndef Q_OS_MAC
  QAction *quit = file->addAction(tr("Exit"));
  quit->setMenuRole(QAction::QuitRole);
  quitHotkey.use(quit);
  connect(quit, &QAction::triggered, &QApplication::closeAllWindows);
#endif

  // Edit
  QMenu *edit = addMenu(tr("Edit"));

  mUndo = edit->addAction(tr("Undo"));
  undoHotkey.use(mUndo);
  connect(mUndo, &QAction::triggered, [] {
    QWidget *widget = QApplication::focusWidget();
    if (TextEditor *editor = qobject_cast<TextEditor *>(widget)) {
      editor->undo();
    } else if (QLineEdit *editor = qobject_cast<QLineEdit *>(widget)) {
      editor->undo();
    } else if (QTextEdit *editor = qobject_cast<QTextEdit *>(widget)) {
      editor->undo();
    }
  });

  mRedo = edit->addAction(tr("Redo"));
  redoHotkey.use(mRedo);
  connect(mRedo, &QAction::triggered, [] {
    QWidget *widget = QApplication::focusWidget();
    if (TextEditor *editor = qobject_cast<TextEditor *>(widget)) {
      editor->redo();
    } else if (QTextEdit *editor = qobject_cast<QTextEdit *>(widget)) {
      editor->redo();
    } else if (QLineEdit *editor = qobject_cast<QLineEdit *>(widget)) {
      editor->redo();
    }
  });

  edit->addSeparator();

  mCut = edit->addAction(tr("Cut"));
  cutHotkey.use(mCut);
  connect(mCut, &QAction::triggered, [] {
    QWidget *widget = QApplication::focusWidget();
    if (TextEditor *editor = qobject_cast<TextEditor *>(widget)) {
      editor->cut();
    } else if (QTextEdit *editor = qobject_cast<QTextEdit *>(widget)) {
      editor->cut();
    } else if (QLineEdit *editor = qobject_cast<QLineEdit *>(widget)) {
      editor->cut();
    }
  });

  mCopy = edit->addAction(tr("Copy"));
  copyHotkey.use(mCopy);
  connect(mCopy, &QAction::triggered, [] {
    QWidget *widget = QApplication::focusWidget();
    if (TextEditor *editor = qobject_cast<TextEditor *>(widget)) {
      editor->copy();
    } else if (QTextEdit *editor = qobject_cast<QTextEdit *>(widget)) {
      editor->copy();
    } else if (QLineEdit *editor = qobject_cast<QLineEdit *>(widget)) {
      editor->copy();
    } else if (LogView *logView = qobject_cast<LogView *>(widget)) {
      logView->copy();
    }
  });

  mPaste = edit->addAction(tr("Paste"));
  pasteHotkey.use(mPaste);
  connect(mPaste, &QAction::triggered, [] {
    QWidget *widget = QApplication::focusWidget();
    if (TextEditor *editor = qobject_cast<TextEditor *>(widget)) {
      editor->paste();
    } else if (QTextEdit *editor = qobject_cast<QTextEdit *>(widget)) {
      editor->paste();
    } else if (QLineEdit *editor = qobject_cast<QLineEdit *>(widget)) {
      editor->paste();
    }
  });

  mSelectAll = edit->addAction(tr("Select All"));
  selectAllHotkey.use(mSelectAll);
  connect(mSelectAll, &QAction::triggered, [] {
    QWidget *widget = QApplication::focusWidget();
    if (TextEditor *editor = qobject_cast<TextEditor *>(widget)) {
      editor->selectAll();
    } else if (QTextEdit *editor = qobject_cast<QTextEdit *>(widget)) {
      editor->selectAll();
    } else if (QLineEdit *editor = qobject_cast<QLineEdit *>(widget)) {
      editor->selectAll();
    }
  });

  edit->addSeparator();

  mFind = edit->addAction(tr("Find..."));
  mFind->setObjectName("Find");
  findHotkey.use(mFind);
  connect(mFind, &QAction::triggered, [] {
    QWidget *widget = QApplication::activeWindow();
    if (MainWindow *window = qobject_cast<MainWindow *>(widget)) {
      if (auto c = window->currentView())
        c->find();
    } else if (EditorWindow *window = qobject_cast<EditorWindow *>(widget)) {
      window->widget()->find();
    }
  });

  mFindNext = edit->addAction(tr("Find Next"));
  findNextHotkey.use(mFindNext);
  connect(mFindNext, &QAction::triggered, [] {
    QWidget *widget = QApplication::activeWindow();
    if (MainWindow *window = qobject_cast<MainWindow *>(widget)) {
      if (auto c = window->currentView())
        c->findNext();
    } else if (EditorWindow *window = qobject_cast<EditorWindow *>(widget)) {
      window->widget()->findNext();
    }
  });

  mFindPrevious = edit->addAction(tr("Find Previous"));
  findPreviousHotkey.use(mFindPrevious);
  connect(mFindPrevious, &QAction::triggered, [] {
    QWidget *widget = QApplication::activeWindow();
    if (MainWindow *window = qobject_cast<MainWindow *>(widget)) {
      if (auto c = window->currentView())
        c->findPrevious();
    } else if (EditorWindow *window = qobject_cast<EditorWindow *>(widget)) {
      window->widget()->findPrevious();
    }
  });

  mFindSelection = edit->addAction(tr("Use Selection for Find"));
  findSelectionHotkey.use(mFindSelection);
  connect(mFindSelection, &QAction::triggered, [this] {
    QWidget *widget = QApplication::focusWidget();
    if (TextEditor *editor = qobject_cast<TextEditor *>(widget)) {
      FindWidget::setText(editor->selText());
    } else if (QTextEdit *editor = qobject_cast<QTextEdit *>(widget)) {
      FindWidget::setText(editor->textCursor().selectedText());
    } else if (QLineEdit *editor = qobject_cast<QLineEdit *>(widget)) {
      FindWidget::setText(editor->selectedText());
    }

    // Update next/prev.
    updateFind();
  });

  // View
  QMenu *viewMenu = addMenu(tr("View"));

  mRefresh = viewMenu->addAction(tr("Refresh"));
  refreshHotkey.use(mRefresh);
  connect(mRefresh, &QAction::triggered, [this] { view()->refresh(); });

  viewMenu->addSeparator();

  mToggleLog = viewMenu->addAction(tr("Show Log"));
  toggleLogHotkey.use(mToggleLog);
  connect(mToggleLog, &QAction::triggered, [this] {
    RepoView *view = this->view();
    view->setLogVisible(!view->isLogVisible());
  });

  mToggleMaximize = new StateAction(tr("Normal"), tr("Maximize"), viewMenu);
  viewMenu->addAction(mToggleMaximize);
  toggleMaximizeHotkey.use(mToggleMaximize);
  mToggleMaximize->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
  connect(mToggleMaximize, &QAction::triggered, [this] {
    bool maximize = mToggleMaximize->isActive();
    RepoView *view = this->view();
    RepoView::DetailSplitterWidgets widget =
        view->detailSplitterMaximize(maximize);
    assert(!maximize ||
           (maximize == true &&
            widget != RepoView::DetailSplitterWidgets::NotDefined));

    QList<RepoView *> repos = this->views();
    for (auto repo : repos) {
      repo->detailSplitterMaximize(maximize, widget);
    }
  });

  mToggleView = viewMenu->addAction(tr("Show Tree View"));
  toggleViewHotkey.use(mToggleView);
  connect(mToggleView, &QAction::triggered, [this] {
    RepoView *view = this->view();
    bool diff = (view->viewMode() == RepoView::DoubleTree);
    view->setViewMode(diff ? RepoView::Tree : RepoView::DoubleTree);
  });

  mToggleMenuBar = viewMenu->addAction(tr("Hide Menu Bar"));
  toggleMenuBarHotkey.use(mToggleMenuBar);
  connect(mToggleMenuBar, &QAction::triggered, [this] {
    setHidden(!isHidden());
    Settings::instance()->setValue(Setting::Id::HideMenuBar, isHidden());
  });

  // Repository
  QMenu *repository = addMenu(tr("Repository"));

  mConfigureRepository = repository->addAction(tr("Configure Repository..."));
  configureRepositoryHotkey.use(mConfigureRepository);
  connect(mConfigureRepository, &QAction::triggered,
          [this] { view()->configureSettings(); });

  repository->addSeparator();

  mStageAll = repository->addAction(tr("Stage All"));
  stageAllHotkey.use(mStageAll);
  connect(mStageAll, &QAction::triggered, [this] { view()->stage(); });

  mUnstageAll = repository->addAction(tr("Unstage All"));
  unstageAllHotkey.use(mUnstageAll);
  connect(mUnstageAll, &QAction::triggered, [this] { view()->unstage(); });

  repository->addSeparator();

  mCommit = repository->addAction(tr("Commit"));
  commitHotkey.use(mCommit);
  connect(mCommit, &QAction::triggered, [this] { view()->commit(); });

  mAmendCommit = repository->addAction(tr("Amend Commit"));
  amendCommitHotkey.use(mAmendCommit);
  connect(mAmendCommit, &QAction::triggered, [this] { view()->amendCommit(); });

  repository->addSeparator();

  QMenu *lfs = repository->addMenu(tr("Git LFS"));
  mLfsUnlock = lfs->addAction(tr("Remove all locks"));
  lfsUnlockHotkey.use(mLfsUnlock);
  connect(mLfsUnlock, &QAction::triggered, [this] {
    view()->lfsSetLocked(view()->repo().lfsLocks().values(), false);
  });

  lfs->addSeparator();

  mLfsInitialize = lfs->addAction(tr("Initialize"));
  lfsInitializeHotkey.use(mLfsInitialize);
  connect(mLfsInitialize, &QAction::triggered,
          [this] { view()->lfsInitialize(); });

  // Remote
  QMenu *remote = addMenu(tr("Remote"));

  mConfigureRemotes = remote->addAction(tr("Configure Remotes..."));
  mConfigureRemotes->setMenuRole(QAction::NoRole);
  configureRemotesHotkey.use(mConfigureRemotes);
  connect(mConfigureRemotes, &QAction::triggered,
          [this] { view()->configureSettings(ConfigDialog::Remotes); });

  remote->addSeparator();

  mFetch = remote->addAction(tr("Fetch"));
  fetchHotkey.use(mFetch);
  connect(mFetch, &QAction::triggered, [this] { view()->fetch(); });

  mFetchAll = remote->addAction(tr("Fetch All"));
  fetchAllHotkey.use(mFetchAll);
  connect(mFetchAll, &QAction::triggered, [this] { view()->fetchAll(); });

  mFetchFrom = remote->addAction(tr("Fetch From..."));
  fetchFromHotkey.use(mFetchFrom);
  connect(mFetchFrom, &QAction::triggered, [this] {
    RemoteDialog *dialog = new RemoteDialog(RemoteDialog::Fetch, view());
    dialog->open();
  });

  remote->addSeparator();

  mPull = remote->addAction(tr("Pull"));
  pullHotkey.use(mPull);
  connect(mPull, &QAction::triggered, [this] { view()->pull(); });

  mPullFrom = remote->addAction(tr("Pull From..."));
  pullFromHotkey.use(mPullFrom);
  connect(mPullFrom, &QAction::triggered, [this] {
    RemoteDialog *dialog = new RemoteDialog(RemoteDialog::Pull, view());
    dialog->open();
  });

  remote->addSeparator();

  mPush = remote->addAction(tr("Push"));
  pushHotkey.use(mPush);
  connect(mPush, &QAction::triggered, [this] { view()->push(); });

  mPushTo = remote->addAction(tr("Push To..."));
  pushToHotkey.use(mPushTo);
  connect(mPushTo, &QAction::triggered, [this] {
    RemoteDialog *dialog = new RemoteDialog(RemoteDialog::Push, view());
    dialog->open();
  });

  // Branch
  QMenu *branch = addMenu(tr("Branch"));

  mConfigureBranches = branch->addAction(tr("Configure Branches..."));
  mConfigureBranches->setMenuRole(QAction::NoRole);
  configureBranchesHotkey.use(mConfigureBranches);
  connect(mConfigureBranches, &QAction::triggered,
          [this] { view()->configureSettings(ConfigDialog::Branches); });

  mNewBranch = branch->addAction(tr("New Branch..."));
  newBranchHotkey.use(mNewBranch);
  connect(mNewBranch, &QAction::triggered,
          [this] { view()->promptToCreateBranch(); });

  mRenameBranch = branch->addAction(tr("Rename Branch"));
  renameBranchHotkey.use(mRenameBranch);
  connect(mRenameBranch, &QAction::triggered, [this] {
    this->view()->promptToRenameBranch(this->view()->reference());
  });

  branch->addSeparator();

  mCheckoutCurrent = branch->addAction(tr("Checkout Current"));
  checkoutCurrentHotkey.use(mCheckoutCurrent);
  connect(mCheckoutCurrent, &QAction::triggered, [this] {
    RepoView *view = this->view();
    git::Reference ref = view->reference();
    Q_ASSERT(ref.qualifiedName() != view->repo().head().qualifiedName());

    // FIXME: Disallow non-local branch?
    view->checkout(ref, !ref.isLocalBranch());
  });

  mCheckout = branch->addAction(tr("Checkout..."));
  checkoutHotkey.use(mCheckout);
  connect(mCheckout, &QAction::triggered,
          [this] { view()->promptToCheckout(); });

  branch->addSeparator();

  mMerge = branch->addAction(tr("Merge..."));
  mergeHotkey.use(mMerge);
  connect(mMerge, &QAction::triggered, [this] {
    RepoView *view = this->view();
    MergeDialog *dialog = new MergeDialog(RepoView::Merge, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->open();
  });

  mRebase = branch->addAction(tr("Rebase..."));
  rebaseHotkey.use(mRebase);
  connect(mRebase, &QAction::triggered, [this] {
    RepoView *view = this->view();
    MergeDialog *dialog = new MergeDialog(RepoView::Rebase, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->open();
  });

  mSquash = branch->addAction(tr("Squash..."));
  squashHotkey.use(mSquash);
  connect(mSquash, &QAction::triggered, [this] {
    RepoView *view = this->view();
    MergeDialog *dialog = new MergeDialog(RepoView::Squash, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->open();
  });

  branch->addSeparator();

  mAbort = branch->addAction(tr("Abort Merge"));
  abortHotkey.use(mAbort);
  connect(mAbort, &QAction::triggered, [this] { view()->mergeAbort(); });

  // Submodule
  QMenu *submodule = addMenu(tr("Submodule"));

  mConfigureSubmodules = submodule->addAction(tr("Configure Submodules..."));
  mConfigureSubmodules->setMenuRole(QAction::NoRole);
  configureSubmodulesHotkey.use(mConfigureSubmodules);
  connect(mConfigureSubmodules, &QAction::triggered,
          [this] { view()->configureSettings(ConfigDialog::Submodules); });

  submodule->addSeparator();

  mUpdateSubmodules = submodule->addAction(tr("Update All"));
  updateSubmodulesHotkey.use(mUpdateSubmodules);
  connect(mUpdateSubmodules, &QAction::triggered,
          [this] { view()->updateSubmodules(view()->repo().submodules()); });

  mInitSubmodules = submodule->addAction(tr("Update..."));
  initSubmodulesHotkey.use(mInitSubmodules);
  connect(mInitSubmodules, &QAction::triggered, [this] {
    RepoView *view = this->view();
    git::Repository repo = view->repo();
    UpdateSubmodulesDialog *dialog = new UpdateSubmodulesDialog(repo, view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->updateSubmodules(dialog->submodules(), dialog->recursive(),
                             dialog->init());
    });

    dialog->open();
  });

  submodule->addSeparator();

  mOpenSubmodule = submodule->addMenu(tr("Open"));
  connect(mOpenSubmodule, &QMenu::aboutToShow, [this] {
    mOpenSubmodule->clear();

    // FIXME: Disabling the submenu doesn't actually work on Mac.
    MainWindow *win = qobject_cast<MainWindow *>(window());
    if (!win)
      return;

    RepoView *view = win->currentView();
    if (view) {
      foreach (const git::Submodule &submodule, view->repo().submodules()) {
        QAction *action = mOpenSubmodule->addAction(submodule.name());
        connect(action, &QAction::triggered,
                [view, submodule] { view->openSubmodule(submodule); });
      }
    }
  });

  // Stash
  QMenu *stash = addMenu(tr("Stash"));

  mShowStashes = stash->addAction(tr("Show Stashes"));
  showStashesHotkey.use(mShowStashes);
  connect(mShowStashes, &QAction::triggered, [this] {
    RepoView *view = this->view();
    view->selectReference(view->repo().stashRef());
  });

  stash->addSeparator();

  mStash = stash->addAction(tr("Stash..."));
  stashHotkey.use(mStash);
  connect(mStash, &QAction::triggered, [this] { view()->promptToStash(); });

  mStashPop = stash->addAction(tr("Pop Stash"));
  stashPopHotkey.use(mStashPop);
  connect(mStashPop, &QAction::triggered, [this] { view()->popStash(); });

  // History
  QMenu *historyMenu = addMenu(tr("History"));

  mPrev = historyMenu->addAction(tr("Back"));
  prevHotkey.use(mPrev);
  mPrev->setEnabled(false);
  connect(mPrev, &QAction::triggered, [this] { view()->history()->prev(); });

  mNext = historyMenu->addAction(tr("Forward"));
  nextHotkey.use(mNext);
  mNext->setEnabled(false);
  connect(mNext, &QAction::triggered, [this] { view()->history()->next(); });

  // Window
  QMenu *windowMenu = addMenu(tr("Window"));
  mPrevTab = windowMenu->addAction(tr("Show Previous Tab"));
  prevTabHotkey.use(mPrevTab);
  connect(mPrevTab, &QAction::triggered, [this] {
    MainWindow *win = static_cast<MainWindow *>(window());
    TabWidget *tabs = win->tabWidget();
    int index = tabs->currentIndex();
    tabs->setCurrentIndex((index ? index : tabs->count()) - 1);
  });

  mNextTab = windowMenu->addAction(tr("Show Next Tab"));
  nextTabHotkey.use(mNextTab);
  connect(mNextTab, &QAction::triggered, [this] {
    MainWindow *win = static_cast<MainWindow *>(window());
    TabWidget *tabs = win->tabWidget();
    int index = tabs->currentIndex();
    tabs->setCurrentIndex((index == tabs->count() - 1) ? 0 : index + 1);
  });

  windowMenu->addSeparator();

  QAction *chooser = windowMenu->addAction(tr("Show Repository Chooser..."));
  chooserHotkey.use(chooser);
  connect(chooser, &QAction::triggered, &StartDialog::openSharedInstance);

  // Tools
  QMenu *tools = addMenu(tr("Tools"));
  QAction *preferences = tools->addAction(tr("Options..."));
  preferencesHotkey.use(preferences);
  connect(preferences, &QAction::triggered,
          [] { SettingsDialog::openSharedInstance(); });

  // Help
  QMenu *help = addMenu(tr("Help"));
  QString name = QCoreApplication::applicationName();
  QAction *about = help->addAction(tr("About %1").arg(name));
  about->setMenuRole(QAction::AboutRole);
  connect(about, &QAction::triggered,
          [] { AboutDialog::openSharedInstance(); });

  QAction *update = help->addAction(tr("Check For Updates..."));
  update->setMenuRole(QAction::ApplicationSpecificRole);
  connect(update, &QAction::triggered, Updater::instance(), &Updater::update);

  QAction *plugin = help->addAction(tr("Plugin Documentation..."));
  connect(plugin, &QAction::triggered, [] {
    QString url = Settings::docDir().filePath("plugin.html");
    QDesktopServices::openUrl(QUrl::fromLocalFile(url));
  });

  QAction *donation = help->addAction(tr("Support us via Liberapay"));
  connect(donation, &QAction::triggered,
          [] { QDesktopServices::openUrl(QUrl(donationUrlLiberapay)); });

  // Debug
  if (sDebugMenuVisible) {
    QMenu *debug = addMenu(tr("Debug"));
    QAction *abort = debug->addAction(tr("Abort"));
    connect(abort, &QAction::triggered, [] {
#ifdef Q_OS_WIN
      // Cause a crash.
      int *crash = nullptr;
      *crash = 0;
#endif

      ::abort();
    });

    debug->addSeparator();

    QAction *indexer = debug->addAction(tr("Log Indexer Progress"));
    indexer->setCheckable(true);
    indexer->setChecked(Index::isLoggingEnabled());
    connect(indexer, &QAction::triggered,
            [](bool checked) { Index::setLoggingEnabled(checked); });

    QAction *cred = debug->addAction(tr("Log Credential Helper"));
    cred->setCheckable(true);
    cred->setChecked(CredentialHelper::isLoggingEnabled());
    connect(cred, &QAction::triggered,
            [](bool checked) { CredentialHelper::setLoggingEnabled(checked); });

    QAction *remote = debug->addAction(tr("Log Remote Connection"));
    remote->setCheckable(true);
    remote->setChecked(git::Remote::isLoggingEnabled());
    connect(remote, &QAction::triggered,
            [](bool checked) { git::Remote::setLoggingEnabled(checked); });

    QAction *debugMessages = debug->addAction(tr("Log Debug Messages"));
    debugMessages->setCheckable(true);
    debugMessages->setChecked(Debug::isLogging());
    connect(debugMessages, &QAction::triggered,
            [](bool checked) { Debug::setLogging(checked); });

    debug->addSeparator();

    QAction *diffs = debug->addAction(tr("Load All Diffs"));
    connect(diffs, &QAction::triggered, [this] {
      if (MainWindow *win = qobject_cast<MainWindow *>(window())) {
        if (RepoView *view = win->currentView()) {
          CommitList *commits = view->commitList();
          QAbstractItemModel *model = commits->model();
          for (int i = 0; i < model->rowCount(); ++i) {
            commits->setCurrentIndex(model->index(i, 0));
            view->find(); // Force editors to load.
            QCoreApplication::processEvents();
          }
        }
      }
    });

    QAction *walk = debug->addAction(tr("Walk Commits"));
    connect(walk, &QAction::triggered, [this] {
      if (MainWindow *win = qobject_cast<MainWindow *>(window())) {
        if (auto c = win->currentView()) {
          git::RevWalk walker = c->repo().walker();
          while (git::Commit commit = walker.next())
            (void)commit;
        }
      }
    });
  }

  // Respond to window activation.
  connect(qApp, &QApplication::focusChanged, this, &MenuBar::update);
}

void MenuBar::update() {
  updateFile();
  updateSave();
  updateUndoRedo();
  updateCutCopyPaste();
  updateSelectAll();
  updateFind();
  updateView();
  updateRepository();
  updateRemote();
  updateBranch();
  updateSubmodules();
  updateStash();
  updateHistory();
  updateWindow();
}

void MenuBar::updateFile() { mClose->setEnabled(QApplication::activeWindow()); }

void MenuBar::updateSave() {
  EditorWindow *win = qobject_cast<EditorWindow *>(window());
  mSave->setEnabled(win && win->widget()->editor()->isModified());
}

void MenuBar::updateUndoRedo() {
  mUndo->setEnabled(false);
  mRedo->setEnabled(false);

  QWidget *widget = QApplication::focusWidget();
  if (TextEditor *editor = qobject_cast<TextEditor *>(widget)) {
    mUndo->setEnabled(editor->canUndo());
    mRedo->setEnabled(editor->canRedo());
  } else if (QTextEdit *editor = qobject_cast<QTextEdit *>(widget)) {
    mUndo->setEnabled(editor->document()->isUndoAvailable());
    mRedo->setEnabled(editor->document()->isRedoAvailable());
  } else if (QLineEdit *editor = qobject_cast<QLineEdit *>(widget)) {
    mUndo->setEnabled(editor->isUndoAvailable());
    mRedo->setEnabled(editor->isRedoAvailable());
  }
}

void MenuBar::updateCutCopyPaste() {
  mCut->setEnabled(false);
  mCopy->setEnabled(false);
  mPaste->setEnabled(false);
  mFindSelection->setEnabled(false);

  QWidget *widget = QApplication::focusWidget();
  bool canPaste = !QApplication::clipboard()->text().isEmpty();
  if (TextEditor *editor = qobject_cast<TextEditor *>(widget)) {
    mCut->setEnabled(!editor->selectionEmpty() && !editor->readOnly());
    mCopy->setEnabled(!editor->selectionEmpty());
    mPaste->setEnabled(canPaste && editor->canPaste());
    mFindSelection->setEnabled(!editor->selectionEmpty());
  } else if (QTextEdit *editor = qobject_cast<QTextEdit *>(widget)) {
    bool selection = editor->textCursor().hasSelection();
    mCut->setEnabled(selection && !editor->isReadOnly());
    mCopy->setEnabled(selection);
    mPaste->setEnabled(canPaste && !editor->isReadOnly());
    mFindSelection->setEnabled(selection);
  } else if (QLineEdit *editor = qobject_cast<QLineEdit *>(widget)) {
    mCut->setEnabled(editor->hasSelectedText());
    mCopy->setEnabled(editor->hasSelectedText());
    mPaste->setEnabled(canPaste);
    mFindSelection->setEnabled(editor->hasSelectedText());
  } else if (LogView *logView = qobject_cast<LogView *>(widget)) {
    (void)logView; // unused
    mCopy->setEnabled(true);
  }
}

void MenuBar::updateSelectAll() {
  QWidget *widget = QApplication::focusWidget();
  mSelectAll->setEnabled(qobject_cast<TextEditor *>(widget) ||
                         qobject_cast<QTextEdit *>(widget) ||
                         qobject_cast<QLineEdit *>(widget));
}

void MenuBar::updateFind() {
  MainWindow *win = qobject_cast<MainWindow *>(window());
  EditorWindow *editor = qobject_cast<EditorWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  bool empty = FindWidget::text().isEmpty();
  mFind->setEnabled(view || editor);
  mFindNext->setEnabled((view || editor) && !empty);
  mFindPrevious->setEnabled((view || editor) && !empty);
}

void MenuBar::updateView() {
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  mRefresh->setEnabled(view);
  mToggleLog->setEnabled(view);
  mToggleView->setEnabled(view);
  mToggleMaximize->setEnabled(view);

  if (!view)
    return;

  bool diff = (view->viewMode() == RepoView::DoubleTree);
  mToggleLog->setText(view->isLogVisible() ? tr("Hide Log") : tr("Show Log"));
  mToggleView->setText(diff ? tr("Show Tree View")
                            : tr("Show Double Tree View"));
}

void MenuBar::updateRepository() {
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  mConfigureRepository->setEnabled(view);
  mCommit->setEnabled(view && view->isCommitEnabled());
  mStageAll->setEnabled(view && view->isStageEnabled());
  mUnstageAll->setEnabled(view && view->isUnstageEnabled());
  mAmendCommit->setEnabled(view);

  bool lfs = view && view->repo().lfsIsInitialized();
  mLfsUnlock->setEnabled(lfs);
  mLfsInitialize->setEnabled(!lfs);
}

void MenuBar::updateRemote() {
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  mConfigureRemotes->setEnabled(view);
  mFetch->setEnabled(view);
  mFetchAll->setEnabled(view);
  mPull->setEnabled(view && !view->repo().isBare());
  mPush->setEnabled(view);
  mFetchFrom->setEnabled(view);
  mPullFrom->setEnabled(view);
  mPushTo->setEnabled(view);
}

void MenuBar::updateBranch() {
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  mConfigureBranches->setEnabled(view);

  git::Reference ref = view ? view->reference() : git::Reference();
  git::Reference head = view ? view->repo().head() : git::Reference();
  mCheckoutCurrent->setEnabled(ref.isValid() && head.isValid() &&
                               ref.qualifiedName() != head.qualifiedName());
  mCheckout->setEnabled(head.isValid() && !view->repo().isBare());
  mRenameBranch->setEnabled(ref.isLocalBranch());
  mNewBranch->setEnabled(head.isValid());

  mMerge->setEnabled(head.isValid());
  mRebase->setEnabled(head.isValid());
  mSquash->setEnabled(head.isValid());

  bool merging = false;
  QString text = tr("Merge");
  if (view) {
    switch (view->repo().state()) {
      case GIT_REPOSITORY_STATE_MERGE:
        merging = true;
        break;

      case GIT_REPOSITORY_STATE_REVERT:
      case GIT_REPOSITORY_STATE_REVERT_SEQUENCE:
        merging = true;
        text = tr("Revert");
        break;

      case GIT_REPOSITORY_STATE_CHERRYPICK:
      case GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE:
        merging = true;
        text = tr("Cherry-pick");
        break;

      case GIT_REPOSITORY_STATE_REBASE:
      case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
      case GIT_REPOSITORY_STATE_REBASE_MERGE:
        text = tr("Rebase");
        break;
    }
  }

  git::Branch headBranch = head;
  mAbort->setText(tr("Abort %1").arg(text));
  mAbort->setEnabled(headBranch.isValid() && merging);
}

void MenuBar::updateSubmodules() {
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  QList<git::Submodule> submodules =
      view ? view->repo().submodules() : QList<git::Submodule>();

  mConfigureSubmodules->setEnabled(view);
  mUpdateSubmodules->setEnabled(!submodules.isEmpty());
  mInitSubmodules->setEnabled(!submodules.isEmpty());

  // FIXME: This doesn't actually work on Mac.
  mOpenSubmodule->setEnabled(!submodules.isEmpty());
}

void MenuBar::updateStash() {
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  bool stash = view && view->repo().stashRef().isValid();
  mShowStashes->setEnabled(stash);
  mStash->setEnabled(view && view->isWorkingDirectoryDirty());
  mStashPop->setEnabled(stash);
}

void MenuBar::updateHistory() {
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  mPrev->setEnabled(view && view->history()->hasPrev());
  mNext->setEnabled(view && view->history()->hasNext());
}

void MenuBar::updateWindow() {
  MainWindow *win = qobject_cast<MainWindow *>(window());
  // First tab is the welcome tab
  mPrevTab->setEnabled(win && win->count() > 2);
  mNextTab->setEnabled(win && win->count() > 2);
}

QWidget *MenuBar::window() const {
  QWidget *parent = parentWidget();
  return parent ? parent : QApplication::activeWindow();
}

RepoView *MenuBar::view() const {
  return static_cast<MainWindow *>(window())->currentView();
}

QList<RepoView *> MenuBar::views() const {
  MainWindow *win = static_cast<MainWindow *>(window());

  QList<RepoView *> repos;
  for (int i = 0; i < win->count(); i++) {
    auto* view = win->view(i);
    if (view) {
      repos.append(view);
    }
  }
  return repos;
}

void MenuBar::setDebugMenuVisible(bool visible) { sDebugMenuVisible = visible; }
bool MenuBar::isDebugMenuVisible() { return sDebugMenuVisible; }

MenuBar *MenuBar::instance(QWidget *widget) {
#ifdef Q_OS_MAC
  static MenuBar *instance = nullptr;
  if (!instance)
    instance = new MenuBar;
  return instance;
#else
  Q_ASSERT(widget);

  QMainWindow *window = qobject_cast<QMainWindow *>(widget->window());
  if (!window)
    return nullptr;

  if (!qobject_cast<MenuBar *>(window->menuBar()))
    window->setMenuBar(new MenuBar(window));

  return static_cast<MenuBar *>(window->menuBar());
#endif
}

bool MenuBar::isMaximized() { return mToggleMaximize->isActive(); }

void MenuBar::registerActions(QWidget *parent) const {
  parent->addActions(actions());
}
