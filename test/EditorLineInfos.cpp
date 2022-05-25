#include "Test.h"
#include "dialogs/ExternalToolsDialog.h"

#include "ui/DiffView/HunkWidget.h"
#include "ui/DiffView/FileWidget.h"

#include "ui/MainWindow.h"
#include "ui/DiffView/DiffView.h"
#include "ui/RepoView.h"
#include <QString>

#include "git/Reference.h"
#include "git/Diff.h"
#include "git/Commit.h"
#include "git/Tree.h"

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  mRepo = git::Repository::open(path);                                         \
  QVERIFY(mRepo.isValid());                                                    \
  auto window = new MainWindow(mRepo);                                         \
                                                                               \
  git::Reference head = mRepo.head();                                          \
  git::Commit commit = head.target();                                          \
  git::Diff stagedDiff = mRepo.diffTreeToIndex(commit.tree()); /* correct */   \
                                                                               \
  RepoView *repoView = window->currentView();                                  \
  Test::refresh(repoView);                                                     \
  DiffView diffView = DiffView(mRepo, repoView);                               \
  auto diff = mRepo.status(mRepo.index(), nullptr, false);

#define BITSET(value, bit) ((value & (1 << bit)) == (1 << bit))

#define checkEditorMarkers(editor, unstagedAddition, stagedAddition,           \
                           unstagedDeletion, stagedDeletion)                   \
  for (int i = 0; i < editor->lineCount(); i++) {                              \
    QString line = editor->line(i);                                            \
    int markers = editor->markers(i);                                          \
                                                                               \
    QString state = "";                                                        \
    if (BITSET(markers, TextEditor::Marker::Addition))                         \
      state = "Addition";                                                      \
    else if (BITSET(markers, TextEditor::Marker::Deletion))                    \
      state = "Deletion";                                                      \
    else if (BITSET(markers, TextEditor::Marker::Context))                     \
      state = "Unchanged";                                                     \
    else                                                                       \
      state = QString::number(markers);                                        \
                                                                               \
    qDebug() << "Index: " << i << ", State: " << state << ", Staged: "         \
             << BITSET(markers, TextEditor::Marker::StagedMarker) << line;     \
    if (unstagedAddition.indexOf(i) != -1) {                                   \
      /* unstaged addition */                                                  \
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));                  \
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));             \
    } else if (stagedAddition.indexOf(i) != -1) {                              \
      /* staged addition */                                                    \
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));                  \
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));              \
    } else if (unstagedDeletion.indexOf(i) != -1) {                            \
      /* unstaged deletion */                                                  \
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));                  \
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));             \
    } else if (stagedDeletion.indexOf(i) != -1) {                              \
      /* staged deletion */                                                    \
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));                  \
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));              \
    } else {                                                                   \
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));                 \
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));                 \
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));             \
    }                                                                          \
  }

/*
 * For DEBUGGING PURPOSE:
 * git diff HEAD: to get the diff shown in the editor
 * git diff --cached: to get the staged diff
 */

#define EXECUTE_ONLY_LAST_TEST 0

using namespace QTest;

class TestEditorLineInfo : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();

  // void editorLineMarkers1();
#if EXECUTE_ONLY_LAST_TEST == 0
  void editorLineSingleHunkAdditionStaged();
  void editorLineSingleHunkDeletionStaged();
  void editorLineSingleHunkChangeStaged();
  void editorLineSingleHunkChange_onlyAdditionStaged();
  void editorLineSingleHunkChange_onlyDeletionStaged();
  void singleHunk_multipleDeletions();
  void singleHunk_multipleAdditions();
  void multipleHunks_multipleDeletions();
  void multipleHunks_multipleAdditions();
  void multipleHunks_misc1();
  void singleHunk_additionsOnly_secondStagedPatch();
  void singleHunk_deletionsOnly_secondStagedPatch();
  void multipleHunks_StageSingleLines();
  void multipleHunks_StageSingleLines2();
#endif

  void windowsCRLF();

private:
  int closeDelay = 0;
  git::Repository mRepo;
};

void TestEditorLineInfo::initTestCase() {}

