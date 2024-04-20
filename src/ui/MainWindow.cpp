//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "MainWindow.h"
#include "AdvancedSearchWidget.h"
#include "IndexCompleter.h"
#include "MenuBar.h"
#include "RepoView.h"
#include "SearchField.h"
#include "SideBar.h"
#include "TabWidget.h"
#include "ToolBar.h"
#include "conf/RecentRepositories.h"
#include "conf/Settings.h"
#include "git/Repository.h"
#include "git/Config.h"
#include "git/Submodule.h"
#include "qmap.h"
#include <QApplication>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QCryptographicHash>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QTimeLine>
#include <QToolButton>
#include "util/Debug.h"

namespace {

const int kDefaultWidth = 1200;
const int kDefaultHeight = 800;

const QString kPathKey = "path";
const QString kIndexKey = "index";
const QString kStateKey = "state";
const QString kActiveKey = "active";
const QString kSidebarKey = "sidebar";
const QString kGeometryKey = "geometry";
const QString kWindowsGroup = "windows";

class TabName {
public:
  TabName(const QString &path) : mPath(path) {}

  QString name() const { return mPath.section('/', -mSections); }

  void increment() { ++mSections; }
  int sections() const { return mSections; }
  void setSections(int sections) { mSections = sections; }

private:
  QString mPath;
  int mSections = 1;
};

} // namespace

bool MainWindow::sSaveWindowSettings = false;

MainWindow::MainWindow(const git::Repository &repo, QWidget *parent,
                       Qt::WindowFlags flags)
    : QMainWindow(parent, flags) {
  setAttribute(Qt::WA_DeleteOnClose);
  setUnifiedTitleAndToolBarOnMac(true);
  setAcceptDrops(true);

  // Create new menu bar for this window if there isn't a shared one.
  mMenuBar = MenuBar::instance(this);
  mMenuBar->registerActions(this);

  // Create tool bar.
  mToolBar = new ToolBar(this);
  addToolBar(Qt::TopToolBarArea, mToolBar);

  // Initialize search.
  SearchField *searchField = mToolBar->searchField();
  connect(searchField, &QLineEdit::textEdited, mMenuBar,
          &MenuBar::updateUndoRedo);
  connect(searchField, &QLineEdit::selectionChanged, mMenuBar,
          &MenuBar::updateCutCopyPaste);

  // Hook up advanced search.
  AdvancedSearchWidget *advancedSearch = new AdvancedSearchWidget(this);
  connect(advancedSearch, &AdvancedSearchWidget::accepted, searchField,
          &QLineEdit::setText);
  connect(searchField->advancedButton(), &QToolButton::clicked, this,
          [this, searchField, advancedSearch] {
            advancedSearch->exec(searchField, currentView()->index());
          });

  // Update title and refresh when settings change.
  mFullPath =
      Settings::instance()->value(Setting::Id::ShowFullRepoPath).toBool();
  connect(Settings::instance(), &Settings::settingsChanged, this,
          [this](bool refresh) {
            Settings *settings = Settings::instance();

            bool menuBarHidden =
                settings->value(Setting::Id::HideMenuBar).toBool();
            if (mMenuBar->isHidden() != menuBarHidden)
              mMenuBar->setHidden(menuBarHidden);

            bool fullPath =
                settings->value(Setting::Id::ShowFullRepoPath).toBool();
            if (mFullPath != fullPath) {
              mFullPath = fullPath;
              updateWindowTitle();
            }

            if (refresh) {
              for (int i = 0; i < count(); ++i)
                view(i)->refresh();
            }
          });

  // Create splitter.
  QSplitter *splitter = new QSplitter(this);
  splitter->setHandleWidth(0);
  connect(splitter, &QSplitter::splitterMoved, [this] {
    QSplitter *splitter = static_cast<QSplitter *>(centralWidget());
    mIsSideBarVisible = (splitter->sizes().first() > 0);
  });

  // Create tab container.
  TabWidget *tabs = new TabWidget(splitter);
  connect(tabs, &TabWidget::currentChanged, [this](int index) {
    updateInterface();
    MenuBar::instance(this)->update();
  });

  connect(tabs, QOverload<>::of(&TabWidget::tabInserted), this,
          &MainWindow::updateTabNames);
  connect(tabs, QOverload<>::of(&TabWidget::tabRemoved), this,
          &MainWindow::updateTabNames);

  splitter->addWidget(new SideBar(tabs, splitter));
  splitter->addWidget(tabs);
  splitter->setCollapsible(1, false);
  splitter->setStretchFactor(1, 1);

  setCentralWidget(splitter);

  if (repo)
    addTab(repo);

  // Set search completer.
  searchField->setCompleter(new IndexCompleter(this, searchField));

  // Set default size and position.
  resize(kDefaultWidth, kDefaultHeight);

  QRect desktop = QGuiApplication::primaryScreen()->availableGeometry();
  int x = (desktop.width() / 2) - (kDefaultWidth / 2);
  int y = (desktop.height() / 2) - (kDefaultHeight / 2);
  move(x, y);

  // Position with respect to existing windows.
  if (MainWindow *win = activeWindow())
    move(win->x() + 24, win->y() + 24);

  // Restore sidebar.
  setSideBarVisible(QSettings().value(kSidebarKey, true).toBool());

  // Set initial state of interface.
  updateInterface();
}

