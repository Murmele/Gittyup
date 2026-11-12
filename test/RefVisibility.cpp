#include "Test.h"
#include "conf/RefVisibility.h"
#include "git/Reference.h"
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

void TestRefVisibility::cleanupTestCase() {
  QDir configDir(
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
  QVERIFY(configDir.remove("config"));
  QVERIFY(QDir().rmdir(configDir.path()));
}

TEST_MAIN(TestRefVisibility)
#include "RefVisibility.moc"
