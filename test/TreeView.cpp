#include "Test.h"
#include "ui/DiffView/DiffView.h"
#include "ui/MainWindow.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/TreeView.h"
#include "ui/TreeProxy.h"
#include "ui/FileContextMenu.h"
#include "conf/Settings.h"

#include <QTextEdit>

using namespace Test;
using namespace QTest;

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
  RepoView *repoView = window.currentView();

static void disableListView(TreeView &treeView, RepoView &repoView) {
  auto treeProxy = dynamic_cast<TreeProxy *>(treeView.model());
  QVERIFY(treeProxy);

  auto diffTreeModel = dynamic_cast<DiffTreeModel *>(treeProxy->sourceModel());
  QVERIFY(diffTreeModel);

  diffTreeModel->enableListView(false);
  Settings::instance()->setValue(Setting::Id::ShowChangedFilesAsList, false);
  repoView.refresh();
}

class TestTreeView : public QObject {
  Q_OBJECT

private slots:
  void restoreStagedFileAfterCommit();
  void discardFiles();
  void fileMergeCrash();
  void dirtySubmoduleAndStagedSubmodule();
  void conflictedAndStagedFile();

private:
};

void TestTreeView::restoreStagedFileAfterCommit() {
  INIT_REPO("TreeViewCollapseCount.zip", true);

  // Check for a single file called "test".
  RepoView *view = window.currentView();
  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  {
    auto unstagedTree = doubleTree->findChild<TreeView *>("Unstaged");
    QVERIFY(unstagedTree);
    disableListView(*unstagedTree, *view);
    QAbstractItemModel *unstagedModel = unstagedTree->model();
    // Wait for refresh
    auto timeout = Timeout(10000, "Repository didn't refresh in time");
    while (unstagedModel->rowCount() < 1)
      qWait(300);

    QCOMPARE(unstagedModel->rowCount(), 2);
    auto folder = unstagedModel->index(0, 0);
    auto subfolder = unstagedModel->index(0, 0, folder);
    auto file_txt = unstagedModel->index(0, 0, subfolder);
    QCOMPARE(unstagedModel->data(file_txt).toString(), QString("file.txt"));
    unstagedTree->selectionModel()->select(file_txt,
                                           QItemSelectionModel::Select);

    // Click on the check box. --> Stage file.txt
    mouseClick(unstagedTree->viewport(), Qt::LeftButton,
               Qt::KeyboardModifiers(),
               unstagedTree->checkRect(file_txt).center());
  }

  refresh(view, true);

  auto stagedTree = doubleTree->findChild<TreeView *>("Staged");
  stagedTree->expandAll();

  QAbstractItemModel *stagedModel = stagedTree->model();
  QVERIFY(stagedTree);
  {
    QCOMPARE(stagedModel->rowCount(), 1);
    auto folder = stagedModel->index(0, 0);
    auto subfolder = stagedModel->index(0, 0, folder);
    auto file_txt = stagedModel->index(0, 0, subfolder);
    QCOMPARE(stagedModel->data(file_txt).toString(), QString("file.txt"));

    // Select file
    stagedTree->selectionModel()->clearSelection();
    stagedTree->selectionModel()->select(file_txt, QItemSelectionModel::Select);
  }

  QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
  QVERIFY(editor);
  editor->setText("conflicting commit b");
  view->commit();

  // The application should not crash!
}

