//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "ui/MainWindow.h"
#include "ui/DetailView.h"
#include "ui/DiffView/DiffView.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/RepoView.h"
#include "ui/TreeView.h"
#include <QFile>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>

using namespace Test;
using namespace QTest;

class TestMerge : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void firstCommit();
  void secondCommit();
  void thirdCommit();
  void mergeConflict();
  void resolve();
  void cleanupTestCase();

private:
  int inputDelay = 0;
  int closeDelay = 0;

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  QString mMainBranch;
};

void TestMerge::initTestCase() {
  mMainBranch = mRepo->unbornHeadName();
  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));
}

void TestMerge::firstCommit() {
  // Add file and refresh.
  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This will be a test." << endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  auto files = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(files);

  QAbstractItemModel *model = files->model();
  QCOMPARE(model->rowCount(), 1);

  // Click on the check box.
  QModelIndex index = model->index(0, 0);
  mouseClick(files->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
             files->checkRect(index).center());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("base commit");
  view->commit();
  refresh(view, false);
}

void TestMerge::secondCommit() {
  RepoView *view = mWindow->currentView();
  git::Branch branch = mRepo->createBranch("branch2", mRepo->head().target());
  QVERIFY(branch.isValid());

  view->checkout(branch);
  QCOMPARE(mRepo->head().name(), QString("branch2"));

  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a conflict." << endl;

  refresh(view);

  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  auto files = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(files);

  QAbstractItemModel *model = files->model();
  QCOMPARE(model->rowCount(), 1);

  // Click on the check box.
  QModelIndex index = model->index(0, 0);
  mouseClick(files->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
             files->checkRect(index).center());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("conflicting commit b");
  view->commit();
  refresh(view, false);
}

void TestMerge::thirdCommit() {
  RepoView *view = mWindow->currentView();
  git::Reference ref =
      mRepo->lookupRef(QString("refs/heads/%1").arg(mMainBranch));
  QVERIFY(ref);

  view->checkout(ref);
  QCOMPARE(mRepo->head().name(), mMainBranch);

  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a test." << endl;

  refresh(view);

  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  auto files = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(files);

  QAbstractItemModel *model = files->model();
  QCOMPARE(model->rowCount(), 1);

  // Click on the check box.
  QModelIndex index = model->index(0, 0);
  mouseClick(files->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
             files->checkRect(index).center());

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("conflicting commit a");
  view->commit();
  refresh(view, false);
}

void TestMerge::mergeConflict() {
  RepoView *view = mWindow->currentView();
  git::Reference master =
      mRepo->lookupRef(QString("refs/heads/%1").arg(mMainBranch));
  QVERIFY(master);

  git::Reference branch2 = mRepo->lookupRef("refs/heads/branch2");
  QVERIFY(branch2);

  QCOMPARE(mRepo->head().name(), mMainBranch);

  view->merge(RepoView::Merge, branch2);

  // Diff is in a conflicted state
  git::Diff diff = mRepo->diffIndexToWorkdir();
  QVERIFY(diff.isConflicted());
}

void TestMerge::resolve() {
  RepoView *view = mWindow->currentView();
  DiffView *diffView = view->findChild<DiffView *>();

  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  auto files = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(files);

  // Wait for refresh
  QAbstractItemModel *model = files->model();
  qWait(1000); // Because before the merge, there is already an item in the
               // unstaged model
  while (model->rowCount() < 1)
    qWait(300);

  files->selectionModel()->select(files->model()->index(0, 0),
                                  QItemSelectionModel::Select);

  QToolButton *theirs = diffView->findChild<QToolButton *>("ConflictTheirs");
  QVERIFY(theirs);
  mouseClick(theirs, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(),
             inputDelay);

  QToolButton *undo =
      diffView->widget()->findChild<QToolButton *>("ConflictUndo");
  QVERIFY(undo);
  mouseClick(undo, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(),
             inputDelay);

  QToolButton *ours =
      diffView->widget()->findChild<QToolButton *>("ConflictOurs");
  QVERIFY(ours);
  mouseClick(ours, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(),
             inputDelay);

  QToolButton *save =
      diffView->widget()->findChild<QToolButton *>("ConflictSave");
  QVERIFY(save);
  mouseClick(save, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(),
             inputDelay);

  DetailView *detailView = view->findChild<DetailView *>();
  QPushButton *stageAll = nullptr;
  while (stageAll == nullptr) {
    stageAll = detailView->findChild<QPushButton *>("StageAll");
    qWait(100);
  }
  mouseClick(stageAll, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(),
             inputDelay);

  // Commit and refresh.
  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);

  editor->setText("conflicts resolved");
  view->commit();
  refresh(view, false);

  // Diff is not in a conflicted state
  git::Diff diff = mRepo->diffIndexToWorkdir();
  QVERIFY(!diff.isConflicted());
}

void TestMerge::cleanupTestCase() {
  qWait(closeDelay);
  mWindow->close();
}

TEST_MAIN(TestMerge)

#include "merge.moc"
