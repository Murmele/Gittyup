#include "Test.h"
#include "conf/RefVisibility.h"
#include "dialogs/RefVisibilityPanel.h"
#include "git/Branch.h"
#include "git/Reference.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>

using namespace Test;

class TestRefVisibility : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void matching();
  void validation();
  void persistence();
  void protectedRefs();
  void settingsPanel();
  void cleanupTestCase();
};

void TestRefVisibility::initTestCase() {
  // Keep application configuration writes away from the user's settings.
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setApplicationName("Gittyup-RefVisibility-" +
                                       QUuid::createUuid().toString());
}

void TestRefVisibility::matching() {
  RefVisibility::Rules rules(
      {"refs/agents/*", "refs/heads/topic", "refs/tags/v?.0"});
  QVERIFY(rules.hides("refs/agents/nested/task"));
  QVERIFY(rules.hides("refs/heads/topic"));
  QVERIFY(rules.hides("refs/tags/v1.0"));
  QVERIFY(!rules.hides("refs/tags/v10.0"));
  QVERIFY(!rules.hides("refs/heads/topic-more"));
  QVERIFY(!rules.hides("refs/heads/Topic"));
  QVERIFY(!rules.hides("prefix/refs/heads/topic"));
  QVERIFY(!RefVisibility::Rules({}).hides("refs/agents/task"));
}

void TestRefVisibility::validation() {
  QVERIFY(RefVisibility::validationError({"refs/heads/*", "refs/tags/v?.0"})
              .isEmpty());
  QVERIFY(!RefVisibility::validationError({"topic"}).isEmpty());
  QVERIFY(!RefVisibility::validationError({"refs/"}).isEmpty());
  QVERIFY(!RefVisibility::validationError({"refs/heads/a b"}).isEmpty());
  QVERIFY(!RefVisibility::validationError({"refs/heads/a\\b"}).isEmpty());
}

void TestRefVisibility::persistence() {
  ScratchRepository first;
  ScratchRepository second;
  QSignalSpy changes(RefVisibility::instance(), &RefVisibility::changed);
  QCOMPARE(RefVisibility::patterns(first), RefVisibility::defaults());
  QVERIFY(!RefVisibility::hasOverride(first));

  QVERIFY(RefVisibility::save({"refs/heads/global"}));
  QCOMPARE(RefVisibility::patterns(first), QStringList({"refs/heads/global"}));
  QVERIFY(RefVisibility::save({"refs/tags/*"}, first));
  QVERIFY(RefVisibility::hasOverride(first));
  QCOMPARE(RefVisibility::patterns(first), QStringList({"refs/tags/*"}));
  QCOMPARE(RefVisibility::patterns(second), QStringList({"refs/heads/global"}));

  QVERIFY(RefVisibility::save({}, first));
  QVERIFY(RefVisibility::hasOverride(first));
  QVERIFY(RefVisibility::patterns(first).isEmpty());
  QVERIFY(RefVisibility::save({"refs/agents/*"}));
  QVERIFY(RefVisibility::patterns(first).isEmpty());
  QVERIFY(RefVisibility::reset(first));
  QVERIFY(!RefVisibility::hasOverride(first));
  QCOMPARE(RefVisibility::patterns(first), RefVisibility::defaults());
  QVERIFY(RefVisibility::save({}));
  QVERIFY(RefVisibility::patterns(second).isEmpty());
  QVERIFY(RefVisibility::reset());
  QCOMPARE(RefVisibility::patterns(second), RefVisibility::defaults());
  QCOMPARE(changes.count(), 7);
}

void TestRefVisibility::protectedRefs() {
  ScratchRepository repo;
  RefVisibility::Rules rules({"refs/*"});
  QVERIFY(repo->commit("Initial commit").isValid());
  QVERIFY(repo->head().isValid());
  QVERIFY(!rules.hides(repo->head()));
  QVERIFY(!rules.hides(git::Reference()));
}

void TestRefVisibility::settingsPanel() {
  ScratchRepository repo;
  QVERIFY(repo->commit("Initial commit").isValid());
  QVERIFY(repo->createBranch("topic").isValid());
  RefVisibilityPanel panel(repo);
  auto *inherit = panel.findChild<QCheckBox *>("inheritRefVisibility");
  auto *editor = panel.findChild<QPlainTextEdit *>("hiddenRefRules");
  auto *list = panel.findChild<QListWidget *>("refVisibilityList");
  auto *search = panel.findChild<QLineEdit *>("refVisibilitySearch");
  auto *apply = panel.findChild<QPushButton *>("applyRefVisibility");
  auto *showAll = panel.findChild<QPushButton *>("showAllRefs");
  auto *reset = panel.findChild<QPushButton *>("resetRefVisibility");
  QVERIFY(inherit && editor && list && search && apply && showAll && reset);
  QVERIFY(inherit->isChecked());
  QVERIFY(!editor->isEnabled());
  QVERIFY(!apply->isEnabled());
  QCOMPARE(list->count(), 2);

  inherit->setChecked(false);
  QVERIFY(editor->isEnabled());
  search->setText("topic");
  QListWidgetItem *topic = nullptr;
  for (int i = 0; i < list->count(); ++i) {
    if (list->item(i)->text() == "refs/heads/topic")
      topic = list->item(i);
    else
      QVERIFY(list->item(i)->isHidden());
  }
  QVERIFY(topic);
  QVERIFY(!topic->isHidden());
  topic->setCheckState(Qt::Checked);
  QVERIFY(editor->toPlainText().contains("refs/heads/topic"));
  QVERIFY(apply->isEnabled());
  apply->click();
  QVERIFY(RefVisibility::hasOverride(repo));
  QVERIFY(RefVisibility::Rules(RefVisibility::patterns(repo))
              .hides("refs/heads/topic"));
  QVERIFY(!apply->isEnabled());

  editor->setPlainText("invalid");
  QVERIFY(!apply->isEnabled());
  showAll->click();
  QVERIFY(apply->isEnabled());
  apply->click();
  QVERIFY(RefVisibility::hasOverride(repo));
  QVERIFY(RefVisibility::patterns(repo).isEmpty());
  reset->click();
  QVERIFY(inherit->isChecked());
  QVERIFY(!RefVisibility::hasOverride(repo));
  QCOMPARE(RefVisibility::patterns(repo), RefVisibility::defaults());

  RefVisibilityPanel global;
  auto *globalEditor = global.findChild<QPlainTextEdit *>("hiddenRefRules");
  auto *globalApply = global.findChild<QPushButton *>("applyRefVisibility");
  QVERIFY(globalEditor && globalApply);
  globalEditor->setPlainText("refs/tags/*");
  globalApply->click();
  QCOMPARE(RefVisibility::patterns(repo), QStringList({"refs/tags/*"}));
  QVERIFY(inherit->isChecked());
  QVERIFY(RefVisibility::reset());
}

void TestRefVisibility::cleanupTestCase() {
  QDir configDir(
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
  QVERIFY(configDir.remove("config"));
  QVERIFY(QDir().rmdir(configDir.path()));
}

TEST_MAIN(TestRefVisibility)
#include "RefVisibility.moc"