void TestTreeView::discardFiles() {
  // staging single files and discard files afterwards. It should not discard
  // not selected files Discarding a folder in staged treeview should only
  // delete the staged files, but not the unstaged files in that folder!

  INIT_REPO("TestRepository.zip", false);

  git::Commit commit =
      repo.lookupCommit("5c61b24e236310ad4a8a64f7cd1ccc968f1eec20");
  QVERIFY(commit);

  // modifying all files
  QHash<QString, QString> fileContent{
      {"file.txt", "Modified file"},
      {"file2.txt", "Modified file2"},
      {"folder1/file.txt", "Modified file in folder1"},
      {"folder1/file2.txt", "Modified file2 in folder1"},
      {"GittyupTestRepo/README.md", "Modified readme in submodule"},
  };
  {
    QHashIterator<QString, QString> i(fileContent);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::WriteOnly));
      file.write(i.value().toLatin1());
    }
  }

  // refresh repo
  refresh(repoView);

  // let the changes settle
  QApplication::processEvents();

  // Check for a single file called "test".
  RepoView *view = window.currentView();
  auto doubleTree = view->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);

  // stage folder1/file.txt
  {
    auto unstagedTree = doubleTree->findChild<TreeView *>("Unstaged");
    QVERIFY(unstagedTree);
    QAbstractItemModel *unstagedModel = unstagedTree->model();
    // Wait for refresh
    auto timeout = Timeout(10000, "Repository didn't refresh in time");
    while (unstagedModel->rowCount() < 1)
      qWait(300);

    QCOMPARE(unstagedModel->rowCount(), 4);
    auto folder1 = unstagedModel->index(3, 0);
    auto file_txt = unstagedModel->index(0, 0, folder1);
    QCOMPARE(unstagedModel->data(file_txt).toString(), QString("file.txt"));
    unstagedTree->selectionModel()->select(file_txt,
                                           QItemSelectionModel::Select);

    // Click on the check box. --> Stage file.txt
    mouseClick(unstagedTree->viewport(), Qt::LeftButton,
               Qt::KeyboardModifiers(),
               unstagedTree->checkRect(file_txt).center());
  }

  refresh(view, true);

  auto stagedTree = doubleTree->findChild<TreeView *>("Staged");
  stagedTree->expandAll();

  // discard staged folder1
  QAbstractItemModel *stagedModel = stagedTree->model();
  QVERIFY(stagedTree);
  {
    QCOMPARE(stagedModel->rowCount(), 1);
    auto folder1 = stagedModel->index(0, 0);
    QCOMPARE(stagedModel->data(folder1).toString(), QString("folder1"));

    // Select file
    stagedTree->selectionModel()->clearSelection();
    stagedTree->selectionModel()->select(folder1, QItemSelectionModel::Select);
  }

  DoubleTreeWidget::showFileContextMenu(QPoint(), repoView, stagedTree, true);

  auto *menu = doubleTree->findChild<FileContextMenu *>();
  QVERIFY(menu);
  auto files = menu->mAccumFiles.getFiles();
  QCOMPARE(files.count(), 1);
  // only folder1/file.txt shall get discarded.
  // folder1/file2.txt shall not discarded!
  QCOMPARE(files.at(0), "folder1/file.txt");

  // From here on everything is tested in TestFileContextMenu
}

void TestTreeView::fileMergeCrash() {
  INIT_REPO("CrashMerge.zip", false);

  git::Reference otherBranch = repo.lookupRef("refs/heads/otherBranch");
  QVERIFY(otherBranch);

  git::Reference master =
      repo.lookupRef(QString("refs/heads/%1").arg("master"));
  QVERIFY(master);

  QCOMPARE(repo.head().name(), "master");

  repoView->merge(RepoView::Merge, otherBranch);

  // Diff is in a conflicted state
  git::Diff diff = repo.diffIndexToWorkdir();
  QVERIFY(diff.isConflicted());

  auto doubleTree = repoView->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);
  doubleTree->fileCountExpansionThreshold = 5;
  auto stagedTree = doubleTree->findChild<TreeView *>("Staged");
  QVERIFY(stagedTree);
  auto unstagedTree = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(unstagedTree);

  QAbstractItemModel *stagedModel = stagedTree->model();

  // Wait for refresh
  while (stagedModel->rowCount() < 3)
    qWait(300);

  QAbstractItemModel *unstagedModel = unstagedTree->model();
  QCOMPARE(unstagedModel->rowCount(), 1);

  unstagedTree->expandAll();

  QModelIndex index = unstagedModel->index(0, 0); // common
  QVERIFY(index.isValid());
  index = unstagedModel->index(0, 0, index); // src
  QVERIFY(index.isValid());
  index = unstagedModel->index(0, 0, index); // main
  QVERIFY(index.isValid());
  index = unstagedModel->index(0, 0, index); // java
  QVERIFY(index.isValid());
  index = unstagedModel->index(0, 0, index); // com
  QVERIFY(index.isValid());
  index = unstagedModel->index(0, 0, index); // something
  QVERIFY(index.isValid());
  index = unstagedModel->index(0, 0, index); // common
  QVERIFY(index.isValid());
  index = unstagedModel->index(0, 0, index); // configs
  QVERIFY(index.isValid());
  index = unstagedModel->index(0, 0, index); // security_config
  QVERIFY(index.isValid());
  index = unstagedModel->index(0, 0, index); // File_security_config
  QVERIFY(index.isValid());

  unstagedTree->selectionModel()->select(
      index, QItemSelectionModel::SelectionFlag::Select);

  auto diffView = doubleTree->findChild<DiffView *>();
  QVERIFY(diffView);

  QToolButton *theirs = diffView->findChild<QToolButton *>("ConflictTheirs");
  QVERIFY(theirs);
  mouseClick(theirs, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(), 0);

  QToolButton *save =
      diffView->widget()->findChild<QToolButton *>("ConflictSave");
  QVERIFY(save);
  mouseClick(save, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(), 0);

  // should not crash
}