#if EXECUTE_ONLY_LAST_TEST == 0
void TestEditorLineInfo::editorLineSingleHunkAdditionStaged() {
  INIT_REPO("01_singleHunkAdditionStaged.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    if (i == 3) {
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
      QCOMPARE(line, "3.5\n");
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }
}

void TestEditorLineInfo::editorLineSingleHunkDeletionStaged() {
  INIT_REPO("02_singleHunkDeletionStaged.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    if (i == 2) {
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
      QCOMPARE(line, "3\n");
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }
}

void TestEditorLineInfo::editorLineSingleHunkChangeStaged() {
  INIT_REPO("03_singleHunkChangeSingleLine.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    switch (i) {
      case 2:
        QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
        QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
        QCOMPARE(line, "3\n");
        break;
      case 3:
        QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
        QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
        QCOMPARE(line, "3.5\n");
        break;
      default:
        QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
        QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
        QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
        break;
    }
  }
}

void TestEditorLineInfo::editorLineSingleHunkChange_onlyAdditionStaged() {
  INIT_REPO("04_singleHunkChange_onlyAdditionStaged.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    switch (i) {
      case 2:
        QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
        QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
        QCOMPARE(line, "3\n");
        break;
      case 3:
        QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
        QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
        QCOMPARE(line, "3.5\n");
        break;
      default:
        QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
        QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
        QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
        break;
    }
  }
}

void TestEditorLineInfo::editorLineSingleHunkChange_onlyDeletionStaged() {
  INIT_REPO("05_singleHunkChange_onlyDeletionStaged.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    switch (i) {
      case 2:
        QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
        QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
        QCOMPARE(line, "3\n");
        break;
      case 3:
        QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
        QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
        QCOMPARE(line, "3.5\n");
        break;
      default:
        QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
        QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
        QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
        break;
    }
  }
}

