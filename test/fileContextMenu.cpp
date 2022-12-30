#include "Test.h"

#include "ui/FileContextMenu.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/IgnoreDialog.h"
#include "git/Reference.h"

#include <QMessageBox>
#include <QPushButton>

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  git::Repository repo = git::Repository::open(path);                          \
  QVERIFY(repo.isValid());                                                     \
  MainWindow window(repo);                                                     \
  window.show();                                                               \
  QVERIFY(QTest::qWaitForWindowExposed(&window));                              \
  RepoView *repoView = window.currentView();

class TestFileContextMenu : public QObject {
  Q_OBJECT

private slots:
  void testDiscardFile();
  void testDiscardSubmodule();
  void testDiscardFolder();
  void testDiscardNothing();
  void testIgnoreFile();
  void testIgnoreFileUntracked();
  void testIgnoreFolder();
  void testRemoveUntrackedFolder();
};

using namespace git;

void TestFileContextMenu::testDiscardFile() {
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
  emit repo.notifier()->referenceUpdated(repo.head());
  QTest::qWait(1000); // Wait until status is finished (own thread executed)

  // let the changes settle
  QApplication::processEvents();

  QStringList files = {"file.txt"};
  FileContextMenu m(repoView, files, repo.index());

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->text() == tr("Discard Changes")) {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QVERIFY(action->isEnabled());
  action->triggered(true);

  auto *msgBox = repoView->findChild<QMessageBox *>();
  QVERIFY(msgBox);
  auto *button = msgBox->findChild<QPushButton *>("DiscardButton");
  QVERIFY(button);
  emit button->clicked(true);

  // original text
  //  {"file.txt", "File.txt\n"},
  //  {"file2.txt", "file2.txt\n"},
  //  {"folder1/file.txt", "file in folder1\n"},
  //  {"folder1/file2.txt", "file2 in folder1\n"},
  //  {"GittyupTestRepo/README.md",
  //   "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},

  {
    QHash<QString, QString> fileContentRef = fileContent;
    fileContentRef.insert("file.txt", "File.txt\n"); // this file was discarded
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

void TestFileContextMenu::testDiscardSubmodule() {
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
  emit repo.notifier()->referenceUpdated(repo.head());
  QTest::qWait(1000); // Wait until status is finished (own thread executed)

  // let the changes settle
  QApplication::processEvents();

  QStringList files = {"GittyupTestRepo"};
  FileContextMenu m(repoView, files, repo.index());

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->text() == tr("Discard Changes")) {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QCOMPARE(action->isEnabled(), true);
  action->triggered(true);

  auto *msgBox = repoView->findChild<QMessageBox *>();
  QVERIFY(msgBox);
  auto *button = msgBox->findChild<QPushButton *>("DiscardButton");
  QVERIFY(button);
  QVERIFY(button->isEnabled());
  emit button->clicked(true);

  // original text
  //  {"file.txt", "File.txt\n"},
  //  {"file2.txt", "file2.txt\n"},
  //  {"folder1/file.txt", "file in folder1\n"},
  //  {"folder1/file2.txt", "file2 in folder1\n"},
  //  {"GittyupTestRepo/README.md",
  //   "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},

  {
    QHash<QString, QString> fileContentRef = fileContent;
    fileContentRef.insert("GittyupTestRepo/README.md",
                          "# GittyupTestRepo\nTest repo for Gittyup used in "
                          "the unittests\n"); // this submodule was discarded
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

void TestFileContextMenu::testDiscardFolder() {
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
  emit repo.notifier()->referenceUpdated(repo.head());
  QTest::qWait(1000); // Wait until status is finished (own thread executed)

  // let the changes settle
  QApplication::processEvents();

  QStringList files = {"folder1"};
  FileContextMenu m(repoView, files, repo.index());

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->text() == tr("Discard Changes")) {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QCOMPARE(action->isEnabled(), true);
  action->triggered(true);

  auto *msgBox = repoView->findChild<QMessageBox *>();
  QVERIFY(msgBox);
  auto *button = msgBox->findChild<QPushButton *>("DiscardButton");
  QVERIFY(button);
  emit button->clicked(true);

  // original text
  //  {"file.txt", "File.txt\n"},
  //  {"file2.txt", "file2.txt\n"},
  //  {"folder1/file.txt", "file in folder1\n"},
  //  {"folder1/file2.txt", "file2 in folder1\n"},
  //  {"GittyupTestRepo/README.md",
  //   "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},

  {
    QHash<QString, QString> fileContentRef = fileContent;
    // files in the folder got discarded
    fileContentRef.insert("folder1/file.txt", "file in folder1\n");
    fileContentRef.insert("folder1/file2.txt", "file2 in folder1\n");
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

void TestFileContextMenu::testDiscardNothing() {
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
  repo.notifier()->referenceUpdated(repo.head());

  // let the changes settle
  QApplication::processEvents();

  QStringList files; // no files passed
  FileContextMenu m(repoView, files, repo.index());

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->text() == tr("Discard Changes")) {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QCOMPARE(action->isEnabled(), false);

  // original text
  //  {"file.txt", "File.txt\n"},
  //  {"file2.txt", "file2.txt\n"},
  //  {"folder1/file.txt", "file in folder1\n"},
  //  {"folder1/file2.txt", "file2 in folder1\n"},
  //  {"GittyupTestRepo/README.md",
  //   "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},

  {
    // No file shall be discarded
    QHash<QString, QString> fileContentRef = fileContent;
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

void TestFileContextMenu::testIgnoreFile() {
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
  repo.notifier()->referenceUpdated(repo.head());

  // let the changes settle
  QApplication::processEvents();

  QStringList files = {"file.txt"};
  FileContextMenu m(repoView, files, repo.index(), repoView);

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->objectName() == "IgnoreAction") {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QCOMPARE(action->isEnabled(), false); // Modified file cannot be ignored
}

void TestFileContextMenu::testIgnoreFileUntracked() {
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

  {
    // Create new file which shall be ignored
    QFile file(repo.workdir().filePath("newFile.txt"));
    QVERIFY(file.open(QFile::WriteOnly));
    QVERIFY(file.write("Content of new file") > 0);
  }

  // refresh repo
  repo.notifier()->referenceUpdated(repo.head());

  // let the changes settle
  QApplication::processEvents();

  QStringList files = {"newFile.txt"};
  FileContextMenu m(repoView, files, repo.index(), repoView);

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->objectName() == "IgnoreAction") {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QVERIFY(action->isEnabled());
  action->triggered(true);

  auto *ignoreDialog = repoView->findChild<IgnoreDialog *>();
  QVERIFY(ignoreDialog);
  emit ignoreDialog->accepted();

  QFile file(repo.workdir().filePath(".gitignore"));
  QVERIFY(file.exists());
  QVERIFY(file.open(QFile::ReadOnly));
  const QString ignores = file.readAll();

#ifdef Q_OS_WIN
  QCOMPARE(ignores, "newFile.txt\r\n");
#else
  QCOMPARE(ignores, "newFile.txt\n");
#endif
}

void TestFileContextMenu::testIgnoreFolder() {
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
  repo.notifier()->referenceUpdated(repo.head());

  // let the changes settle
  QApplication::processEvents();

  QStringList files = {"folder1"};
  FileContextMenu m(repoView, files, repo.index(), repoView);

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->objectName() == "IgnoreAction") {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QVERIFY(action->isEnabled());
  action->triggered(true);

  auto *ignoreDialog = repoView->findChild<IgnoreDialog *>();
  QVERIFY(ignoreDialog);
  emit ignoreDialog->accepted();

  QFile file(repo.workdir().filePath(".gitignore"));
  QVERIFY(file.exists());
  QVERIFY(file.open(QFile::ReadOnly));
  const QString ignores = file.readAll();

#ifdef Q_OS_WIN
  QCOMPARE(ignores, "folder1\r\n");
#else
  QCOMPARE(ignores, "folder1\n");
#endif
}

void TestFileContextMenu::testRemoveUntrackedFolder() {
  INIT_REPO("TestRepository.zip", false);

  git::Commit commit =
      repo.lookupCommit("5c61b24e236310ad4a8a64f7cd1ccc968f1eec20");
  QVERIFY(commit);

  // modifying all files
  QHash<QString, QString> fileContent{
      {"folder_new/file.txt", "Modified file"},
      {"folder_new/file2.txt", "Modified file2"},
  };

  repo.workdir().mkdir("folder_new");

  {
    QHashIterator<QString, QString> i(fileContent);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.open(QFile::WriteOnly));
      file.write(i.value().toLatin1());
    }
  }

  {
    QHashIterator<QString, QString> i(fileContent);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QCOMPARE(file.exists(), true);
    }
  }

  // refresh repo
  repo.notifier()->referenceUpdated(repo.head());

  // let the changes settle
  QApplication::processEvents();

  QStringList files = {"folder_new"};
  FileContextMenu m(repoView, files, repo.index(), repoView);

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->objectName() == "RemoveAction") {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QVERIFY(action->isEnabled());
  action->triggered(true);

  // Click remove in dialog
  auto *msgBox = repoView->findChild<QMessageBox *>();
  QVERIFY(msgBox);
  auto *button = msgBox->findChild<QPushButton *>("RemoveButton");
  QVERIFY(button);
  emit button->clicked(true);

  // Check that files do not exist anymore
  {
    QHashIterator<QString, QString> i(fileContent);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QCOMPARE(file.exists(), false);
    }
  }

  // Check that other files are still available
  {
    // No file shall be discarded
    QHash<QString, QString> fileContentRef = {
        {"file.txt", "File.txt\n"},
        {"file2.txt", "file2.txt\n"},
        {"folder1/file.txt", "file in folder1\n"},
        {"folder1/file2.txt", "file2 in folder1\n"},
        {"GittyupTestRepo/README.md",
         "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},
    };
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

TEST_MAIN(TestFileContextMenu)
#include "fileContextMenu.moc"
