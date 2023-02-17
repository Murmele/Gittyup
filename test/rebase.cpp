//
//          Copyright (c) 2022, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "Test.h"
#include "ui/RepoView.h"
#include "ui/DetailView.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/DiffView/DiffView.h"

#include "ui/MainWindow.h"
#include "ui/DetailView.h"
#include "ui/DiffView/FileWidget.h"
#include "ui/DiffView/HunkWidget.h"

#include "git/Reference.h"
#include "git/Diff.h"
#include "git/Commit.h"
#include "git/Tree.h"
#include "git/Patch.h"
#include "git/Command.h"

#include "log/LogEntry.h"

#include <QStackedWidget>
#include <QTextEdit>
#include <QPushButton>

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  mRepo = git::Repository::open(path);                                         \
  QVERIFY(mRepo.isValid());                                                    \
  Test::initRepo(mRepo);                                                       \
  MainWindow window(mRepo);                                                    \
  window.show();                                                               \
  QVERIFY(QTest::qWaitForWindowExposed(&window));                              \
                                                                               \
  git::Reference head = mRepo.head();                                          \
  git::Commit commit = head.target();                                          \
  git::Diff stagedDiff = mRepo.diffTreeToIndex(commit.tree()); /* correct */   \
                                                                               \
  RepoView *repoView = window.currentView();                                   \
  auto diff = mRepo.status(mRepo.index(), nullptr, false);

#ifndef GIT_EXECUTABLE
#error                                                                         \
    "To execute those tests it is neccessary to have git installed on your computer. Turn off tests or exclude this test to build the project"
#endif

#define EXECUTE_GIT_COMMAND(workdir, arguments, expectedExitCode)              \
  {                                                                            \
    QProcess p(this);                                                          \
    p.setWorkingDirectory(workdir);                                            \
    QString bash = git::Command::bashPath();                                   \
    QVERIFY(!bash.isEmpty());                                                  \
    QString command = QString(GIT_EXECUTABLE) + " " + arguments;               \
    QStringList a = {"-c", command};                                           \
    p.start(bash, a);                                                          \
    p.waitForStarted();                                                        \
    QCOMPARE(p.waitForFinished(), true);                                       \
    QCOMPARE(p.exitCode(), expectedExitCode);                                  \
  }

using namespace git;

class TestRebase : public QObject {
  Q_OBJECT

private slots:
  void withoutConflicts();
  void conflictingRebase();
  void continueExternalStartedRebase(); // must have conflicts otherwise it is
                                        // not possible to continue
  void startRebaseContinueInCLI();
  void startRebaseContinueInCLIContinueGUI(); // start rebase, continue in the
                                              // cli and finish in the GUI
  void abortMR();
  void commitDuringRebase();

private:
  git::Repository mRepo;
};

//###################################################################################################
//###################################################################################################
//###################################################################################################
// Tests starting
// ######################################################################################
//###################################################################################################
//###################################################################################################
//###################################################################################################

