#include "Test.h"

#include "ui/CommitEditor.h"

#include <QTextEdit>

class TestCommitEditor : public QObject {
  Q_OBJECT

private slots:
  void testCreateFileList();
  void applyTemplate1();
  void applyTemplate2();
  void applyTemplate3();
  void applyTemplate4();
  void applyTemplate5();
  void applyTemplate6();
  void applyTemplate7();
};

void TestCommitEditor::testCreateFileList() {
  QCOMPARE(CommitEditor::createFileList({"file.txt"}, 1),
           QStringLiteral("file.txt"));
  QCOMPARE(CommitEditor::createFileList({"file.txt", "file2.txt"}, 1),
           QStringLiteral("file.txt, and 1 more file"));
  QCOMPARE(
      CommitEditor::createFileList({"file.txt", "file2.txt", "file2.txt"}, 1),
      QStringLiteral("file.txt, and 2 more files"));
  QCOMPARE(CommitEditor::createFileList({"file.txt", "file2.txt"}, 2),
           QStringLiteral("file.txt and file2.txt"));
  QCOMPARE(
      CommitEditor::createFileList({"file.txt", "file2.txt", "file2.txt"}, 2),
      QStringLiteral("file.txt, file2.txt, and 1 more file"));
}

void TestCommitEditor::applyTemplate1() {
  Test::ScratchRepository repo;
  CommitEditor e(repo);

  e.applyTemplate(QStringLiteral("Description: %|\nfiles: ${files:3}"), "", "",
                  {"file.txt"});
  QCOMPARE(e.textEdit()->toPlainText(),
           QStringLiteral("Description: \nfiles: file.txt"));
  QCOMPARE(e.textEdit()->textCursor().position(), 13);
}

void TestCommitEditor::applyTemplate2() {
  Test::ScratchRepository repo;
  CommitEditor e(repo);

  // Cursor after inserted files
  e.applyTemplate(
      QStringLiteral("Description: \nfiles: ${files:3}\nCursorPosition: %|"),
      "", "", {"reallylongfilename.txt"});
  QCOMPARE(
      e.textEdit()->toPlainText(),
      QStringLiteral(
          "Description: \nfiles: reallylongfilename.txt\nCursorPosition: "));
  QCOMPARE(e.textEdit()->textCursor().position(), 60);
}

void TestCommitEditor::applyTemplate3() {
  Test::ScratchRepository repo;
  CommitEditor e(repo);

  // author
  e.applyTemplate(
      QStringLiteral("Description: \nauthor: ${author}\nCursorPosition: %|"),
      "john.doe", "", {"file.txt"});
  QCOMPARE(e.textEdit()->toPlainText(),
           QStringLiteral("Description: \nauthor: john.doe\nCursorPosition: "));
  QCOMPARE(e.textEdit()->textCursor().position(), 47);
}

void TestCommitEditor::applyTemplate4() {
  Test::ScratchRepository repo;
  CommitEditor e(repo);

  // branch
  e.applyTemplate(
      QStringLiteral("Description: \nbranch: ${branch}\nCursorPosition: %|"),
      "john.doe", "bugfix/GTTY-01", {"file.txt"});
  QCOMPARE(e.textEdit()->toPlainText(),
           QStringLiteral(
               "Description: \nbranch: bugfix/GTTY-01\nCursorPosition: "));
  QCOMPARE(e.textEdit()->textCursor().position(), 53);
}

void TestCommitEditor::applyTemplate5() {
  Test::ScratchRepository repo;
  CommitEditor e(repo);

  // branch
  e.applyTemplate(
      QStringLiteral("Description: \nissue: "
                     "${branch:(GTTY-[0-9]{2})}\nCursorPosition: %|"),
      "john.doe", "bugfix/GTTY-01", {"file.txt"});
  QCOMPARE(e.textEdit()->toPlainText(),
           QStringLiteral("Description: \nissue: GTTY-01\nCursorPosition: "));
  QCOMPARE(e.textEdit()->textCursor().position(), 45);
}

void TestCommitEditor::applyTemplate6() {
  Test::ScratchRepository repo;
  CommitEditor e(repo);

  // branch (no match)
  e.applyTemplate(
      QStringLiteral(
          "Description: \nissue: ${branch:(XXX-[0-9]{2})}\nCursorPosition: %|"),
      "john.doe", "bugfix/GTTY-01", {"file.txt"});
  QCOMPARE(e.textEdit()->toPlainText(),
           QStringLiteral("Description: \nissue: \nCursorPosition: "));
  QCOMPARE(e.textEdit()->textCursor().position(), 38);
}

void TestCommitEditor::applyTemplate7() {
  Test::ScratchRepository repo;
  CommitEditor e(repo);

  // author, branch and files
  e.applyTemplate(
      QStringLiteral(
          "Description: \nauthor: ${author} \nissue: ${branch:(GTTY-[0-9]{2})} "
          "\nfiles: ${files:2} \nCursorPosition: %|"),
      "john.doe", "bugfix/GTTY-02", {"file1.txt", "file2.txt", "file3.txt"});
  QCOMPARE(e.textEdit()->toPlainText(),
           QStringLiteral(
               "Description: \nauthor: john.doe \nissue: GTTY-02 \nfiles: "
               "file1.txt, file2.txt, and 1 more file \nCursorPosition: "));
  QCOMPARE(e.textEdit()->textCursor().position(), 110);
}

TEST_MAIN(TestCommitEditor)
#include "commitEditor.moc"
