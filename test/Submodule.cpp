//
//          Copyright (c) 2022, Gittyup Team
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "Test.h"

#include "dialogs/CloneDialog.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/RepoView.h"
#include "conf/Settings.h"
#include "git/Submodule.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/TreeView.h"

#include <QToolButton>
#include <QMenu>
#include <QWizard>
#include <QLineEdit>

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  auto repo = git::Repository::open(path);                                     \
  QVERIFY(repo.isValid());                                                     \
  Test::initRepo(repo);                                                        \
  MainWindow window(repo);                                                     \
  window.show();                                                               \
  QVERIFY(QTest::qWaitForWindowExposed(&window));                              \
                                                                               \
  RepoView *repoView = window.currentView();                                   \
  auto diff = repo.status(repo.index(), nullptr, false);

using namespace Test;
using namespace QTest;

class TestSubmodule : public QObject {
  Q_OBJECT

private slots:
  void updateSubmoduleClone();
  void noUpdateSubmoduleClone();
  void discardFile();

private:
};

void TestSubmodule::updateSubmoduleClone() {
  // Update submodules after cloning
  QString remote = Test::extractRepository("SubmoduleTest.zip", true);
  QCOMPARE(remote.isEmpty(), false);

  Settings *settings = Settings::instance();
  settings->setValue(Setting::Id::UpdateSubmodulesAfterPullAndClone, true);
  CloneDialog *d = new CloneDialog(CloneDialog::Kind::Clone);

  RepoView *view = nullptr;

  bool cloneFinished = false;
  QObject::connect(d, &CloneDialog::accepted, [d, &view, &cloneFinished] {
    cloneFinished = true;
    if (MainWindow *window = MainWindow::open(d->path())) {
      view = window->currentView();
    }
  });

  QTemporaryDir tempdir;
  QVERIFY(tempdir.isValid());
  d->setField("url", remote);
  d->setField("name", "TestrepoSubmodule");
  d->setField("path", tempdir.path());
  d->setField("bare", "false");
  d->page(2)->initializePage(); // start clone

  {
    auto timeout = Timeout(10e3, "Failed to clone");
    while (!cloneFinished)
      qWait(300);
  }

  QVERIFY(view);
  QCOMPARE(view->repo().submodules().count(), 1);
  for (const auto &s : view->repo().submodules()) {
    QVERIFY(s.isValid());
    QVERIFY(s.isInitialized());
  }
}

void TestSubmodule::noUpdateSubmoduleClone() {
  // Don't update submodules after cloning
  QString remote = Test::extractRepository("SubmoduleTest.zip", true);
  QCOMPARE(remote.isEmpty(), false);

  Settings *settings = Settings::instance();
  settings->setValue(Setting::Id::UpdateSubmodulesAfterPullAndClone, false);
  CloneDialog *d = new CloneDialog(CloneDialog::Kind::Clone);

  RepoView *view = nullptr;

  bool cloneFinished = false;
  QObject::connect(d, &CloneDialog::accepted, [d, &view, &cloneFinished] {
    cloneFinished = true;
    if (MainWindow *window = MainWindow::open(d->path())) {
      view = window->currentView();
    }
  });

  QTemporaryDir tempdir;
  QVERIFY(tempdir.isValid());
  d->setField("url", remote);
  d->setField("name", "TestrepoSubmodule");
  d->setField("path", tempdir.path());
  d->setField("bare", "false");
  d->page(2)->initializePage(); // start clone

  {
    auto timeout = Timeout(10e3, "Failed to clone");
    while (!cloneFinished)
      qWait(300);
  }

  QVERIFY(view);
  QCOMPARE(view->repo().submodules().count(), 1);
  for (const auto &s : view->repo().submodules()) {
    QVERIFY(s.isValid());
    QCOMPARE(s.isInitialized(), false);
  }
}

void TestSubmodule::discardFile() {
  // Discarding a file should not reset the submodule
  INIT_REPO("SubmoduleTest.zip", true);
  repoView->updateSubmodules(repo.submodules(), true, true);

  qWait(1000); // Not needed if the test is long enough and the fetch operation
               // finishes

  QCOMPARE(repo.submodules().count(), 1);
  for (const auto &submodule : repo.submodules())
    QVERIFY(submodule.isInitialized());

  {
    QFile file(repo.workdir().filePath("README.md"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "Changing readme of main repository" << endl;
    file.close();
  }

  {
    QFile file(repo.workdir().filePath("GittyupTestRepo/README.md"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "Changing content of submodule readme" << endl;
    file.close();
  }

  auto doubleTree = repoView->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  // Select head
  // Does not work
  // repoView->selectHead();
  // repoView->selectFirstCommit();

  refresh(repoView); // Do a refresh to simulate selecting the working directory
                     // entry in the commit list

  {
    // wait for refresh!
    auto unstagedTree = doubleTree->findChild<TreeView *>("Unstaged");
    QVERIFY(unstagedTree);
    QAbstractItemModel *unstagedModel = unstagedTree->model();
    auto timeout = Timeout(10000, "Repository didn't refresh in time");
    while (unstagedModel->rowCount() < 2 ||
           unstagedModel->data(unstagedModel->index(1, 0)) != "README.md")
      qWait(300);
  }

  {
    auto unstagedTree = doubleTree->findChild<TreeView *>("Unstaged");
    QVERIFY(unstagedTree);
    QAbstractItemModel *unstagedModel = unstagedTree->model();

    QCOMPARE(unstagedModel->rowCount(), 2);
    auto submodule = unstagedModel->index(0, 0);
    auto readme = unstagedModel->index(1, 0);
    QCOMPARE(unstagedModel->data(readme).toString(), QString("README.md"));

    unstagedTree->discard(readme, true);
  }

  QFile file(repo.workdir().filePath("GittyupTestRepo/README.md"));
  QVERIFY(file.open(QFile::ReadOnly));
  QCOMPARE(file.readAll(), "Changing content of submodule readme\n");
}

TEST_MAIN(TestSubmodule)

#include "Submodule.moc"
