//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "dialogs/CloneDialog.h"
#include "dialogs/StartDialog.h"
#include "ui/Footer.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QMenu>
#include <QToolButton>

using namespace Test;
using namespace QTest;

class TestBareRepo : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void checkDir();
  void cleanupTestCase();

private:
  MainWindow *mWindow = nullptr;
};

void TestBareRepo::initTestCase() {
  StartDialog *dialog = StartDialog::openSharedInstance();
  QVERIFY(qWaitForWindowExposed(dialog));

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

  {
    auto timeout = Timeout(1000, "Start dialog hasn't been dismissed in time");

    // Show popup menu.
    mouseClick(plus, Qt::LeftButton);
  }

  CloneDialog *cloneDialog =
      qobject_cast<CloneDialog *>(QApplication::activeModalWidget());
  QVERIFY(cloneDialog);

  // Set fields.
  cloneDialog->setField("name", "test_bare_repo");
  cloneDialog->setField("path", QDir::tempPath());
  cloneDialog->setField("bare", true);
  qWait(2000);
  qDebug() << cloneDialog->field("bare").toBool();

  // Click return.
  keyClick(cloneDialog, Qt::Key_Return);

  // Wait on the new window.
  mWindow = MainWindow::activeWindow();
  QVERIFY(mWindow && qWaitForWindowActive(mWindow));
}

void TestBareRepo::checkDir() {
  RepoView *view = mWindow->currentView();
  QVERIFY(view->repo().isBare());

  QDir dir = QDir::temp();
  QVERIFY(dir.cd("test_bare_repo"));

  QVERIFY(!dir.exists(".git"));

  QVERIFY(dir.exists("config"));
  QVERIFY(dir.exists("objects"));
  QVERIFY(dir.exists("HEAD"));
  QVERIFY(dir.exists("info"));
  QVERIFY(dir.exists("description"));
  QVERIFY(dir.exists("hooks"));
  QVERIFY(dir.exists("refs"));
}

void TestBareRepo::cleanupTestCase() {
  if (mWindow) {
    mWindow->close();
  }
  QDir dir = QDir::temp();
  QVERIFY(dir.cd("test_bare_repo"));
  QVERIFY(dir.removeRecursively());
}

TEST_MAIN(TestBareRepo)

#include "bare_repo.moc"