void TestTreeView::dirtySubmoduleAndStagedSubmodule() {
  INIT_REPO("DirtySubmoduleUnstagedTree.zip", false);

  auto doubleTree = repoView->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);
  auto stagedTree = doubleTree->findChild<TreeView *>("Staged");
  QVERIFY(stagedTree);
  auto unstagedTree = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(unstagedTree);

  {
    QAbstractItemModel *stagedModel = stagedTree->model();
    QCOMPARE(stagedModel->rowCount(), 1);
    QModelIndex index = stagedModel->index(0, 0); // submodules folder
    QVERIFY(index.isValid());
    QCOMPARE(index.data(), "submodules");

    QCOMPARE(stagedModel->rowCount(index), 1);
    index = stagedModel->index(0, 0, index); // submodule1
    QVERIFY(index.isValid());
    QCOMPARE(index.data(), "submodule1");
  }

  {
    QAbstractItemModel *unstagedModel = unstagedTree->model();
    QCOMPARE(unstagedModel->rowCount(), 1);
    QModelIndex index = unstagedModel->index(0, 0); // submodules folder
    QVERIFY(index.isValid());
    QCOMPARE(index.data(), "submodules");

    QCOMPARE(unstagedModel->rowCount(index), 1);
    index = unstagedModel->index(0, 0, index); // submodule2
    QVERIFY(index.isValid());
    QCOMPARE(index.data(), "submodule2");
  }
}

void TestTreeView::conflictedAndStagedFile() {
  INIT_REPO("ConflictedAndStagedFile.zip", false);

  auto doubleTree = repoView->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);
  auto stagedTree = doubleTree->findChild<TreeView *>("Staged");
  QVERIFY(stagedTree);
  auto unstagedTree = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(unstagedTree);

  {
    QAbstractItemModel *stagedModel = stagedTree->model();
    QCOMPARE(stagedModel->rowCount(), 1);
    QModelIndex index = stagedModel->index(0, 0); // "folder" folder
    QVERIFY(index.isValid());
    QCOMPARE(index.data(), "folder");

    QCOMPARE(stagedModel->rowCount(index), 1);
    index = stagedModel->index(0, 0, index);
    QVERIFY(index.isValid());
    QCOMPARE(index.data(), "NotConflictedFile.txt");
  }

  {
    QAbstractItemModel *unstagedModel = unstagedTree->model();
    QCOMPARE(unstagedModel->rowCount(), 1);
    QModelIndex index = unstagedModel->index(0, 0); // "folder" folder
    QVERIFY(index.isValid());
    QCOMPARE(index.data(), "folder");

    QCOMPARE(unstagedModel->rowCount(index), 1);
    index = unstagedModel->index(0, 0, index);
    QVERIFY(index.isValid());
    QCOMPARE(index.data(), "conflictedFile.txt");
  }
}

TEST_MAIN(TestTreeView)

#include "TreeView.moc"
