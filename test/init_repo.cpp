//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Test.h"
#include "dialogs/CloneDialog.h"
#include "dialogs/StartDialog.h"
#include "ui/CommitList.h"
#include "ui/DetailView.h"
#include "ui/DiffView/DiffView.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/Footer.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/TreeView.h"
#include <QFile>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTextStream>
#include <QToolButton>

using namespace Test;
using namespace QTest;

class TestInitRepo : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void addFile();
  void commitFile();
  void amendCommit();
  void editFile();
  void cleanupTestCase();

private:
  MainWindow *mWindow = nullptr;
};

void TestInitRepo::initTestCase()
{
  QDir dir = QDir::temp();
  if (dir.cd("test_init_repo"))
    QVERIFY(dir.removeRecursively());

  StartDialog *dialog = StartDialog::openSharedInstance();
  QVERIFY(qWaitForWindowActive(dialog));

  // Find the first button in the first footer.
  Footer *footer = dialog->findChild<Footer *>();
  QToolButton *plus = footer->findChild<QToolButton *>();

  // Set up timer to dismiss the popup.
  QTimer::singleShot(500, [] {
    QMenu *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
    QVERIFY(menu);

    keyClick(menu, Qt::Key_Down);
    keyClick(menu, Qt::Key_Down);
    keyClick(menu, Qt::Key_Down);
    keyClick(menu, Qt::Key_Return);
  });

  // Show popup menu.
  mouseClick(plus, Qt::LeftButton);

  CloneDialog *cloneDialog =
    qobject_cast<CloneDialog *>(QApplication::activeModalWidget());
  QVERIFY(cloneDialog);

  // Set fields.
  cloneDialog->setField("name", "test_init_repo");
  cloneDialog->setField("path", QDir::tempPath());

  // Click return.
  keyClick(cloneDialog, Qt::Key_Return);

  // Wait on the new window.
  mWindow = MainWindow::activeWindow();
  QVERIFY(mWindow && qWaitForWindowActive(mWindow));
}

void TestInitRepo::addFile()
{
  // Create a file.
  QDir dir = QDir::temp();
  QVERIFY(dir.cd("test_init_repo"));

  QFile file(dir.filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a test.";
  file.close();

  // Check for a single file called "test".
  RepoView *view = mWindow->currentView();
  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  auto files = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(files);

  // Wait for refresh
  QAbstractItemModel *model = files->model();
  while(model->rowCount() < 1)
    qWait(300);

  QCOMPARE(model->rowCount(), 1);
  QCOMPARE(model->data(model->index(0, 0)).toString(), QString("test"));
}

void TestInitRepo::commitFile()
{
  RepoView *view = mWindow->currentView();
  DetailView *detailView = view->findChild<DetailView *>();
  QVERIFY(detailView);

  QPushButton *stageAll = detailView->findChild<QPushButton *>("StageAll");
  QVERIFY(stageAll);

  mouseClick(stageAll, Qt::LeftButton);
  view->commit();
}

void TestInitRepo::amendCommit()
{
  RepoView *view = mWindow->currentView();
  QVERIFY(view);

  bool finished = false;
  connect(view, &RepoView::statusChanged, [&finished]() {
    finished = true;
  });

  view->amendCommit();

  while (!finished)
    qWait(300);

  // Verify commit amended
  CommitList *commitList = view->findChild<CommitList *>();
  QVERIFY(commitList);
  QAbstractItemModel *commitModel = commitList->model();
  QModelIndex index = commitModel->index(0,0);
  QString name = commitModel->data(index, Qt::DisplayRole).toString();
  QCOMPARE(name, QString("Uncommitted changes"));
}

void TestInitRepo::editFile()
{
  RepoView *view = mWindow->currentView();

  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  auto files = doubleTree->findChild<TreeView *>("Staged");
  QVERIFY(files);

  files->selectionModel()->select(
    files->model()->index(0, 0),
    QItemSelectionModel::Select
  );

  DiffView *diff = view->findChild<DiffView *>();
  QVERIFY(diff);

  QToolButton *edit = diff->findChild<QToolButton *>("EditButton");
  QVERIFY(edit);

  // Set up timer to dismiss the popup.
  QTimer::singleShot(500, [] {
    QMenu *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
    QVERIFY(menu);

    keyClick(menu, Qt::Key_Down);
    keyClick(menu, Qt::Key_Return);
  });

  mouseClick(edit, Qt::LeftButton);
}

void TestInitRepo::cleanupTestCase()
{
  mWindow->close();
  QDir dir = QDir::temp();
  QVERIFY(dir.cd("test_init_repo"));
  QVERIFY(dir.removeRecursively());
}

TEST_MAIN(TestInitRepo)

#include "init_repo.moc"
