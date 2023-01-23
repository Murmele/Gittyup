//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "dialogs/ConfigDialog.h"
#include "ui/Footer.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QComboBox>
#include <QDialog>
#include <QMenu>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableView>
#include <QToolButton>

using namespace Test;
using namespace QTest;

class TestBranchesPanel : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void createBranch();
  void cleanupTestCase();

private:
  int inputDelay = 0;
  int closeDelay = 0;

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  ConfigDialog *mConfigDialog = nullptr;
};

void TestBranchesPanel::initTestCase() {
  mWindow = new MainWindow(mRepo);
  RepoView *view = mWindow->currentView();

  git::Remote remote = mRepo->addRemote(
      "origin", "https://github.com/Murmele/GittyupTestRepo.git");
  fetch(view, remote);

  git::Branch upstream =
      mRepo->lookupBranch("origin/master", GIT_BRANCH_REMOTE);
  QVERIFY(upstream.isValid());

  git::Branch branch =
      view->createBranch("master", upstream.target(), upstream, true);
  QVERIFY(branch.isValid());

  mWindow->show();
  mConfigDialog = view->configureSettings(ConfigDialog::Branches);
  QVERIFY(qWaitForWindowExposed(mConfigDialog));
}

void TestBranchesPanel::createBranch() {
  QStackedWidget *stack = mConfigDialog->findChild<QStackedWidget *>();
  QVERIFY(stack);

  // Click add branch icon
  QWidget *panel = stack->currentWidget();
  Footer *remotesFooter = panel->findChild<Footer *>();
  QToolButton *addRemote = remotesFooter->findChild<QToolButton *>();
  QVERIFY(addRemote);
  mouseClick(addRemote, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(),
             inputDelay);

  // Click upstream combobox
  QComboBox *referenceList = panel->findChild<QComboBox *>();
  QVERIFY(referenceList);
  mouseClick(referenceList, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(),
             inputDelay);

  // Select upstream test_remote/master
  auto *menu = qobject_cast<QFrame *>(QApplication::activePopupWidget());
  QVERIFY(menu);
  keyClick(menu, Qt::Key_Down, Qt::NoModifier, inputDelay);
  keyClick(menu, Qt::Key_Return, Qt::NoModifier, inputDelay);

  // Click Accept
  QDialog *newBranchDialog = panel->findChild<QDialog *>();
  QVERIFY(newBranchDialog);
  QList<QPushButton *> buttons = newBranchDialog->findChildren<QPushButton *>();
  QVERIFY(buttons.count() >= 2);
  QPushButton *createBranch = buttons.at(1);
  QVERIFY(createBranch);
  mouseClick(createBranch, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(),
             inputDelay);

  // Verify branch created
  QTableView *branchTable = panel->findChild<QTableView *>();
  QVERIFY(branchTable);
  QVERIFY(branchTable->rowAt(0) != -1);
}

void TestBranchesPanel::cleanupTestCase() {
  qWait(closeDelay);
  mWindow->close();
}

TEST_MAIN(TestBranchesPanel)

#include "branches_panel.moc"
