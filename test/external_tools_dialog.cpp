//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "dialogs/ExternalToolsDialog.h"
#include <QMainWindow>
#include <QPlainTextEdit>

using namespace Test;
using namespace QTest;

class TestExternalToolsDialog : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();

private:
  int closeDelay = 0;
};

void TestExternalToolsDialog::initTestCase() {
  ExternalToolsDialog *dialog = new ExternalToolsDialog("diff");
  dialog->show();
  QVERIFY(qWaitForWindowActive(dialog));
}

void TestExternalToolsDialog::cleanupTestCase() { qWait(closeDelay); }

TEST_MAIN(TestExternalToolsDialog)

#include "external_tools_dialog.moc"