bool MainWindow::isSideBarVisible() const { return mIsSideBarVisible; }

void MainWindow::setSideBarVisible(bool visible) {
  if (visible == mIsSideBarVisible)
    return;

  mIsSideBarVisible = visible;

  // Remember in settings.
  QSettings().setValue(kSidebarKey, visible);

  // Animate sidebar sliding in or out.
  QSplitter *splitter = static_cast<QSplitter *>(centralWidget());
  QWidget *sidebar = splitter->widget(0);
  int pos = visible ? sidebar->sizeHint().width() : splitter->sizes().first();

  QTimeLine *timeline = new QTimeLine(250, this);
  timeline->setDirection(visible ? QTimeLine::Forward : QTimeLine::Backward);
  timeline->setEasingCurve(QEasingCurve(QEasingCurve::Linear));
  timeline->setUpdateInterval(20);

  connect(timeline, &QTimeLine::valueChanged, [this, pos](qreal value) {
    QSplitter *splitter = static_cast<QSplitter *>(centralWidget());
    splitter->setSizes({static_cast<int>(pos * value), 1});
  });

  connect(timeline, &QTimeLine::finished,
          [timeline] { timeline->deleteLater(); });

  timeline->start();
}

TabWidget *MainWindow::tabWidget() const {
  QSplitter *splitter = static_cast<QSplitter *>(centralWidget());
  return static_cast<TabWidget *>(splitter->widget(1));
}

RepoView *MainWindow::addTab(const QString &path) {
  if (path.isEmpty())
    return nullptr;

  TabWidget *tabs = tabWidget();
  for (int i = 0; i < tabs->count(); i++) {
    RepoView *view = static_cast<RepoView *>(tabs->widget(i));
    if (path == view->repo().dir(false).path()) {
      tabs->setCurrentIndex(i);
      return view;
    }
  }

  git::Repository repo = git::Repository::open(path, true);
  if (!repo.isValid()) {
    warnInvalidRepo(path);
    return nullptr;
  }

  return addTab(repo);
}

RepoView *MainWindow::addTab(const git::Repository &repo) {
  // Update recent repository settings.
  QDir dir = repo.dir(false);
  RecentRepositories::instance()->add(dir.path());

  TabWidget *tabs = tabWidget();
  for (int i = 0; i < tabs->count(); i++) {
    RepoView *view = static_cast<RepoView *>(tabs->widget(i));
    if (dir.path() == view->repo().dir(false).path()) {
      tabs->setCurrentIndex(i);
      return view;
    }
  }

  RepoView *view = new RepoView(repo, this);
  view->detailSplitterMaximize(mMenuBar->isMaximized());
  git::RepositoryNotifier *notifier = repo.notifier();
  connect(notifier, &git::RepositoryNotifier::referenceUpdated, this,
          &MainWindow::updateInterface);
  connect(notifier, &git::RepositoryNotifier::stateChanged, this,
          [this] { updateWindowTitle(); });

  emit tabs->tabAboutToBeInserted();
  tabs->setCurrentIndex(tabs->addTab(view, dir.dirName()));

  Settings *settings = Settings::instance();
  bool enable =
      settings->value(Setting::Id::UpdateSubmodulesAfterPullAndClone).toBool();
  if (repo.appConfig().value<bool>("autoupdate.enable", enable)) {
    // update submodules
    view->updateSubmodules(repo.submodules(), true, true, false, nullptr);
  }

  // Start status diff.
  view->refresh(false);
  return view;
}

int MainWindow::count() const { return tabWidget()->count(); }

RepoView *MainWindow::currentView() const {
  return static_cast<RepoView *>(tabWidget()->currentWidget());
}

RepoView *MainWindow::view(int index) const {
  return static_cast<RepoView *>(tabWidget()->widget(index));
}