void TestRebase::withoutConflicts() {
  INIT_REPO("rebaseConflicts.zip", true);

  int rebaseFinished = 0;
  int rebaseAboutToRebase = 0;
  int rebaseCommitSuccess = 0;
  int rebaseConflict = 0;

  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseInitError,
          [=]() { QVERIFY(false); }); // Should not be called
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseAboutToRebase,
          [=, &rebaseAboutToRebase](const Rebase rebase, const Commit before,
                                    int count) {
            QVERIFY(rebase.isValid());
            QCOMPARE(count, 1);
            QCOMPARE(before.message(), "File2.txt added");
            rebaseAboutToRebase++;
          });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitInvalid,
          [=]() { QVERIFY(false); }); // Should not be called
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseFinished,
          [=, &rebaseFinished]() { rebaseFinished++; });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitSuccess,
          [=, &rebaseCommitSuccess](const Rebase rebase, const Commit before,
                                    const Commit after, int counter) {
            QVERIFY(rebase.isValid());
            rebaseCommitSuccess++;
          });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseConflict,
          [=, &rebaseConflict]() { rebaseConflict++; });

  const QString rebaseBranchName = "refs/heads/noConflict";

  git::Reference branch = mRepo.lookupRef(rebaseBranchName);
  QVERIFY(branch.isValid());
  auto c = branch.annotatedCommit().commit();

  LogEntry *entry = repoView->addLogEntry("Rebase", "Rebase", nullptr);

  // Checkout correct branch
  QCOMPARE(mRepo.checkout(c), true);

  // Rebase on main
  git::Reference mainBranch = mRepo.lookupRef(QString("refs/heads/main"));
  QVERIFY(mainBranch.isValid());
  auto ac = mainBranch.annotatedCommit();
  repoView->rebase(ac, entry);

  // Check that branch is based on "main" now
  branch = mRepo.lookupRef(rebaseBranchName);
  QVERIFY(branch.isValid());
  QList<Commit> parents = branch.annotatedCommit().commit().parents();
  QCOMPARE(parents.count(), 1);
  QCOMPARE(parents.at(0).id(), ac.commit().id());

  // Check that rebase was really finished
  QCOMPARE(mRepo.rebaseOngoing(), false);

  // Check call counters
  QCOMPARE(rebaseFinished, 1);
  QCOMPARE(rebaseAboutToRebase, 1);
  QCOMPARE(rebaseCommitSuccess, 1);
  QCOMPARE(rebaseConflict, 0);

  auto *detailview = repoView->findChild<DetailView *>();
  QVERIFY(detailview);
  auto *abortRebaseButton = detailview->findChild<QPushButton *>("AbortRebase");
  QVERIFY(abortRebaseButton);
  auto *continueRebaseButton =
      detailview->findChild<QPushButton *>("ContinueRebase");
  QVERIFY(continueRebaseButton);
  QCOMPARE(continueRebaseButton->isVisible(), false);
  QCOMPARE(abortRebaseButton->isVisible(), false);
}

