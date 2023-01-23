//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Test.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/TreeView.h"
#include <QFile>
#include <QTextEdit>
#include <QTextStream>

using namespace Test;
using namespace QTest;

class TestIndex : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void stageAddition();
  void stageDeletion();
  void stageDirectory();
  void cleanupTestCase();

private:
  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
};

void TestIndex::initTestCase() {
  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowActive(mWindow));
}

void TestIndex::stageAddition() {
  // Add file and refresh.
  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a test." << endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  auto unstagedFiles = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(unstagedFiles);

  auto stagedFiles = doubleTree->findChild<TreeView *>("Staged");
  QVERIFY(stagedFiles);

  QAbstractItemModel *unstagedModel = unstagedFiles->model();
  QCOMPARE(unstagedModel->rowCount(), 1);

  // Check that it starts unstaged.
  QModelIndex unstagedIndex = unstagedModel->index(0, 0);
  QVERIFY(!unstagedIndex.data(Qt::CheckStateRole).toBool());

  // Click on the check box.
  mouseClick(unstagedFiles->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
             unstagedFiles->checkRect(unstagedIndex).center());

  QAbstractItemModel *stagedModel = stagedFiles->model();
  QCOMPARE(stagedModel->rowCount(), 1);

  // Check that it's staged now.
  QModelIndex stagedIndex = stagedModel->index(0, 0);
  QVERIFY(stagedIndex.data(Qt::CheckStateRole).toBool());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("addition");
  view->commit();
  refresh(view, false);
}

void TestIndex::stageDeletion() {
  // Remove file and refresh.
  mRepo->workdir().remove("test");

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  auto unstagedFiles = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(unstagedFiles);

  auto stagedFiles = doubleTree->findChild<TreeView *>("Staged");
  QVERIFY(stagedFiles);

  QAbstractItemModel *unstagedModel = unstagedFiles->model();
  QCOMPARE(unstagedModel->rowCount(), 1);

  // Check that it starts unstaged.
  QModelIndex unstagedIndex = unstagedModel->index(0, 0);
  QVERIFY(!unstagedIndex.data(Qt::CheckStateRole).toBool());

  // Click on the check box.
  mouseClick(unstagedFiles->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
             unstagedFiles->checkRect(unstagedIndex).center());

  QAbstractItemModel *stagedModel = stagedFiles->model();
  QCOMPARE(stagedModel->rowCount(), 1);

  // Check that it's staged now.
  QModelIndex stagedIndex = stagedModel->index(0, 0);
  QVERIFY(stagedIndex.data(Qt::CheckStateRole).toBool());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("deletion");
  view->commit();
  refresh(view, false);
}

void TestIndex::stageDirectory() {
  QDir dir = mRepo->workdir();
  dir.mkdir("dir");
  QVERIFY(dir.cd("dir"));

  QFile file1(dir.filePath("test1"));
  QVERIFY(file1.open(QFile::WriteOnly));
  QTextStream(&file1) << "This is a test." << endl;

  QFile file2(dir.filePath("test2"));
  QVERIFY(file2.open(QFile::WriteOnly));
  QTextStream(&file2) << "This is a test." << endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  auto unstagedFiles = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(unstagedFiles);

  auto stagedFiles = doubleTree->findChild<TreeView *>("Staged");
  QVERIFY(stagedFiles);

  QAbstractItemModel *unstagedModel = unstagedFiles->model();
  QCOMPARE(unstagedModel->rowCount(), 1);

  // Check that it starts unstaged.
  QModelIndex unstagedIndex = unstagedModel->index(0, 0);
  QCOMPARE(unstagedModel->rowCount(unstagedIndex), 2);
  QVERIFY(!unstagedIndex.data(Qt::CheckStateRole).toBool());

  QVERIFY(!unstagedModel->index(0, 0, unstagedIndex)
               .data(Qt::CheckStateRole)
               .toBool());
  QVERIFY(!unstagedModel->index(1, 0, unstagedIndex)
               .data(Qt::CheckStateRole)
               .toBool());

  // Click on the check box.
  mouseClick(unstagedFiles->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
             unstagedFiles->checkRect(unstagedIndex).center());

  // Check for two staged files.
  QAbstractItemModel *stagedModel = stagedFiles->model();
  QCOMPARE(stagedModel->rowCount(), 1);

  QModelIndex stagedIndex = stagedModel->index(0, 0);
  QCOMPARE(stagedModel->rowCount(stagedIndex), 2);

  // Check that they're staged now.
  QVERIFY(
      stagedModel->index(0, 0, stagedIndex).data(Qt::CheckStateRole).toBool());
  QVERIFY(
      stagedModel->index(1, 0, stagedIndex).data(Qt::CheckStateRole).toBool());
}

void TestIndex::cleanupTestCase() { mWindow->close(); }

TEST_MAIN(TestIndex)

#include "index.moc"
