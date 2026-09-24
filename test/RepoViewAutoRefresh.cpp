//
//          Copyright (c) 2026, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
//

#include "Test.h"

#include "ui/CommitList.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "watcher/RepositoryWatcher.h"

#include <memory>
#include <QSignalSpy>

using namespace Test;

namespace {

const int kDebounceMs = 300;

// Long enough for the watcher to arm its timer, short of the debounce.
const int kArmMs = 100;

// Late notifications from earlier changes must land before the spy is cleared.
const int kSettleMs = 2 * kDebounceMs;

// Long enough for a spurious notification to show up.
const int kQuietMs = 3 * kDebounceMs;

// Timeouts are generous so a starved CI runner can't cause false failures.
const int kAttemptMs = 1500;
const int kAttempts = 10;
const int kSignalMs = 10000;

bool writeFile(const QString &path) {
  static int counter = 0;
  QFile file(path);
  if (!file.open(QFile::WriteOnly | QFile::Truncate))
    return false;

  file.write(QByteArray::number(++counter));
  return true;
}

RepositoryWatcher *findWatcher(RepoView *view) {
  for (QObject *child : view->children()) {
    if (auto *watcher = dynamic_cast<RepositoryWatcher *>(child))
      return watcher;
  }

  return nullptr;
}

} // namespace

class TestRepoViewAutoRefresh : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void changeSurvivesStatusFinishing();
  void refreshDoesNotRetrigger();
  void cleanupTestCase();

private:
  void settle();

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  RepoView *mView = nullptr;
  std::unique_ptr<QSignalSpy> mSpy;
};

void TestRepoViewAutoRefresh::settle() {
  QTest::qWait(kSettleMs);
  mSpy->clear();
}

void TestRepoViewAutoRefresh::initTestCase() {
  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(QTest::qWaitForWindowActive(mWindow));
  mView = mWindow->currentView();

  RepositoryWatcher *watcher = findWatcher(mView);
  QVERIFY(watcher);
  watcher->setDebounceInterval(kDebounceMs);

  mSpy = std::make_unique<QSignalSpy>(mRepo->notifier(),
                                      &git::RepositoryNotifier::workdirChanged);

  // The watcher installs its watches on a thread, so retry until it reports.
  bool watching = false;
  for (int i = 0; i < kAttempts && !watching; ++i) {
    QVERIFY(writeFile(mRepo->workdir().filePath("canary")));
    watching = mSpy->wait(kAttemptMs);
  }

  QVERIFY2(watching, "watcher never reported a root-level change");
  settle();
}

void TestRepoViewAutoRefresh::changeSurvivesStatusFinishing() {
  QVERIFY(writeFile(mRepo->workdir().filePath("race")));
  QTest::qWait(kArmMs);

  // A finishing status check must not swallow a change made while it ran.
  auto *commits = mView->findChild<CommitList *>();
  QVERIFY(commits);
  emit commits->statusChanged(true);

  QVERIFY2(mSpy->wait(kSignalMs), "pending change was cancelled");
}

void TestRepoViewAutoRefresh::refreshDoesNotRetrigger() {
  settle();
  refresh(mView);

  mSpy->clear();
  QVERIFY2(!mSpy->wait(kQuietMs), "a refresh triggered another refresh");
}

void TestRepoViewAutoRefresh::cleanupTestCase() {
  mSpy.reset();
  mWindow->close();

  // Delete the window now so its watcher stops before the repository goes.
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

TEST_MAIN(TestRepoViewAutoRefresh)
#include "RepoViewAutoRefresh.moc"