void TestRebase::conflictingRebase() {
  INIT_REPO("rebaseConflicts.zip", true);

  auto *detailview = repoView->findChild<DetailView *>();
  QVERIFY(detailview);
  auto *abortRebaseButton = detailview->findChild<QPushButton *>("AbortRebase");
  QVERIFY(abortRebaseButton);
  auto *continueRebaseButton =
      detailview->findChild<QPushButton *>("ContinueRebase");
  QVERIFY(continueRebaseButton);
  QCOMPARE(continueRebaseButton->isVisible(), false);
  QCOMPARE(abortRebaseButton->isVisible(), false);

  int rebaseFinished = 0;
  int rebaseAboutToRebase = 0;
  int rebaseCommitSuccess = 0;
  int rebaseConflict = 0;
  int refreshTriggered = 0;

  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseInitError,
          [=]() { QVERIFY(false); }); // Should not be called
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseAboutToRebase,
          [=, &rebaseAboutToRebase](const Rebase rebase, const Commit before,
                                    int count) {
            QVERIFY(rebase.isValid());
            QCOMPARE(count, 1);
            QCOMPARE(before.message(), "File.txt changed by second branch\n");
            rebaseAboutToRebase++;
          });
  // TODO: = needed?
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitInvalid,
          [=]() { QVERIFY(false); }); // Should not be called
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseFinished,
          [=, &rebaseFinished]() { rebaseFinished++; });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitSuccess,
          [=, &rebaseCommitSuccess](const Rebase rebase, const Commit before,
                                    const Commit after, int counter) {
            QVERIFY(rebase.isValid());
            rebaseCommitSuccess++;
          });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseConflict,
          [=, &rebaseConflict, &rebaseCommitSuccess]() {
            QCOMPARE(rebaseCommitSuccess, 0); // was not called yet
            rebaseConflict++;
          });

  connect(mRepo.notifier(), &RepositoryNotifier::referenceUpdated,
          [this, &refreshTriggered](const Reference &ref) {
            // TODO: enable
            // QCOMPARE(ref, mRepo.head());
            refreshTriggered++;
          });

  const QString rebaseBranchName = "refs/heads/singleCommitConflict";

  git::Reference branch = mRepo.lookupRef(rebaseBranchName);
  QVERIFY(branch.isValid());
  auto c = branch.annotatedCommit().commit();

  // Checkout correct branch
  repoView->checkout(branch);

  QTest::qWait(100);

  // Rebase on main
  git::Reference mainBranch = mRepo.lookupRef(QString("refs/heads/main"));
  QVERIFY(mainBranch.isValid());
  auto ac = mainBranch.annotatedCommit();
  refreshTriggered = 0;
  LogEntry *entry = repoView->addLogEntry("Rebase", "Rebase", nullptr);
  repoView->rebase(ac, entry);
  QCOMPARE(refreshTriggered, 1); // Check that refresh was triggered

  QCOMPARE(mRepo.rebaseOngoing(), true);
  QCOMPARE(rebaseFinished, 0);
  QCOMPARE(rebaseConflict, 1);

  // Check that buttons are visible
  QTest::qWait(100);
  QCOMPARE(continueRebaseButton->isVisible(), true);
  QCOMPARE(abortRebaseButton->isVisible(), true);

  // Resolve conflicts
  diff = mRepo.status(mRepo.index(), nullptr, false);
  QCOMPARE(diff.count(), 1);
  QCOMPARE(diff.patch(0).isConflicted(), true);
  QFile f(mRepo.workdir().filePath(diff.patch(0).name()));
  QCOMPARE(f.open(QIODevice::WriteOnly), true);
  QVERIFY(f.write("Test123") !=
          -1); // just write something to resolve the conflict
  f.close();

  QTest::qWait(100);

  refreshTriggered = 0;
  repoView->continueRebase();  // should fail
  QCOMPARE(rebaseConflict, 2); // User tries to continue without staging
  QCOMPARE(refreshTriggered, 1);

  QTest::qWait(100); // Wait until refresh is done

  // Staging the file
  auto filewidgets = repoView->findChildren<FileWidget *>();
  QCOMPARE(filewidgets.length(), 1);
  filewidgets.at(0)->stageStateChanged(filewidgets.at(0)->modelIndex(),
                                       git::Index::StagedState::Staged);

  QTextEdit *editor = repoView->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);
  editor->setText("Test message");

  refreshTriggered = 0;
  rebaseConflict = 0;
  repoView->continueRebase();
  QCOMPARE(refreshTriggered, 1);

  // Check that branch is based on "main" now
  branch = mRepo.lookupRef(rebaseBranchName);
  QVERIFY(branch.isValid());
  QList<Commit> parents = branch.annotatedCommit().commit().parents();
  QCOMPARE(parents.count(), 1);
  QCOMPARE(parents.at(0).id(), ac.commit().id());
  QCOMPARE(
      branch.annotatedCommit().commit().message(),
      "Test message"); // custom message was used instead of the original one

  // Check that rebase was really finished
  QCOMPARE(mRepo.rebaseOngoing(), false);

  QTest::qWait(100); // Wait until refresh finished

  // Check that buttons are visible
  QCOMPARE(continueRebaseButton->isVisible(), false);
  QCOMPARE(abortRebaseButton->isVisible(), false);

  // Check call counters
  QCOMPARE(rebaseFinished, 1);
  QCOMPARE(rebaseAboutToRebase, 1);
  QCOMPARE(rebaseCommitSuccess, 1);
  QCOMPARE(rebaseConflict, 0);
}