void TestEditorLineInfo::singleHunk_multipleDeletions() {
  INIT_REPO("06_singleHunk_multipleDeletions.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    QVector<int> unstaged({3, 4, 7, 8, 10});
    QVector<int> staged({5, 6, 9});
    if (unstaged.indexOf(i) != -1) {
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (staged.indexOf(i) != -1) {
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }
}

void TestEditorLineInfo::singleHunk_multipleAdditions() {
  INIT_REPO("07_singleHunk_multipleAdditions.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    QVector<int> unstaged({3, 6, 7, 10});
    QVector<int> staged({4, 5, 8, 9, 11});
    if (unstaged.indexOf(i) != -1) {
      // unstaged
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (staged.indexOf(i) != -1) {
      // staged
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }
}

void TestEditorLineInfo::multipleHunks_multipleDeletions() {
  INIT_REPO("08_multipleHunks_multipleDeletions.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    QVector<int> unstaged({3, 6, 7});
    QVector<int> staged({4, 5});
    if (unstaged.indexOf(i) != -1) {
      // unstaged
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (staged.indexOf(i) != -1) {
      // staged
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }

  // Second hunk
  auto hw2 = HunkWidget(&diffView, diff, patch, stagedPatch, 1, false, false,
                        repoView);
  hw2.load(stagedPatch, true);
  editor = hw2.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    QVector<int> unstaged({3, 4, 7, 8});
    QVector<int> staged({5, 6});
    if (unstaged.indexOf(i) != -1) {
      // unstaged
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (staged.indexOf(i) != -1) {
      // staged
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }
}

void TestEditorLineInfo::multipleHunks_multipleAdditions() {
  INIT_REPO("09_multipleHunks_multipleAdditions.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    QVector<int> unstaged({3, 4, 7, 8});
    QVector<int> staged({5, 6, 9});
    if (unstaged.indexOf(i) != -1) {
      // unstaged
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (staged.indexOf(i) != -1) {
      // staged
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }

  // Second hunk
  auto hw2 = HunkWidget(&diffView, diff, patch, stagedPatch, 1, false, false,
                        repoView);
  hw2.load(stagedPatch, true);
  editor = hw2.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    QVector<int> unstaged({3, 6, 7, 8, 10});
    QVector<int> staged({4, 5, 9, 11});
    if (unstaged.indexOf(i) != -1) {
      // unstaged
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (staged.indexOf(i) != -1) {
      // staged
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }
}

void TestEditorLineInfo::singleHunk_additionsOnly_secondStagedPatch() {
  // Testing the finding of the staged patch index

  INIT_REPO("11_singleHunk_additionsOnly_secondStagedPatch.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    QVector<int> unstagedAddition({2});
    QVector<int> stagedAddition({6});
    if (unstagedAddition.indexOf(i) != -1) {
      // unstaged addition
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (stagedAddition.indexOf(i) != -1) {
      // staged addition
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }
}

void TestEditorLineInfo::singleHunk_deletionsOnly_secondStagedPatch() {
  // Testing the finding of the staged patch index

  INIT_REPO("12_singleHunk_deletionsOnly_secondStagedPatch.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    QVector<int> unstagedDeletion({1});
    QVector<int> stagedDeletion({4});
    if (unstagedDeletion.indexOf(i) != -1) {
      // unstaged addition
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (stagedDeletion.indexOf(i) != -1) {
      // staged addition
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }
}

void TestEditorLineInfo::multipleHunks_misc1() {
  INIT_REPO("10_misc.zip", true)
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  git::Patch stagedPatch = stagedDiff.patch(0);

  auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false, false,
                       repoView);
  hw.load(stagedPatch, true);
  auto editor = hw.editor();

  for (int i = 0; i < editor->lineCount(); i++) {
    QString line = editor->line(i);
    int markers = editor->markers(i);

    QVector<int> unstagedAddition({6, 9, 19, 23});
    QVector<int> stagedAddition({24, 25});
    QVector<int> unstagedDeletion({3, 4, 5, 16, 17, 18, 21, 22});
    QVector<int> stagedDeletion({});
    if (unstagedAddition.indexOf(i) != -1) {
      // unstaged addition
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (stagedAddition.indexOf(i) != -1) {
      // staged addition
      QVERIFY(BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (unstagedDeletion.indexOf(i) != -1) {
      // unstaged deletion
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    } else if (stagedDeletion.indexOf(i) != -1) {
      // staged deletion
      QVERIFY(BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(BITSET(markers, TextEditor::Marker::StagedMarker));
    } else {
      QVERIFY(!BITSET(markers, TextEditor::Marker::Deletion));
      QVERIFY(!BITSET(markers, TextEditor::Marker::Addition));
      QVERIFY(!BITSET(markers, TextEditor::Marker::StagedMarker));
    }
  }
}

// void TestEditorLineInfo::editorLineMarkers1() {
//     //INIT_REPO("editorLineMarkers1.zip")
////     QString path =
/// Test::extractRepository("01_singleHunkAdditionStaged.zip"); /
/// QVERIFY(!path.isEmpty()); /    mRepo = git::Repository::open(path); /
/// QVERIFY(mRepo.isValid()); /    auto window = new MainWindow(mRepo);

////    git::Reference head = mRepo.head();
////    git::Commit commit = head.target();
////    git::Diff stagedDiff = mRepo.diffTreeToIndex(commit.tree()); // correct
////    QVERIFY(stagedDiff.count() > 0);

////    RepoView *view = window->currentView();
////    //Test::refresh(view);
////    auto diff = mRepo.status(mRepo.index(), nullptr, false);
/////commit.diff(git::Commit(), -1, false); /    QVERIFY(diff.count() > 0); /
/// git::Patch patch = diff.patch(0); /    auto stagedPatch =
/// stagedDiff.patch(0); /    DiffView diffView = DiffView(mRepo, view);

////    auto hw = HunkWidget(&diffView, diff, patch, stagedPatch, 0, false,
/// false, view); /    hw.load(stagedPatch, true);
//}

void TestEditorLineInfo::multipleHunks_StageSingleLines() {
  INIT_REPO("13_singleHunkNoStaged.zip", true)
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  // no staged lines yet, so no staged patch
  QCOMPARE(mRepo.diffTreeToIndex(commit.tree()).count(), 0);
  git::Patch stagedPatch = git::Patch();

  QString name = patch.name();
  QString path_ = mRepo.workdir().filePath(name);
  bool submodule = mRepo.lookupSubmodule(name).isValid();

  auto *fw = new FileWidget(&diffView, diff, patch, stagedPatch, QModelIndex(),
                            name, path_, submodule);
  fw->setStageState(git::Index::StagedState::Unstaged);

  auto hunks = fw->hunks();
  QVERIFY(hunks.count() == 2);
  for (auto *hunk : hunks)
    hunk->load();

  checkEditorMarkers(hunks.at(0)->editor(),
                     QVector<int>({0, 1, 2, 3, 4, 5, 11}), QVector<int>(),
                     QVector<int>(), QVector<int>());
  checkEditorMarkers(
      hunks.at(1)->editor(),
      QVector<int>({3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 20, 21, 22, 23, 30}),
      QVector<int>({}), QVector<int>({14, 19, 29}), QVector<int>());

  // Stage single lines
  hunks.at(0)->stageSelected(5, 6);

  Test::refresh(repoView);
  stagedDiff = mRepo.diffTreeToIndex(commit.tree()); /* correct */
  diff = mRepo.status(mRepo.index(), nullptr, false);
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  stagedPatch = stagedDiff.patch(0);

  delete fw;
  fw = new FileWidget(&diffView, diff, patch, stagedPatch, QModelIndex(), name,
                      path, submodule);

  // It is important that all hunks are loaded!!!!
  hunks = fw->hunks();
  QVERIFY(hunks.count() == 2);
  for (auto *hunk : hunks)
    hunk->load();

  checkEditorMarkers(hunks.at(0)->editor(), QVector<int>({0, 1, 2, 3, 4, 11}),
                     QVector<int>({5}), QVector<int>(), QVector<int>());
  checkEditorMarkers(
      hunks.at(1)->editor(),
      QVector<int>({3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 20, 21, 22, 23, 30}),
      QVector<int>({}), QVector<int>({14, 19, 29}), QVector<int>());

  hunks.at(0)->stageSelected(4, 5);

  Test::refresh(repoView);
  stagedDiff = mRepo.diffTreeToIndex(commit.tree()); /* correct */
  diff = mRepo.status(mRepo.index(), nullptr, false);
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  stagedPatch = stagedDiff.patch(0);

  delete fw;
  fw = new FileWidget(&diffView, diff, patch, stagedPatch, QModelIndex(), name,
                      path, submodule);

  hunks = fw->hunks();
  QCOMPARE(hunks.count(), 2);
  for (auto *hunk : hunks)
    hunk->load();

  checkEditorMarkers(hunks.at(0)->editor(), QVector<int>({0, 1, 2, 3, 11}),
                     QVector<int>({4, 5}), QVector<int>(), QVector<int>());
  checkEditorMarkers(
      hunks.at(1)->editor(),
      QVector<int>({3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 20, 21, 22, 23, 30}),
      QVector<int>({}), QVector<int>({14, 19, 29}), QVector<int>());

  delete fw;
}

void TestEditorLineInfo::multipleHunks_StageSingleLines2() {
  /*
   * Staging the last line, first only the deleted one
   * and then the added line
   */

  INIT_REPO("13_singleHunkNoStaged.zip", true)
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  // no staged lines yet, so no staged patch
  QCOMPARE(mRepo.diffTreeToIndex(commit.tree()).count(), 0);
  git::Patch stagedPatch = git::Patch();

  QString name = patch.name();
  QString path_ = mRepo.workdir().filePath(name);
  bool submodule = mRepo.lookupSubmodule(name).isValid();

  auto *fw = new FileWidget(&diffView, diff, patch, stagedPatch, QModelIndex(),
                            name, path_, submodule);
  fw->setStageState(git::Index::StagedState::Unstaged);

  auto hunks = fw->hunks();
  QVERIFY(hunks.count() == 2);
  for (auto *hunk : hunks)
    hunk->load();

  checkEditorMarkers(hunks.at(0)->editor(),
                     QVector<int>({0, 1, 2, 3, 4, 5, 11}), QVector<int>(),
                     QVector<int>(), QVector<int>());
  checkEditorMarkers(
      hunks.at(1)->editor(),
      QVector<int>({3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 20, 21, 22, 23, 30}),
      QVector<int>({}), QVector<int>({14, 19, 29}), QVector<int>());

  // Stage single lines
  hunks.at(1)->stageSelected(29, 30);

  Test::refresh(repoView);
  stagedDiff = mRepo.diffTreeToIndex(commit.tree()); /* correct */
  diff = mRepo.status(mRepo.index(), nullptr, false);
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  stagedPatch = stagedDiff.patch(0);

  delete fw;
  fw = new FileWidget(&diffView, diff, patch, stagedPatch, QModelIndex(), name,
                      path, submodule);

  // It is important that all hunks are loaded!!!!
  hunks = fw->hunks();
  QVERIFY(hunks.count() == 2);
  for (auto *hunk : hunks)
    hunk->load();

  checkEditorMarkers(hunks.at(0)->editor(),
                     QVector<int>({0, 1, 2, 3, 4, 5, 11}), QVector<int>(),
                     QVector<int>(), QVector<int>());
  checkEditorMarkers(
      hunks.at(1)->editor(),
      QVector<int>({3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 20, 21, 22, 23, 30}),
      QVector<int>({}), QVector<int>({14, 19}), QVector<int>({29}));

  hunks.at(1)->stageSelected(30, 31);

  Test::refresh(repoView);
  stagedDiff = mRepo.diffTreeToIndex(commit.tree()); /* correct */
  diff = mRepo.status(mRepo.index(), nullptr, false);
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  stagedPatch = stagedDiff.patch(0);

  delete fw;
  fw = new FileWidget(&diffView, diff, patch, stagedPatch, QModelIndex(), name,
                      path, submodule);

  hunks = fw->hunks();
  QCOMPARE(hunks.count(), 2);
  for (auto *hunk : hunks)
    hunk->load();

  checkEditorMarkers(hunks.at(0)->editor(),
                     QVector<int>({0, 1, 2, 3, 4, 5, 11}), QVector<int>(),
                     QVector<int>(), QVector<int>());
  checkEditorMarkers(
      hunks.at(1)->editor(),
      QVector<int>({3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 20, 21, 22, 23}),
      QVector<int>({30}), QVector<int>({14, 19}), QVector<int>({29}));

  delete fw;
}

#endif

void TestEditorLineInfo::windowsCRLF() {
  /*
   * Staging single lines in a file with CRLF instead of single LF
   */

  INIT_REPO("14_windowsCRLF.zip", false)
  QVERIFY(diff.count() > 0);
  git::Patch patch = diff.patch(0);
  // no staged lines yet, so no staged patch
  QCOMPARE(mRepo.diffTreeToIndex(commit.tree()).count(), 0);
  git::Patch stagedPatch = git::Patch();

  QString name = patch.name();
  QString path_ = mRepo.workdir().filePath(name);
  bool submodule = mRepo.lookupSubmodule(name).isValid();

  auto *fw = new FileWidget(&diffView, diff, patch, stagedPatch, QModelIndex(),
							name, path_, submodule);
  fw->setStageState(git::Index::StagedState::Unstaged);

  auto hunks = fw->hunks();
  QVERIFY(hunks.count() == 1);
  hunks[0]->load();

  checkEditorMarkers(hunks.at(0)->editor(),
                     QVector<int>(), QVector<int>(),
                     QVector<int>({3, 4, 5}), QVector<int>());

  // Stage single lines
  hunks[0]->stageSelected(3, 5);

  Test::refresh(repoView);
  stagedDiff = mRepo.diffTreeToIndex(commit.tree()); /* correct */
  diff = mRepo.status(mRepo.index(), nullptr, false);
  QVERIFY(stagedDiff.count() > 0);
  QVERIFY(diff.count() > 0);
  stagedPatch = stagedDiff.patch(0);

  delete fw;
  fw = new FileWidget(&diffView, diff, patch, stagedPatch, QModelIndex(), name,
					  path, submodule);

  hunks = fw->hunks();
  QVERIFY(hunks.count() == 1);
  hunks[0]->load();

  checkEditorMarkers(hunks.at(0)->editor(),
                     QVector<int>(), QVector<int>(),
                     QVector<int>({5}), QVector<int>({3, 4}));

  delete fw;
}


void TestEditorLineInfo::cleanupTestCase() { qWait(closeDelay); }

TEST_MAIN(TestEditorLineInfo)
#include "EditorLineInfos.moc"
