#include "Test.h"

#include "ui/TemplateButton.h"
#include "ui/TemplateDialog.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>

class TestCommitMessageTemplate : public QObject {
  Q_OBJECT

private slots:
  void testImportExport();
  void testMoveUp();
  void testMoveDown();
  void testRemove();
  void testRemoveNoItemAvailable();
};

void TestCommitMessageTemplate::testImportExport() {
  QList<TemplateButton::Template> templates;
  QTemporaryFile f;
  QVERIFY(f.open());
  QVERIFY(!f.fileName().isEmpty());

  {
    TemplateDialog d(templates);

    d.mName->setText(QStringLiteral("Template1"));
    d.mTemplate->setPlainText(QStringLiteral("ContentTemplate1:"));
    d.addTemplate();

    d.mName->setText(QStringLiteral("Template2"));
    d.mTemplate->setPlainText(
        QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));
    d.addTemplate();
    d.exportTemplates(f.fileName());
  }

  {
    TemplateDialog d(templates);
    QCOMPARE(d.mTemplates.length(), 0);
    QCOMPARE(d.mTemplateList->count(), 0);
    d.importTemplates(f.fileName());
    d.applyTemplates();
    QCOMPARE(d.mTemplates.length(), 2);
    QCOMPARE(d.mTemplates.at(0).name, QStringLiteral("Template1"));
    QCOMPARE(d.mTemplates.at(0).value, QStringLiteral("ContentTemplate1:"));
    QCOMPARE(d.mTemplates.at(1).name, QStringLiteral("Template2"));
    QCOMPARE(d.mTemplates.at(1).value,
             QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));

    QCOMPARE(d.mTemplateList->count(), 2);
  }
}

void TestCommitMessageTemplate::testMoveUp() {
  QList<TemplateButton::Template> templates;
  TemplateDialog d(templates);

  d.mName->setText(QStringLiteral("Template1"));
  d.mTemplate->setPlainText(QStringLiteral("ContentTemplate1:"));
  d.addTemplate();

  d.mName->setText(QStringLiteral("Template2"));
  d.mTemplate->setPlainText(
      QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));
  d.addTemplate();

  d.mName->setText(QStringLiteral("Template3"));
  d.mTemplate->setPlainText(QStringLiteral("ContentTemplate3:"));
  d.addTemplate();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template2"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template3"));
  QCOMPARE(d.mTemplateList->count(), 3);
  QCOMPARE(d.mTemplateList->item(0)->text(), QStringLiteral("Template1"));
  QCOMPARE(d.mTemplateList->item(1)->text(), QStringLiteral("Template2"));
  QCOMPARE(d.mTemplateList->item(2)->text(), QStringLiteral("Template3"));

  d.mTemplateList->setCurrentRow(1);
  d.moveTemplateUp();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template2"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template3"));
  QCOMPARE(d.mTemplateList->count(), 3);
  QCOMPARE(d.mTemplateList->item(0)->text(), QStringLiteral("Template2"));
  QCOMPARE(d.mTemplateList->item(1)->text(), QStringLiteral("Template1"));
  QCOMPARE(d.mTemplateList->item(2)->text(), QStringLiteral("Template3"));

  d.mTemplateList->setCurrentRow(0);
  d.moveTemplateUp();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template2"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template3"));
  QCOMPARE(d.mTemplateList->count(), 3);
  QCOMPARE(d.mTemplateList->item(0)->text(), QStringLiteral("Template2"));
  QCOMPARE(d.mTemplateList->item(1)->text(), QStringLiteral("Template1"));
  QCOMPARE(d.mTemplateList->item(2)->text(), QStringLiteral("Template3"));
}

void TestCommitMessageTemplate::testMoveDown() {
  QList<TemplateButton::Template> templates;
  TemplateDialog d(templates);

  d.mName->setText(QStringLiteral("Template1"));
  d.mTemplate->setPlainText(QStringLiteral("ContentTemplate1:"));
  d.addTemplate();

  d.mName->setText(QStringLiteral("Template2"));
  d.mTemplate->setPlainText(
      QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));
  d.addTemplate();

  d.mName->setText(QStringLiteral("Template3"));
  d.mTemplate->setPlainText(QStringLiteral("ContentTemplate3:"));
  d.addTemplate();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template2"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template3"));
  QCOMPARE(d.mTemplateList->count(), 3);
  QCOMPARE(d.mTemplateList->item(0)->text(), QStringLiteral("Template1"));
  QCOMPARE(d.mTemplateList->item(1)->text(), QStringLiteral("Template2"));
  QCOMPARE(d.mTemplateList->item(2)->text(), QStringLiteral("Template3"));

  d.mTemplateList->setCurrentRow(1);
  d.moveTemplateDown();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template3"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template2"));
  QCOMPARE(d.mTemplateList->count(), 3);
  QCOMPARE(d.mTemplateList->item(0)->text(), QStringLiteral("Template1"));
  QCOMPARE(d.mTemplateList->item(1)->text(), QStringLiteral("Template3"));
  QCOMPARE(d.mTemplateList->item(2)->text(), QStringLiteral("Template2"));

  d.mTemplateList->setCurrentRow(2);
  d.moveTemplateDown();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template3"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template2"));
  QCOMPARE(d.mTemplateList->count(), 3);
  QCOMPARE(d.mTemplateList->item(0)->text(), QStringLiteral("Template1"));
  QCOMPARE(d.mTemplateList->item(1)->text(), QStringLiteral("Template3"));
  QCOMPARE(d.mTemplateList->item(2)->text(), QStringLiteral("Template2"));
}

void TestCommitMessageTemplate::testRemove() {
  QList<TemplateButton::Template> templates;
  TemplateDialog d(templates);

  d.mName->setText(QStringLiteral("Template1"));
  d.mTemplate->setPlainText(QStringLiteral("ContentTemplate1:"));
  d.addTemplate();

  d.mName->setText(QStringLiteral("Template2"));
  d.mTemplate->setPlainText(
      QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));
  d.addTemplate();

  d.mName->setText(QStringLiteral("Template3"));
  d.mTemplate->setPlainText(QStringLiteral("ContentTemplate3:"));
  d.addTemplate();

  d.mTemplateList->setCurrentRow(1);
  d.removeTemplate();

  QCOMPARE(d.mNew.count(), 2);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template3"));
  QCOMPARE(d.mTemplateList->count(), 2);
  QCOMPARE(d.mTemplateList->item(0)->text(), QStringLiteral("Template1"));
  QCOMPARE(d.mTemplateList->item(1)->text(), QStringLiteral("Template3"));

  QCOMPARE(d.mName->text(), QStringLiteral("Template3"));
  QCOMPARE(d.mTemplate->toPlainText(), QStringLiteral("ContentTemplate3:"));
}

void TestCommitMessageTemplate::testRemoveNoItemAvailable() {
  QList<TemplateButton::Template> templates;
  TemplateDialog d(templates);
  d.removeTemplate();

  // Should not crash!
}

TEST_MAIN(TestCommitMessageTemplate)
#include "commitMessageTemplate.moc"
