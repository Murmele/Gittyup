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
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextEdit>

namespace {

void openCloneDialog(CloneDialog::Kind kind)
{
  CloneDialog *dialog = new CloneDialog(kind);
  QObject::connect(dialog, &CloneDialog::accepted, [dialog] {
    if (MainWindow *window = MainWindow::open(dialog->path())) {
      RepoView *view = window->currentView();
      view->addLogEntry(dialog->message(), dialog->messageTitle());
    }
  });

  dialog->open();
}

} // anon. namespace

bool MenuBar::sDebugMenuVisible = false;

MenuBar::MenuBar(QWidget *parent)
  : QMenuBar(parent)
{
  // File
  QMenu *file = addMenu(tr("File"));

  QAction *newFile = file->addAction(tr("New File"));
  newFile->setShortcut(QKeySequence::New);
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
  newWin->setShortcut(tr("Ctrl+Meta+N"));
  connect(newWin, &QAction::triggered, [] {
    MainWindow::open();
  });

  QAction *clone = file->addAction(tr("Clone Repository..."));
  clone->setShortcut(tr("Ctrl+Shift+N"));
  connect(clone, &QAction::triggered, [] {
    openCloneDialog(CloneDialog::Clone);
  });

  QAction *init = file->addAction(tr("Initialize New Repository..."));
  init->setShortcut(tr("Ctrl+Alt+N"));
  connect(init, &QAction::triggered, [] {
    openCloneDialog(CloneDialog::Init);
  });

  file->addSeparator();

  QAction *open = file->addAction(tr("Open Repository..."));
  open->setShortcut(QKeySequence::Open);
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
      connect(action, &QAction::triggered, [repo] {
        MainWindow::open(repo->path());
      });
    }
  });

  file->addSeparator();

  mClose = file->addAction(tr("Close"));
  mClose->setShortcut(QKeySequence::Close);
  connect(mClose, &QAction::triggered, [] {
    QWidget *window = QApplication::activeWindow();
    if (!window)
      return;

    if (MainWindow *win = qobject_cast<MainWindow *>(window)) {
      if (win->count() > 0) {
        win->currentView()->close();
        return;
      }
    }

    window->close();
  });

  mSave = file->addAction(tr("Save"));
  mSave->setShortcut(QKeySequence::Save);
  mSave->setEnabled(false);
  connect(mSave, &QAction::triggered, [this] {
    static_cast<EditorWindow *>(window())->widget()->save();
  });

  file->addSeparator();

#ifndef Q_OS_MAC
  QAction *quit = file->addAction(tr("Exit"));
  quit->setMenuRole(QAction::QuitRole);
  quit->setShortcut(QKeySequence::Quit);
  connect(quit, &QAction::triggered, &QApplication::closeAllWindows);
