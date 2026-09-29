#include "Test.h"
#include "conf/Settings.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/TreeView.h"
#include "ui/DiffView/DiffView.h"
#include "ui/DiffView/HunkWidget.h"

#include <QScrollBar>

using namespace Test;
using namespace QTest;

namespace {

// Far more hunks than fit in the viewport, so lazy loading has to stop early.
const int kHunkCount = 60;

QMap<QString, QString> contents(bool manyFiles, const QString &state) {
  QMap<QString, QString> files;
  if (manyFiles) {
    for (int i = 0; i < kHunkCount; ++i) {
      QString name = QString("files/file%1.txt").arg(i, 3, 10, QChar('0'));
      files.insert(name, state + "\n");
    }
    return files;
  }

  // Enough unchanged lines between the edits to keep each one in its own hunk.
  QString text;
  for (int i = 0; i < kHunkCount; ++i) {
    for (int j = 0; j < 10; ++j)
      text += QString("line %1.%2\n").arg(i).arg(j);
    text += QString("%1 %2\n").arg(state).arg(i);
  }
  files.insert("files/hunks.txt", text);
  return files;
}

bool writeFiles(const QDir &workdir, const QMap<QString, QString> &files) {
  for (auto it = files.begin(); it != files.end(); ++it) {
    QFile file(workdir.filePath(it.key()));
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
      return false;
    file.write(it.value().toUtf8());
  }
  return true;
}

} // namespace

class TestDiffView : public QObject {
  Q_OBJECT

private slots:
  void loadsUntilScrollbarVisible_data();
  void loadsUntilScrollbarVisible();
};

void TestDiffView::loadsUntilScrollbarVisible_data() {
  QTest::addColumn<bool>("manyFiles");
  QTest::addColumn<bool>("committed");

  // Hunks of a single file are only loaded lazily for commit diffs; a working
  // tree diff always loads them all at once.
  QTest::newRow("many files, working tree") << true << false;
  QTest::newRow("many files, commit") << true << true;
  QTest::newRow("many hunks in one file, commit") << false << true;
}

void TestDiffView::loadsUntilScrollbarVisible() {
  QFETCH(bool, manyFiles);
  QFETCH(bool, committed);

  Settings::instance()->setValue(Setting::Id::ShowChangedFilesAsList, false);

  ScratchRepository repo;
  QDir workdir = repo->workdir();
  QVERIFY(workdir.mkpath("files"));

  auto before = contents(manyFiles, "before");
  QVERIFY(writeFiles(workdir, before));
  repo->index().setStaged(before.keys(), true);
  QVERIFY(repo->commit("initial"));

  QVERIFY(writeFiles(workdir, contents(manyFiles, "after")));
  if (committed) {
    repo->index().setStaged(before.keys(), true);
    QVERIFY(repo->commit("change"));
  }

  MainWindow window(repo);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));

  // Tall enough that the first four hunks fit without a scrollbar.
  window.resize(1200, 2000);

  RepoView *repoView = window.currentView();
  if (committed)
    repoView->selectFirstCommit();

  auto doubleTree = repoView->findChild<DoubleTreeWidget *>();
  QVERIFY(doubleTree);
  auto unstagedTree = doubleTree->findChild<TreeView *>("Unstaged");
  QVERIFY(unstagedTree);

  QAbstractItemModel *model = unstagedTree->model();
  QTRY_COMPARE_WITH_TIMEOUT(model->rowCount(), 1, 10000);

  // Select the folder so the diff view shows every file below it.
  QModelIndex folder = model->index(0, 0);
  QVERIFY(folder.isValid());
  unstagedTree->selectionModel()->select(folder, QItemSelectionModel::Select);

  auto diffView = repoView->findChild<DiffView *>();
  QVERIFY(diffView);
  QScrollBar *scrollBar = diffView->verticalScrollBar();
  auto hunks = [diffView] {
    return diffView->widget()->findChildren<HunkWidget *>();
  };

  // Without the follow-up fetch this stays at zero, since nothing scrolls or
  // resizes once the initial batch fits on screen.
  QTRY_VERIFY_WITH_TIMEOUT(scrollBar->maximum() > 0, 10000);

  int previousCount = -1;
  auto loadingSettled = [&] {
    int count = hunks().size();
    bool settled = count == previousCount;
    previousCount = count;
    return settled;
  };
  QTRY_VERIFY_WITH_TIMEOUT(loadingSettled(), 5000);

  // Once loading stops the scrollbar must still be there, with more than the
  // initial batch of four loaded behind it.
  const QList<HunkWidget *> loaded = hunks();
  QVERIFY(scrollBar->maximum() > 0);
  QVERIFY(loaded.size() > 4);

  // The initial batch of four must fit in the viewport, or this proves nothing.
  QWidget *fourth = loaded.at(3);
  QPoint bottomLeft(0, fourth->height());
  int bottom = fourth->mapTo(diffView->widget(), bottomLeft).y();
  QVERIFY2(bottom < diffView->viewport()->height(),
           "The first four hunks don't fit without a scrollbar");

  // It should stop once the scrollbar shows rather than load everything.
  QVERIFY(loaded.size() < kHunkCount);
}

TEST_MAIN(TestDiffView)

#include "DiffView.moc"