void TestRebase::continueExternalStartedRebase() {
  //    INIT_REPO("rebaseConflicts.zip", true);

  //    QCOMPARE(repoView->isRebaseContinueVisible(), false);
  //    QCOMPARE(repoView->isRebaseAbortVisible(), false);

  //    int rebaseFinished = 0;
  //    int rebaseAboutToRebase = 0;
  //    int rebaseCommitSuccess = 0;
  //    int rebaseConflict = 0;
  //    int refreshTriggered = 0;
  //    int workdirChangeTriggered = 0;
  //    int referenceUpdated = 0;

  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseInitError,
  //    [](){QVERIFY(false);});  // Should not be called
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseAboutToRebase,
  //    [&rebaseAboutToRebase](const Rebase rebase, const Commit before, int
  //    count){
  //        QVERIFY(rebase.isValid());
  //        QCOMPARE(count, 1);
  //        QCOMPARE(before.message(), "File.txt changed by second branch\n");
  //        rebaseAboutToRebase++;
  //    });
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitInvalid,
  //    [](){QVERIFY(false);}); // Should not be called
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseFinished,
  //    [&rebaseFinished](){rebaseFinished++;}); connect(mRepo.notifier(),
  //    &git::RepositoryNotifier::rebaseCommitSuccess,
  //    [&rebaseCommitSuccess](const Rebase rebase, const Commit before, const
  //    Commit after, int counter) {
  //        QVERIFY(rebase.isValid());
  //        rebaseCommitSuccess++;
  //    });
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseConflict, [=,
  //    &rebaseConflict, &rebaseCommitSuccess](){
  //        QCOMPARE(rebaseCommitSuccess, 0); // was not called yet
  //        rebaseConflict++;
  //    });

  //    connect(mRepo.notifier(), &RepositoryNotifier::referenceUpdated, [this,
  //    &refreshTriggered](const Reference &ref) {
  //        // TODO: enable
  //        //QCOMPARE(ref, mRepo.head());
  //        refreshTriggered++;
  //    });

  //    connect(mRepo.notifier(), &RepositoryNotifier::workdirChanged, [this,
  //    &workdirChangeTriggered]() {
  //        workdirChangeTriggered++;
  //    });

  //    connect(mRepo.notifier(), &RepositoryNotifier::referenceUpdated, [this,
  //    &referenceUpdated]() {

  //    });

  //    referenceUpdated = 0;
  //    workdirChangeTriggered = 0;
  //    EXECUTE_GIT_COMMAND(path, "checkout singleCommitConflict", 0) // this
  //    should lead to an update of the tree
  //    //QCOMPARE(referenceUpdated, 1);
  //    EXECUTE_GIT_COMMAND(path, "rebase main", 1)
  //    //QCOMPARE(referenceUpdated, 2);
  //    Test::refresh(repoView); // TODO: must be called, because when changing
  //    externally the repoView will not be notified

  ////    while (workdirChangeTriggered == 0)
  ////        QTest::qWait(100);

  //    // TODO: currently rebaseOpen does not detect an interactive rebase
  //    // Fix was merged to libgit2 with
  //    https://github.com/libgit2/libgit2/pull/6334
  //    // So update of libgit2 is needed
  //    // TODO: turn on again
  //    //QCOMPARE(repoView->isRebaseContinueVisible(), true);
  //    //QCOMPARE(repoView->isRebaseAbortVisible(), true);

  //    diff = mRepo.status(mRepo.index(), nullptr, false);
  //    QCOMPARE(diff.count(), 1);
  //    QCOMPARE(diff.patch(0).isConflicted(), true);
  //    QFile f(mRepo.workdir().filePath(diff.patch(0).name()));
  //    QCOMPARE(f.open(QIODevice::WriteOnly), true);
  //    QVERIFY(f.write("Test123") != -1); // just write something to resolve
  //    the conflict f.close();

  //    // TODO: currently not possible, because no interactive rebase is
  //    detected. Libgit2 must be updated to a newer version
  //    repoView->continueRebase();

  //    QCOMPARE(rebaseFinished, 1);
  //    QCOMPARE(rebaseAboutToRebase, 1);
  //    QCOMPARE(rebaseCommitSuccess, 1);
  //    QCOMPARE(rebaseConflict, 0);
}

void TestRebase::startRebaseContinueInCLI() {
  //    // Check that GUI is updated correctly
  //    INIT_REPO("rebaseConflicts.zip", true);

  //    int rebaseFinished = 0;
  //    int rebaseAboutToRebase = 0;
  //    int rebaseCommitSuccess = 0;
  //    int rebaseConflict = 0;
  //    int refreshTriggered = 0;

  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseInitError,
  //    [](){QVERIFY(false);});  // Should not be called
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseAboutToRebase,
  //    [&rebaseAboutToRebase](const Rebase rebase, const Commit before, int
  //    count){
  //        QVERIFY(rebase.isValid());
  //        QCOMPARE(count, 1);
  //        QCOMPARE(before.message(), "File.txt changed by second branch\n");
  //        rebaseAboutToRebase++;
  //    });
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitInvalid,
  //    [](){QVERIFY(false);}); // Should not be called
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseFinished,
  //    [&rebaseFinished](){rebaseFinished++;}); connect(mRepo.notifier(),
  //    &git::RepositoryNotifier::rebaseCommitSuccess,
  //    [&rebaseCommitSuccess](const Rebase rebase, const Commit before, const
  //    Commit after, int counter) {
  //        QVERIFY(rebase.isValid());
  //        rebaseCommitSuccess++;
  //    });
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseConflict,
  //    [&rebaseConflict, &rebaseCommitSuccess](){
  //        QCOMPARE(rebaseCommitSuccess, 0); // was not called yet
  //        rebaseConflict++;
  //    });

  //    connect(mRepo.notifier(), &RepositoryNotifier::referenceUpdated, [this,
  //    &refreshTriggered](const Reference &ref) {
  //        // TODO: enable
  //        //QCOMPARE(ref, mRepo.head());
  //        refreshTriggered++;
  //    });

  //    const QString rebaseBranchName = "refs/heads/singleCommitConflict";

  //    git::Reference branch = mRepo.lookupRef(rebaseBranchName);
  //    QVERIFY(branch.isValid());
  //    auto c = branch.annotatedCommit().commit();

  //    // Checkout correct branch
  //    repoView->checkout(branch);

  //    // Rebase on main
  //    git::Reference mainBranch = mRepo.lookupRef(QString("refs/heads/main"));
  //    QVERIFY(mainBranch.isValid());
  //    auto ac = mainBranch.annotatedCommit();
  //    refreshTriggered = 0;
  //    LogEntry *entry = repoView->addLogEntry("Rebase", "Rebase", nullptr);
  //    repoView->rebase(ac, entry);
  //    QCOMPARE(refreshTriggered, 1); // Check that refresh was triggered

  //    QCOMPARE(mRepo.rebaseOngoing(), true);
  //    QCOMPARE(rebaseFinished, 0);
  //    QCOMPARE(rebaseConflict, 1);

  //    QTest::qWait(100); // Needed otherwise it is not refreshed and therefore
  //    the buttons are not updated
  //    QCOMPARE(repoView->isRebaseContinueVisible(), true);
  //    QCOMPARE(repoView->isRebaseAbortVisible(), true);

  //    //Test::refresh(repoView); // TODO: should not be needed!

  //    // Solve conflict
  //    diff = mRepo.status(mRepo.index(), nullptr, false);
  //    QCOMPARE(diff.count(), 1);
  //    QCOMPARE(diff.patch(0).isConflicted(), true);
  //    QFile f(mRepo.workdir().filePath(diff.patch(0).name()));
  //    QCOMPARE(f.open(QIODevice::WriteOnly), true);
  //    QVERIFY(f.write("Test123") != -1); // just write something to resolve
  //    the conflict f.close();

  //    EXECUTE_GIT_COMMAND(path, "rebase --continue", 1)

  //    Test::refresh(repoView);  // TODO: must be called, because when changing
  //    externally the repoView will not be notified. QTest::qWait(100);

  //    // Does not work, because libgit2 does not detect interactive rebases
  //    QCOMPARE(repoView->isRebaseContinueVisible(), false);
  //    QCOMPARE(repoView->isRebaseAbortVisible(), false);

  //    // Check that rebase was really finished
  //    QCOMPARE(mRepo.rebaseOngoing(), false);
}

void TestRebase::startRebaseContinueInCLIContinueGUI() {
  //    // Check that GUI is updated correctly

  //    INIT_REPO("rebaseConflicts.zip", true);

  //    QCOMPARE(repoView->isRebaseContinueVisible(), false);
  //    QCOMPARE(repoView->isRebaseAbortVisible(), false);

  //    int rebaseFinished = 0;
  //    int rebaseAboutToRebase = 0;
  //    int rebaseCommitSuccess = 0;
  //    int rebaseConflict = 0;
  //    int refreshTriggered = 0;
  //    int workdirChangeTriggered = 0;

  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseInitError,
  //    [=](){QVERIFY(false);});  // Should not be called
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseAboutToRebase,
  //    [=, &rebaseAboutToRebase](const Rebase rebase, const Commit before, int
  //    count){
  //        QVERIFY(rebase.isValid());
  //        QCOMPARE(count, 1);
  //        QCOMPARE(before.message(), "File.txt changed by second branch\n");
  //        rebaseAboutToRebase++;
  //    });
  //    // TODO: = needed?
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitInvalid,
  //    [=](){QVERIFY(false);}); // Should not be called
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseFinished, [=,
  //    &rebaseFinished](){rebaseFinished++;}); connect(mRepo.notifier(),
  //    &git::RepositoryNotifier::rebaseCommitSuccess, [=,
  //    &rebaseCommitSuccess](const Rebase rebase, const Commit before, const
  //    Commit after, int counter) {
  //        QVERIFY(rebase.isValid());
  //        rebaseCommitSuccess++;
  //    });
  //    connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseConflict, [=,
  //    &rebaseConflict, &rebaseCommitSuccess](){
  //        QCOMPARE(rebaseCommitSuccess, 0); // was not called yet
  //        rebaseConflict++;
  //    });

  //    connect(mRepo.notifier(), &RepositoryNotifier::referenceUpdated, [this,
  //    &refreshTriggered](const Reference &ref) {
  //        // TODO: enable
  //        //QCOMPARE(ref, mRepo.head());
  //        refreshTriggered++;
  //    });

  //    connect(mRepo.notifier(), &RepositoryNotifier::workdirChanged, [this,
  //    &workdirChangeTriggered]() {
  //        workdirChangeTriggered++;
  //    });

  //    const QString rebaseBranchName = "refs/heads/singleCommitConflict";

  //    git::Reference branch = mRepo.lookupRef(rebaseBranchName);
  //    QVERIFY(branch.isValid());
  //    auto c = branch.annotatedCommit().commit();

  //    // Checkout correct branch
  //    repoView->checkout(branch);

  //    // Rebase on main
  //    git::Reference mainBranch = mRepo.lookupRef(QString("refs/heads/main"));
  //    QVERIFY(mainBranch.isValid());
  //    auto ac = mainBranch.annotatedCommit();
  //    refreshTriggered = 0;
  //    LogEntry *entry = repoView->addLogEntry("Rebase", "Rebase", nullptr);
  //    repoView->rebase(ac, entry);
  //    QCOMPARE(refreshTriggered, 1); // Check that refresh was triggered

  //    QCOMPARE(mRepo.rebaseOngoing(), true);
  //    QCOMPARE(rebaseFinished, 0);
  //    QCOMPARE(rebaseConflict, 1);

  //    // Gui updated
  //    QCOMPARE(repoView->isRebaseContinueVisible(), true);
  //    QCOMPARE(repoView->isRebaseAbortVisible(), true);

  //    // Solve conflict
  //    QFile f(mRepo.workdir().filePath(diff.patch(0).name()));
  //    QCOMPARE(f.open(QIODevice::WriteOnly), true);
  //    QVERIFY(f.write("Test123") != -1); // just write something to resolve
  //    the conflict f.close();

  //    EXECUTE_GIT_COMMAND(path, "rebase --continue", 1)

  //    workdirChangeTriggered = 0;
  //    while (workdirChangeTriggered == 0)
  //        QTest::qWait(100);

  //    // TODO: another condition is needed, because there is no change in
  //    those visible so
  //    // it is not enough
  //    QCOMPARE(repoView->isRebaseContinueVisible(), true);
  //    QCOMPARE(repoView->isRebaseAbortVisible(), true);
}

void TestRebase::abortMR() {
  INIT_REPO("rebaseConflicts.zip", true);

  auto *detailview = repoView->findChild<DetailView *>();
  QVERIFY(detailview);
  auto *abortRebaseButton = detailview->findChild<QPushButton *>("AbortRebase");
  QVERIFY(abortRebaseButton);
  auto *continueRebaseButton =
      detailview->findChild<QPushButton *>("ContinueRebase");
  QVERIFY(continueRebaseButton);
  QCOMPARE(continueRebaseButton->isVisible(), false);
  QCOMPARE(abortRebaseButton->isVisible(), false);

  int rebaseFinished = 0;
  int rebaseAboutToRebase = 0;
  int rebaseCommitSuccess = 0;
  int rebaseConflict = 0;
  int refreshTriggered = 0;

  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseInitError,
          []() { QVERIFY(false); }); // Should not be called
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseAboutToRebase,
          [&rebaseAboutToRebase](const Rebase rebase, const Commit before,
                                 int count) {
            QVERIFY(rebase.isValid());
            QCOMPARE(count, 1);
            QCOMPARE(before.message(), "File.txt changed by second branch\n");
            rebaseAboutToRebase++;
          });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitInvalid,
          []() { QVERIFY(false); }); // Should not be called
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseFinished,
          [&rebaseFinished]() { rebaseFinished++; });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitSuccess,
          [&rebaseCommitSuccess](const Rebase rebase, const Commit before,
                                 const Commit after, int counter) {
            QVERIFY(rebase.isValid());
            rebaseCommitSuccess++;
          });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseConflict,
          [&rebaseConflict, &rebaseCommitSuccess]() {
            QCOMPARE(rebaseCommitSuccess, 0); // was not called yet
            rebaseConflict++;
          });

  connect(mRepo.notifier(), &RepositoryNotifier::referenceUpdated,
          [this, &refreshTriggered](const Reference &ref) {
            // TODO: enable
            // QCOMPARE(ref, mRepo.head());
            refreshTriggered++;
          });

  const QString rebaseBranchName = "refs/heads/singleCommitConflict";

  git::Reference branch = mRepo.lookupRef(rebaseBranchName);
  QVERIFY(branch.isValid());
  auto c = branch.annotatedCommit().commit();

  // Checkout correct branch
  repoView->checkout(branch);

  // Rebase on main
  git::Reference mainBranch = mRepo.lookupRef(QString("refs/heads/main"));
  QVERIFY(mainBranch.isValid());
  auto ac = mainBranch.annotatedCommit();
  refreshTriggered = 0;
  LogEntry *entry = repoView->addLogEntry("Rebase", "Rebase", nullptr);
  repoView->rebase(ac, entry);
  QCOMPARE(refreshTriggered, 1); // Check that refresh was triggered

  QCOMPARE(mRepo.rebaseOngoing(), true);
  QCOMPARE(rebaseFinished, 0);
  QCOMPARE(rebaseConflict, 1);

  // Check that buttons are visible
  QTest::qWait(100);
  QCOMPARE(continueRebaseButton->isVisible(), true);
  QCOMPARE(abortRebaseButton->isVisible(), true);

  refreshTriggered = 0;
  rebaseConflict = 0;
  repoView->abortRebase();
  QCOMPARE(refreshTriggered, 1);

  // Check that rebase was really finished
  QCOMPARE(mRepo.rebaseOngoing(), false);

  QTest::qWait(1000); // wait until detailview will be updated, after updating
                      // status is finished

  // Check that buttons are visible
  QCOMPARE(continueRebaseButton->isVisible(), false);
  QCOMPARE(abortRebaseButton->isVisible(), false);

  // Check call counters
  QCOMPARE(rebaseFinished, 0);
  QCOMPARE(rebaseAboutToRebase, 1);
  QCOMPARE(rebaseCommitSuccess, 0);
  QCOMPARE(rebaseConflict, 0);
}

