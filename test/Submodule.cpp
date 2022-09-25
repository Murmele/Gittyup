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

#include <QToolButton>
#include <QMenu>
#include <QWizard>
#include <QLineEdit>

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  auto repo = git::Repository::open(path);                                         \
  QVERIFY(repo.isValid());                                                    \
  MainWindow window(repo);                                                    \
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
    settings->setValue("global/autoupdate/enable", true);
    CloneDialog* d = new CloneDialog(CloneDialog::Kind::Clone);

    RepoView* view = nullptr;

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
    for (const auto& s: view->repo().submodules()) {
        QVERIFY(s.isValid());
        QVERIFY(s.isInitialized());
    }
}

void TestSubmodule::noUpdateSubmoduleClone() {
    // Don't update submodules after cloning
    QString remote = Test::extractRepository("SubmoduleTest.zip", true);
    QCOMPARE(remote.isEmpty(), false);

    Settings *settings = Settings::instance();
    settings->setValue("global/autoupdate/enable", false);
    CloneDialog* d = new CloneDialog(CloneDialog::Kind::Clone);

    RepoView* view = nullptr;

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
    for (const auto& s: view->repo().submodules()) {
        QVERIFY(s.isValid());
        QCOMPARE(s.isInitialized(), false);
    }
}

void TestSubmodule::discardFile() {
    // Discarding a file should not reset the submodule
    INIT_REPO("SubmoduleTest.zip", false);

//    {
//        QFile file(repo.workdir().filePath("Readme.md"));
//        QVERIFY(file.open(QFile::WriteOnly));
//        QTextStream(&file) << "This will be a test." << endl;
//        file.close();
//    }

//    {
//        QFile file(repo.workdir().filePath("Gittyup-Test-Module/Readme.d"));
//        QVERIFY(file.open(QFile::WriteOnly));
//        QTextStream(&file) << "Adding Readme to submodule" << endl;
//        file.close();
//    }
}

TEST_MAIN(TestSubmodule)

#include "Submodule.moc"