MainWindow *MainWindow::activeWindow() {
  QWidget *win = QApplication::activeWindow();
  if (MainWindow *mainWin = qobject_cast<MainWindow *>(win))
    return mainWin;

  QList<MainWindow *> mainWins = windows();
  return !mainWins.isEmpty() ? mainWins.first() : nullptr;
}

QList<MainWindow *> MainWindow::windows() {
  QList<MainWindow *> mainWins;
  foreach (QWidget *win, QApplication::topLevelWidgets()) {
    if (MainWindow *mainWin = qobject_cast<MainWindow *>(win))
      mainWins.append(mainWin);
  }

  return mainWins;
}

bool MainWindow::restoreWindows() {
  QList<MainWindow *> windows;

  // Open windows.
  QSettings settings;
  settings.beginGroup(kWindowsGroup);
  foreach (const QString &group, settings.childGroups()) {
    settings.beginGroup(group);
    int index = settings.value(kIndexKey).toInt();
    bool active = settings.value(kActiveKey).toBool();
    QStringList paths = settings.value(kPathKey).toStringList();
    QByteArray state = settings.value(kStateKey).toByteArray();
    QByteArray geometry = settings.value(kGeometryKey).toByteArray();
    settings.endGroup();

    // This shouldn't ever happen.
    if (paths.isEmpty())
      continue;

    // Open a window new for the first valid repo.
    MainWindow *window = open(paths.takeFirst());
    while (!window && !paths.isEmpty())
      window = open(paths.takeFirst());

    if (!window)
      continue;

    // Add the remainder as tabs.
    foreach (const QString &path, paths)
      window->addTab(path);

    // Select saved index.
    window->tabWidget()->setCurrentIndex(index);

    // Restore state and geometry.
    window->restoreState(state);
    window->restoreGeometry(geometry);

    // Order active window first.
    windows.insert(active ? 0 : windows.size(), window);
  }
  settings.endGroup();

  // Remove all window settings.
  settings.remove(kWindowsGroup);

  // Activate the top window.
  if (!windows.isEmpty()) {
    MainWindow *window = windows.first();
    window->raise();
    window->activateWindow();
  }

  return !windows.isEmpty();
}

MainWindow *MainWindow::open(const QString &path, bool warnOnInvalid) {
  DebugRefresh("Open project: " << path);
  if (path.isEmpty())
    return nullptr;

  git::Repository repo = git::Repository::open(path, true);
  if (!repo.isValid()) {
    if (warnOnInvalid)
      warnInvalidRepo(path);
    return nullptr;
  }

  if (Settings::instance()->value(Setting::Id::OpenAllReposInTabs).toBool()) {
    if (MainWindow *win = activeWindow()) {
      win->addTab(repo);
      return win;
    }
  }

  return open(repo);
}

MainWindow *MainWindow::open(const git::Repository &repo) {
  // Update recent repository settings.
  if (repo.isValid())
    RecentRepositories::instance()->add(repo.dir(false).path());

  // Create the window.
  MainWindow *window = new MainWindow(repo);

  const bool showMaximized =
      Settings::instance()->value(Setting::Id::ShowMaximized).toBool();

  if (showMaximized) {
    window->showMaximized();
  } else {
    window->show();
  }

  return window;
}

void MainWindow::setSaveWindowSettings(bool enabled) {
  sSaveWindowSettings = enabled;
}

void MainWindow::showEvent(QShowEvent *event) {
  QMainWindow::showEvent(event);

  if (mShown)
    return;

  mShown = true;
  updateInterface();
}

void MainWindow::closeEvent(QCloseEvent *event) {
  // FIXME: Attempt to close windows before writing settings?

  if (sSaveWindowSettings) {
    // Store window state.
    // FIXME: Qt doesn't impose a predictable order on top-level windows.
    // Instead order the active window first and leave others undefined.
    QSettings settings;
    settings.beginGroup(kWindowsGroup);
    settings.beginGroup(windowGroup());
    settings.setValue(kPathKey, paths());
    settings.setValue(kIndexKey, tabWidget()->currentIndex());
    settings.setValue(kActiveKey, this == activeWindow());
    settings.setValue(kStateKey, saveState());
    settings.setValue(kGeometryKey, saveGeometry());
    settings.endGroup();
    settings.endGroup();
  }

  for (int i = 0; i < count(); ++i) {
    if (!view(i)->close()) {
      event->ignore();
      return;
    }
  }

  mClosing = true;
  QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  if (!event->mimeData()->hasFormat("text/uri-list"))
    return;

  foreach (const QUrl &url, event->mimeData()->urls()) {
    if (!url.isLocalFile())
      return;

    QDir dir(url.toLocalFile());
    if (!dir.exists())
      return;

    if (!git::Repository::open(dir.path(), true).isValid())
      return;
  }

  event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
  foreach (const QUrl &url, event->mimeData()->urls())
    addTab(url.toLocalFile());
}