void TestRebase::commitDuringRebase() {
  /*
   * Start rebasing in gui
   * Solve conflicts
   * Commit instead of continue rebase
   * Commit something else too
   * Continue rebase */

  INIT_REPO("rebaseConflicts.zip", true);

  auto *detailview = repoView->findChild<DetailView *>();
  QVERIFY(detailview);
  auto *abortRebaseButton = detailview->findChild<QPushButton *>("AbortRebase");
  QVERIFY(abortRebaseButton);
  auto *continueRebaseButton =
      detailview->findChild<QPushButton *>("ContinueRebase");
  QVERIFY(continueRebaseButton);
  QCOMPARE(continueRebaseButton->isVisible(), false);
  QCOMPARE(abortRebaseButton->isVisible(), false);

  int rebaseFinished = 0;
  int rebaseAboutToRebase = 0;
  int rebaseCommitSuccess = 0;
  int rebaseConflict = 0;
  int refreshTriggered = 0;

  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseInitError,
          []() { QVERIFY(false); }); // Should not be called
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseAboutToRebase,
          [&rebaseAboutToRebase](const Rebase rebase, const Commit before,
                                 int count) {
            QVERIFY(rebase.isValid());
            QCOMPARE(count, 1);
            QCOMPARE(before.message(), "File.txt changed by second branch\n");
            rebaseAboutToRebase++;
          });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitInvalid,
          []() { QVERIFY(false); }); // Should not be called
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseFinished,
          [&rebaseFinished]() { rebaseFinished++; });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseCommitSuccess,
          [&rebaseCommitSuccess](const Rebase rebase, const Commit before,
                                 const Commit after, int counter) {
            QVERIFY(rebase.isValid());
            rebaseCommitSuccess++;
          });
  connect(mRepo.notifier(), &git::RepositoryNotifier::rebaseConflict,
          [&rebaseConflict, &rebaseCommitSuccess]() {
            QCOMPARE(rebaseCommitSuccess, 0); // was not called yet
            rebaseConflict++;
          });

  connect(mRepo.notifier(), &RepositoryNotifier::referenceUpdated,
          [this, &refreshTriggered](const Reference &ref) {
            // TODO: enable
            // QCOMPARE(ref, mRepo.head());
            refreshTriggered++;
          });

  const QString rebaseBranchName = "refs/heads/singleCommitConflict";

  git::Reference branch = mRepo.lookupRef(rebaseBranchName);
  QVERIFY(branch.isValid());
  auto c = branch.annotatedCommit().commit();

  // Checkout correct branch
  repoView->checkout(branch);

  // Rebase on main
  git::Reference mainBranch = mRepo.lookupRef(QString("refs/heads/main"));
  QVERIFY(mainBranch.isValid());
  auto ac = mainBranch.annotatedCommit();
  refreshTriggered = 0;
  LogEntry *entry = repoView->addLogEntry("Rebase", "Rebase", nullptr);
  repoView->rebase(ac, entry);
  QCOMPARE(refreshTriggered, 1); // Check that refresh was triggered

  QCOMPARE(mRepo.rebaseOngoing(), true);
  QCOMPARE(rebaseFinished, 0);
  QCOMPARE(rebaseConflict, 1);

  // Check that buttons are visible
  QTest::qWait(100);
  QCOMPARE(continueRebaseButton->isVisible(), true);
  QCOMPARE(abortRebaseButton->isVisible(), true);

  // Resolve conflicts
  diff = mRepo.status(mRepo.index(), nullptr, false);
  QCOMPARE(diff.count(), 1);
  QCOMPARE(diff.patch(0).isConflicted(), true);
  QFile f(mRepo.workdir().filePath(diff.patch(0).name()));
  QCOMPARE(f.open(QIODevice::WriteOnly), true);
  QVERIFY(f.write("Test123") !=
          -1); // just write something to resolve the conflict
  f.close();

  Test::refresh(repoView);

  // stage file otherwise it is not possible to continue
  auto filewidgets = repoView->findChildren<FileWidget *>();
  QCOMPARE(filewidgets.length(), 1);
  filewidgets.at(0)->stageStateChanged(filewidgets.at(0)->modelIndex(),
                                       git::Index::StagedState::Staged);

  QTextEdit *editor = repoView->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);
  editor->setText("Test message");

  // Do commit before going on
  // So the user can commit between the rebase to split up the changes
  bool force = true;
  repoView->commit(force);

  refreshTriggered = 0;
  rebaseConflict = 0;
  repoView->continueRebase();
  QCOMPARE(refreshTriggered, 1);

  // Check that branch is based on "main" now
  branch = mRepo.lookupRef(rebaseBranchName);
  QVERIFY(branch.isValid());
  QList<Commit> parents = branch.annotatedCommit().commit().parents();
  QCOMPARE(parents.count(), 1);
  QCOMPARE(parents.at(0).id(), ac.commit().id());
  QCOMPARE(
      branch.annotatedCommit().commit().message(),
      "Test message"); // custom message was used instead of the original one

  // Check that rebase was really finished
  QCOMPARE(mRepo.rebaseOngoing(), false);

  QTest::qWait(10); // Wait until refresh is finished

  // Check that buttons are visible
  QCOMPARE(continueRebaseButton->isVisible(), false);
  QCOMPARE(abortRebaseButton->isVisible(), false);

  // Check call counters
  QCOMPARE(rebaseFinished, 1);
  QCOMPARE(rebaseAboutToRebase, 1);
  QCOMPARE(rebaseCommitSuccess, 1);
  QCOMPARE(rebaseConflict, 0);
}

TEST_MAIN(TestRebase)

#include "rebase.moc"
