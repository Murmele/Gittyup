#include "Test.h"

#include "ui/CommitEditor.h"

#include <QTextEdit>

class TestCommitEditor : public QObject {
  Q_OBJECT

private slots:
  void testCreateFileList();
  void applyTemplate1();
  void applyTemplate2();
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

  e.applyTemplate(QStringLiteral("Description: %|\nfiles: ${files:3}"),
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
      {"reallylongfilename.txt"});
  QCOMPARE(
      e.textEdit()->toPlainText(),
      QStringLiteral(
          "Description: \nfiles: reallylongfilename.txt\nCursorPosition: "));
  QCOMPARE(e.textEdit()->textCursor().position(), 60);
}

TEST_MAIN(TestCommitEditor)
#include "commitEditor.moc"