#endif

  // Edit
  QMenu *edit = addMenu(tr("Edit"));

  mUndo = edit->addAction(tr("Undo"));
  mUndo->setShortcut(QKeySequence::Undo);
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
  mRedo->setShortcut(QKeySequence::Redo);
  connect(mRedo, &QAction::triggered, [this] {
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
  mCut->setShortcut(QKeySequence::Cut);
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
  mCopy->setShortcut(QKeySequence::Copy);
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
  mPaste->setShortcut(QKeySequence::Paste);
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
  mSelectAll->setShortcut(QKeySequence::SelectAll);
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
  mFind->setShortcut(QKeySequence::Find);
  connect(mFind, &QAction::triggered, [] {
    QWidget *widget = QApplication::activeWindow();
    if (MainWindow *window = qobject_cast<MainWindow *>(widget)) {
      window->currentView()->find();
    } else if (EditorWindow *window = qobject_cast<EditorWindow *>(widget)) {
      window->widget()->find();
    }
  });

  mFindNext = edit->addAction(tr("Find Next"));
  mFindNext->setShortcut(QKeySequence::FindNext);
  connect(mFindNext, &QAction::triggered, [] {
    QWidget *widget = QApplication::activeWindow();
    if (MainWindow *window = qobject_cast<MainWindow *>(widget)) {
      window->currentView()->findNext();
    } else if (EditorWindow *window = qobject_cast<EditorWindow *>(widget)) {
      window->widget()->findNext();
    }
  });

  mFindPrevious = edit->addAction(tr("Find Previous"));
  mFindPrevious->setShortcut(QKeySequence::FindPrevious);
  connect(mFindPrevious, &QAction::triggered, [] {
    QWidget *widget = QApplication::activeWindow();
    if (MainWindow *window = qobject_cast<MainWindow *>(widget)) {
      window->currentView()->findPrevious();
    } else if (EditorWindow *window = qobject_cast<EditorWindow *>(widget)) {
      window->widget()->findPrevious();
    }
  });

  mFindSelection = edit->addAction(tr("Use Selection for Find"));
  mFindSelection->setShortcut(tr("Ctrl+E"));
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
  mRefresh->setShortcut(QKeySequence::Refresh);
  connect(mRefresh, &QAction::triggered, [this] {
    view()->refresh();
  });

  viewMenu->addSeparator();

  mToggleLog = viewMenu->addAction(tr("Show Log"));
  connect(mToggleLog, &QAction::triggered, [this] {
    RepoView *view = this->view();
    view->setLogVisible(!view->isLogVisible());
  });

  mToggleMaximize = new StateAction(tr("Normal"), tr("Maximize"), viewMenu);
  viewMenu->addAction(mToggleMaximize);
  mToggleMaximize->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
  connect(mToggleMaximize, &QAction::triggered, [this] {
    bool maximize = mToggleMaximize->isActive();
    RepoView* view = this->view();
    RepoView::DetailSplitterWidgets widget = view->detailSplitterMaximize(maximize);
    assert(!maximize || (maximize == true && widget != RepoView::DetailSplitterWidgets::NotDefined));

    QList<RepoView*> repos = this->views();
    for (auto repo : repos) {
        repo->detailSplitterMaximize(maximize, widget);
    }
  });

  mToggleView = viewMenu->addAction(tr("Show Tree View"));
  connect(mToggleView, &QAction::triggered, [this] {
    RepoView *view = this->view();
    bool diff = (view->viewMode() == RepoView::Diff);
    view->setViewMode(diff ? RepoView::Tree : RepoView::Diff);
  });

  // Repository
  QMenu *repository = addMenu(tr("Repository"));

  mConfigureRepository = repository->addAction(tr("Configure Repository..."));
  mConfigureRepository->setMenuRole(QAction::NoRole);
  connect(mConfigureRepository, &QAction::triggered, [this] {
    view()->configureSettings();
  });

  repository->addSeparator();

  mStageAll = repository->addAction(tr("Stage All"));
  mStageAll->setShortcut(tr("Ctrl++"));
  connect(mStageAll, &QAction::triggered, [this] {
    view()->stage();
  });

  mUnstageAll = repository->addAction(tr("Unstage All"));
  mUnstageAll->setShortcut(tr("Ctrl+-"));
  connect(mUnstageAll, &QAction::triggered, [this] {
    view()->unstage();
  });

  repository->addSeparator();

  mCommit = repository->addAction(tr("Commit"));
  mCommit->setShortcut(tr("Ctrl+Shift+C"));
  connect(mCommit, &QAction::triggered, [this] {
    view()->commit();
  });

  mAmendCommit = repository->addAction(tr("Amend Commit"));
  mAmendCommit->setShortcut(tr("Ctrl+Shift+A"));
  connect(mAmendCommit, &QAction::triggered, [this] {
    view()->amendCommit();
  });

  repository->addSeparator();

  QMenu *lfs = repository->addMenu(tr("Git LFS"));
  mLfsUnlock = lfs->addAction(tr("Remove all locks"));
  connect(mLfsUnlock, &QAction::triggered, [this] {
    view()->lfsSetLocked(view()->repo().lfsLocks().values(), false);
  });

  lfs->addSeparator();

  mLfsInitialize = lfs->addAction(tr("Initialize"));
  connect(mLfsInitialize, &QAction::triggered, [this] {
    view()->lfsInitialize();
  });

  // Remote
  QMenu *remote = addMenu(tr("Remote"));

  mConfigureRemotes = remote->addAction(tr("Configure Remotes..."));
  mConfigureRemotes->setMenuRole(QAction::NoRole);
  connect(mConfigureRemotes, &QAction::triggered, [this] {
    view()->configureSettings(ConfigDialog::Remotes);
  });

  remote->addSeparator();

  mFetch = remote->addAction(tr("Fetch"));
  mFetch->setShortcut(tr("Ctrl+Shift+Alt+F"));
  connect(mFetch, &QAction::triggered, [this] {
    view()->fetch();
  });

  mFetchAll = remote->addAction(tr("Fetch All"));
  mFetchAll->setShortcut(tr("Ctrl+Shift+Alt+A"));
  connect(mFetchAll, &QAction::triggered, [this] {
    view()->fetchAll();
  });

  mFetchFrom = remote->addAction(tr("Fetch From..."));
  mFetchFrom->setShortcut(tr("Ctrl+Shift+F"));
  connect(mFetchFrom, &QAction::triggered, [this] {
    RemoteDialog *dialog = new RemoteDialog(RemoteDialog::Fetch, view());
    dialog->open();
  });

  remote->addSeparator();

  mPull = remote->addAction(tr("Pull"));
  mPull->setShortcut(tr("Ctrl+Shift+Alt+L"));
  connect(mPull, &QAction::triggered, [this] {
    view()->pull();
  });

  mPullFrom = remote->addAction(tr("Pull From..."));
  mPullFrom->setShortcut(tr("Ctrl+Shift+L"));
  connect(mPullFrom, &QAction::triggered, [this] {
    RemoteDialog *dialog = new RemoteDialog(RemoteDialog::Pull, view());
    dialog->open();
  });

  remote->addSeparator();

  mPush = remote->addAction(tr("Push"));
  mPush->setShortcut(tr("Ctrl+Shift+Alt+P"));
  connect(mPush, &QAction::triggered, [this] {
    view()->push();
  });

  mPushTo = remote->addAction(tr("Push To..."));
  mPushTo->setShortcut(tr("Ctrl+Shift+P"));
  connect(mPushTo, &QAction::triggered, [this] {
    RemoteDialog *dialog = new RemoteDialog(RemoteDialog::Push, view());
    dialog->open();
  });

  // Branch
  QMenu *branch = addMenu(tr("Branch"));

  mConfigureBranches = branch->addAction(tr("Configure Branches..."));
  mConfigureBranches->setMenuRole(QAction::NoRole);
  connect(mConfigureBranches, &QAction::triggered, [this] {
    view()->configureSettings(ConfigDialog::Branches);
  });

  mNewBranch = branch->addAction(tr("New Branch..."));
  connect(mNewBranch, &QAction::triggered, [this] {
    view()->promptToCreateBranch();
  });

  branch->addSeparator();

  mCheckoutCurrent = branch->addAction(tr("Checkout Current"));
  mCheckoutCurrent->setShortcut(tr("Ctrl+Shift+Alt+H"));
  connect(mCheckoutCurrent, &QAction::triggered, [this] {
    RepoView *view = this->view();
    git::Reference ref = view->reference();
    Q_ASSERT(ref.qualifiedName() != view->repo().head().qualifiedName());

    // FIXME: Disallow non-local branch?
    view->checkout(ref, !ref.isLocalBranch());
  });

  mCheckout = branch->addAction(tr("Checkout..."));
  mCheckout->setShortcut(tr("Ctrl+Shift+H"));
  connect(mCheckout, &QAction::triggered, [this] {
    view()->promptToCheckout();
  });

  branch->addSeparator();

  mMerge = branch->addAction(tr("Merge..."));
  mMerge->setShortcut(tr("Ctrl+Shift+M"));
  connect(mMerge, &QAction::triggered, [this] {
    RepoView *view = this->view();
    MergeDialog *dialog =
      new MergeDialog(RepoView::Merge, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->open();
  });

  mRebase = branch->addAction(tr("Rebase..."));
  mRebase->setShortcut(tr("Ctrl+Shift+R"));
  connect(mRebase, &QAction::triggered, [this] {
    RepoView *view = this->view();
    MergeDialog *dialog =
      new MergeDialog(RepoView::Rebase, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->open();
  });

  mSquash = branch->addAction(tr("Squash..."));
  mSquash->setShortcut(tr("Ctrl+Shift+Q"));
  connect(mSquash, &QAction::triggered, [this] {
    RepoView *view = this->view();
    MergeDialog *dialog =
      new MergeDialog(RepoView::Squash, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->open();
  });

  branch->addSeparator();

  mAbort = branch->addAction(tr("Abort Merge"));
  mAbort->setShortcut(tr("Ctrl+Shift+A"));
  connect(mAbort, &QAction::triggered, [this] {
    view()->mergeAbort();
  });

  // Submodule
  QMenu *submodule = addMenu(tr("Submodule"));

  mConfigureSubmodules = submodule->addAction(tr("Configure Submodules..."));
  mConfigureSubmodules->setMenuRole(QAction::NoRole);
  connect(mConfigureSubmodules, &QAction::triggered, [this] {
    view()->configureSettings(ConfigDialog::Submodules);
  });

  submodule->addSeparator();

  mUpdateSubmodules = submodule->addAction(tr("Update All"));
  mUpdateSubmodules->setShortcut(tr("Ctrl+Shift+Alt+U"));
  connect(mUpdateSubmodules, &QAction::triggered, [this] {
    view()->updateSubmodules();
  });

  mInitSubmodules = submodule->addAction(tr("Update..."));
  mInitSubmodules->setShortcut(tr("Ctrl+Shift+U"));
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
    foreach (const git::Submodule &submodule, view->repo().submodules()) {
      QAction *action = mOpenSubmodule->addAction(submodule.name());
      connect(action, &QAction::triggered, [view, submodule] {
        view->openSubmodule(submodule);
      });
    }
  });

  // Stash
  QMenu *stash = addMenu(tr("Stash"));

  mShowStashes = stash->addAction(tr("Show Stashes"));
  connect(mShowStashes, &QAction::triggered, [this] {
    RepoView *view = this->view();
    view->selectReference(view->repo().stashRef());
  });

  stash->addSeparator();

  mStash = stash->addAction(tr("Stash..."));
  mStash->setShortcut(tr("Ctrl+Shift+T"));
  connect(mStash, &QAction::triggered, [this] {
    view()->promptToStash();
  });

  mStashPop = stash->addAction(tr("Pop Stash"));
  mStashPop->setShortcut(tr("Ctrl+Shift+Alt+T"));
  connect(mStashPop, &QAction::triggered, [this] {
    view()->popStash();
  });

  // History
  QMenu *historyMenu = addMenu(tr("History"));

  mPrev = historyMenu->addAction(tr("Back"));
  mPrev->setShortcut(QKeySequence::Back);
  mPrev->setEnabled(false);
  connect(mPrev, &QAction::triggered, [this] {
    view()->history()->prev();
  });

  mNext = historyMenu->addAction(tr("Forward"));
  mNext->setShortcut(QKeySequence::Forward);
  mNext->setEnabled(false);
  connect(mNext, &QAction::triggered, [this] {
    view()->history()->next();
  });

  // Window
  QMenu *windowMenu = addMenu(tr("Window"));
  mPrevTab = windowMenu->addAction(tr("Show Previous Tab"));
  mPrevTab->setShortcut(QKeySequence::PreviousChild);
  connect(mPrevTab, &QAction::triggered, [this] {
    MainWindow *win = static_cast<MainWindow *>(window());
    TabWidget *tabs = win->tabWidget();
    int index = tabs->currentIndex();
    tabs->setCurrentIndex((index ? index : tabs->count()) - 1);
  });

  mNextTab = windowMenu->addAction(tr("Show Next Tab"));
  mNextTab->setShortcut(QKeySequence::NextChild);
  connect(mNextTab, &QAction::triggered, [this] {
    MainWindow *win = static_cast<MainWindow *>(window());
    TabWidget *tabs = win->tabWidget();
    int index = tabs->currentIndex();
    tabs->setCurrentIndex((index == tabs->count() - 1) ? 0 : index + 1);
  });

  windowMenu->addSeparator();

  QAction *chooser = windowMenu->addAction(tr("Show Repository Chooser..."));
  chooser->setShortcut(tr("Ctrl+Shift+O"));
  connect(chooser, &QAction::triggered, &StartDialog::openSharedInstance);

  // Tools
  QMenu *tools = addMenu(tr("Tools"));
  QAction *preferences = tools->addAction(tr("Options..."));
  preferences->setMenuRole(QAction::PreferencesRole);
  connect(preferences, &QAction::triggered, [] {
    SettingsDialog::openSharedInstance();
  });

  // Help
  QMenu *help = addMenu(tr("Help"));
  QString name = QCoreApplication::applicationName();
  QAction *about = help->addAction(tr("About %1").arg(name));
  about->setMenuRole(QAction::AboutRole);
  connect(about, &QAction::triggered, [] {
    AboutDialog::openSharedInstance();
  });

  QAction *update = help->addAction(tr("Check For Updates..."));
  update->setMenuRole(QAction::ApplicationSpecificRole);
  connect(update, &QAction::triggered, Updater::instance(), &Updater::update);

  QAction *plugin = help->addAction(tr("Plugin Documentation..."));
  connect(plugin, &QAction::triggered, [] {
    QString url = Settings::docDir().filePath("plugin.html");
    QDesktopServices::openUrl(QUrl::fromLocalFile(url));
  });

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
    connect(indexer, &QAction::triggered, [](bool checked) {
      Index::setLoggingEnabled(checked);
    });

    QAction *cred = debug->addAction(tr("Log Credential Helper"));
    cred->setCheckable(true);
    cred->setChecked(CredentialHelper::isLoggingEnabled());
    connect(cred, &QAction::triggered, [](bool checked) {
      CredentialHelper::setLoggingEnabled(checked);
    });

    QAction *remote = debug->addAction(tr("Log Remote Connection"));
    remote->setCheckable(true);
    remote->setChecked(git::Remote::isLoggingEnabled());
    connect(remote, &QAction::triggered, [](bool checked) {
      git::Remote::setLoggingEnabled(checked);
    });

    debug->addSeparator();

    QAction *diffs = debug->addAction(tr("Load All Diffs"));
    connect(diffs, &QAction::triggered, [this] {
      if (MainWindow *win = qobject_cast<MainWindow *>(window())) {
        RepoView *view = win->currentView();
        CommitList *commits = view->commitList();
        QAbstractItemModel *model = commits->model();
        for (int i = 0; i < model->rowCount(); ++i) {
          commits->setCurrentIndex(model->index(i, 0));
          view->find(); // Force editors to load.
          QCoreApplication::processEvents();
        }
      }
    });

    QAction *walk = debug->addAction(tr("Walk Commits"));
    connect(walk, &QAction::triggered, [this] {
      if (MainWindow *win = qobject_cast<MainWindow *>(window())) {
        git::RevWalk walker = win->currentView()->repo().walker();
        while (git::Commit commit = walker.next())
          (void) commit;
      }
    });
  }

  // Respond to window activation.
  connect(qApp, &QApplication::focusChanged, this, &MenuBar::update);
}

void MenuBar::update()
{
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

void MenuBar::updateFile()
{
  mClose->setEnabled(QApplication::activeWindow());
}

void MenuBar::updateSave()
{
  EditorWindow *win = qobject_cast<EditorWindow *>(window());
  mSave->setEnabled(win && win->widget()->editor()->isModified());
}

void MenuBar::updateUndoRedo()
{
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

void MenuBar::updateCutCopyPaste()
{
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
  } else if (LogView *logView = qobject_cast<LogView *>(widget)){
    mCopy->setEnabled(true);
  }
}

void MenuBar::updateSelectAll()
{
  QWidget *widget = QApplication::focusWidget();
  mSelectAll->setEnabled(
    qobject_cast<TextEditor *>(widget) ||
    qobject_cast<QTextEdit *>(widget) ||
    qobject_cast<QLineEdit *>(widget));
}

void MenuBar::updateFind()
{
  MainWindow *win = qobject_cast<MainWindow *>(window());
  EditorWindow *editor = qobject_cast<EditorWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  bool empty = FindWidget::text().isEmpty();
  mFind->setEnabled(view || editor);
  mFindNext->setEnabled((view || editor) && !empty);
  mFindPrevious->setEnabled((view || editor) && !empty);
}

void MenuBar::updateView()
{
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  mRefresh->setEnabled(view);
  mToggleLog->setEnabled(view);
  mToggleView->setEnabled(view);
  mToggleMaximize->setEnabled(view);

  if (!view)
    return;

  bool diff = (view->viewMode() == RepoView::Diff);
  mToggleLog->setText(view->isLogVisible() ? tr("Hide Log") : tr("Show Log"));
  mToggleView->setText(diff ? tr("Show Tree View") : tr("Show Diff View"));
}

void MenuBar::updateRepository()
{
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

void MenuBar::updateRemote()
{
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

void MenuBar::updateBranch()
{
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  mConfigureBranches->setEnabled(view);

  git::Reference ref = view ? view->reference() : git::Reference();
  git::Reference head = view ? view->repo().head() : git::Reference();
  mCheckoutCurrent->setEnabled(
    ref.isValid() && head.isValid() &&
    ref.qualifiedName() != head.qualifiedName());
  mCheckout->setEnabled(head.isValid() && !view->repo().isBare());
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

void MenuBar::updateSubmodules()
{
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

void MenuBar::updateStash()
{
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  bool stash = view && view->repo().stashRef().isValid();
  mShowStashes->setEnabled(stash);
  mStash->setEnabled(view && view->isWorkingDirectoryDirty());
  mStashPop->setEnabled(stash);
}

void MenuBar::updateHistory()
{
  MainWindow *win = qobject_cast<MainWindow *>(window());
  RepoView *view = win ? win->currentView() : nullptr;
  mPrev->setEnabled(view && view->history()->hasPrev());
  mNext->setEnabled(view && view->history()->hasNext());
}

void MenuBar::updateWindow()
{
  MainWindow *win = qobject_cast<MainWindow *>(window());
  mPrevTab->setEnabled(win && win->count() > 1);
  mNextTab->setEnabled(win && win->count() > 1);
}

QWidget *MenuBar::window() const
{
  QWidget *parent = parentWidget();
  return parent ? parent : QApplication::activeWindow();
}

RepoView *MenuBar::view() const
{
  return static_cast<MainWindow *>(window())->currentView();
}

QList<RepoView*> MenuBar::views() const
{
   MainWindow* win = static_cast<MainWindow *>(window());

   QList<RepoView*> repos;
   for (int i=0; i< win->count(); i++) {
       repos.append(win->view(i));
   }
   return repos;
}

void MenuBar::setDebugMenuVisible(bool visible)
{
  sDebugMenuVisible = visible;
}

MenuBar *MenuBar::instance(QWidget *widget)
{
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

bool MenuBar::isMaximized()
{
    return mToggleMaximize->isActive();
}
