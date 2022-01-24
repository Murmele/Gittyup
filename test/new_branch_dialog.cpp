//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "dialogs/NewBranchDialog.h"
#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMainWindow>

using namespace Test;
using namespace QTest;

class TestNewBranchDialog : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void verifyName();
  void cleanupTestCase();

private:
  int closeDelay = 0;
  
  ScratchRepository mRepo;
  QMainWindow *mWindow = nullptr;
};

void TestNewBranchDialog::initTestCase()
{
  mWindow = new QMainWindow;
  NewBranchDialog *dialog = new NewBranchDialog(mRepo, git::Commit(), mWindow);
  dialog->show();
  QVERIFY(qWaitForWindowActive(dialog));
}

void TestNewBranchDialog::verifyName()
{
  QLineEdit *nameField = mWindow->findChild<QLineEdit *>();
  QDialogButtonBox *buttons = mWindow->findChild<QDialogButtonBox *>();
  QVERIFY(nameField && buttons);

  // Find the button with the accept role.
  auto button_list = buttons->buttons();
  auto end = button_list.end();
  auto begin = button_list.begin();
  auto it = std::find_if(begin, end, [buttons](QAbstractButton *button) {
    return buttons->buttonRole(button) == QDialogButtonBox::AcceptRole;
  });

  QVERIFY(it != end);
  QAbstractButton *accept = *it;

  keyClicks(nameField, "valid");
  QVERIFY(accept->isEnabled());
  
  keyClick(nameField, 'a', Qt::ControlModifier);
  keyClick(nameField, Qt::Key_Delete);
  
  keyClicks(nameField, "Invalid Name");
  QVERIFY(!accept->isEnabled());
}

void TestNewBranchDialog::cleanupTestCase()
{
  qWait(closeDelay);
  mWindow->close();
}

TEST_MAIN(TestNewBranchDialog)

#include "new_branch_dialog.moc"