void MainWindow::warnInvalidRepo(const QString &path) {
  QString title = tr("Invalid Git Repository");
  QString text = tr("%1 does not contain a valid git repository.");
  QMessageBox::warning(nullptr, title, text.arg(path));
}

void MainWindow::updateTabNames() {
  TabWidget *tabs = tabWidget();
  QHash<QString, QList<int>> names;
  QList<TabName> fullNames;

  for (int i = 0; i < count(); ++i) {
    TabName name(view(i)->repo().dir(false).path());
    names[name.name()].append(i);
    fullNames.append(name);
  }

  QHash<QString, QList<int>>::key_iterator first;
  while ((first = names.keyBegin()) != names.keyEnd()) {
    auto key = *first;
    auto ids = names.take(key);

    if (ids.count() == 1) {
      tabs->setTabText(ids.first(), key);
    } else {
      for (auto id : ids) {
        auto &name = fullNames[id];
        name.increment();
        names[name.name()].append(id);
      }
    }
  }
}

void MainWindow::updateInterface() {
  // Avoid updating during close.
  if (mClosing)
    return;

  int ahead = 0;
  int behind = 0;
  if (RepoView *view = currentView()) {
    if (git::Branch head = view->repo().head()) {
      if (git::Branch upstream = head.upstream()) {
        ahead = head.difference(upstream);
        behind = upstream.difference(head);
      }
    }
  }

  updateWindowTitle(ahead, behind);
  mToolBar->updateButtons(ahead, behind);
}

void MainWindow::updateWindowTitle(int ahead, int behind) {
  RepoView *view = currentView();
  if (!view) {
    setWindowTitle(QCoreApplication::applicationName() + BUILD_DESCRIPTION);
    return;
  }

  git::Repository repo = view->repo();
  QDir dir = repo.dir(false);
  git::Reference head = repo.head();
  QString path = mFullPath ? dir.path() : dir.dirName();
  QString name = head.isValid() ? head.name() : repo.unbornHeadName();
  QString title = tr("%1 - %2").arg(path, name);

  // Add remote tracking information.
  if (git::Branch branch = head) {
    if (git::Branch upstream = branch.upstream()) {
      if (ahead < 0)
        ahead = branch.difference(upstream);
      if (behind < 0)
        behind = upstream.difference(branch);

      QStringList parts;
      if (ahead > 0)
        parts.append(tr("ahead: %1").arg(ahead));
      if (behind > 0)
        parts.append(tr("behind: %1").arg(behind));

      QString status = parts.isEmpty() ? tr("up-to-date") : parts.join(", ");
      QString remote = tr("%1 (%2)").arg(status, upstream.name());
      title = tr("%1 - %2").arg(title, remote);
    }
  }

  // Add state.
  QString state;
  switch (repo.state()) {
    case GIT_REPOSITORY_STATE_MERGE:
      state = tr("MERGING");
      break;

    case GIT_REPOSITORY_STATE_REVERT:
    case GIT_REPOSITORY_STATE_REVERT_SEQUENCE:
      state = tr("REVERTING");
      break;

    case GIT_REPOSITORY_STATE_CHERRYPICK:
    case GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE:
      state = tr("CHERRY-PICKING");
      break;

    case GIT_REPOSITORY_STATE_BISECT:
      break; // FIXME?

    case GIT_REPOSITORY_STATE_REBASE:
    case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
    case GIT_REPOSITORY_STATE_REBASE_MERGE:
      state = tr("REBASING");
      break;

    case GIT_REPOSITORY_STATE_APPLY_MAILBOX:
    case GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE:
      break; // FIXME?
  }

  if (!state.isEmpty())
    title = tr("%1 (%2)").arg(title, state);

  setWindowTitle(QString("%1%2").arg(title, BUILD_DESCRIPTION));
}

QStringList MainWindow::paths() const {
  QStringList paths;
  for (int i = 0; i < count(); ++i)
    paths.append(view(i)->repo().dir(false).path());
  return paths;
}

QString MainWindow::windowGroup() const {
  QByteArray group = paths().join(';').toUtf8();
  QByteArray hash = QCryptographicHash::hash(group, QCryptographicHash::Md5);
  return QString::fromUtf8(hash.toHex());
}
