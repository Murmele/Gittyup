#include "Test.h"
#include "conf/RefVisibility.h"
#include "dialogs/RefVisibilityPanel.h"
#include "git/Branch.h"
#include "git/Reference.h"
#include "git/TagRef.h"
#include "ui/CommitList.h"
#include "ui/ConfigKeys.h"
#include "ui/MainWindow.h"
#include "ui/ReferenceModel.h"
#include "ui/ReferenceWidget.h"
#include "ui/RepoView.h"
#include <QLabel>
#include <QPainter>
#include <QProcess>
#include <QStyledItemDelegate>
#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <memory>

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
  void navigation();
  void historyAndBadges();
  void writeFailures();
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
  QVERIFY(RefVisibility::reset(first));
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
  QCOMPARE(changes.count(), 8);
}

void TestRefVisibility::protectedRefs() {
  ScratchRepository repo;
  RefVisibility::Rules rules({"refs/*"});
  QVERIFY(repo->commit("Initial commit").isValid());
  QVERIFY(repo->head().isValid());
  QVERIFY(!rules.hides(repo->head()));
  QVERIFY(!rules.hides(git::Reference()));
  QVERIFY(repo->setHeadDetached(repo->head().target()));
  QVERIFY(repo->head().isDetachedHead());
  QVERIFY(!rules.hides(repo->head()));
  QCOMPARE(
      QProcess::execute(GIT_EXECUTABLE,
                        {"-C", repo->workdir().path(), "update-ref",
                         "refs/stash", repo->head().target().id().toString()}),
      0);
  QVERIFY(repo->stashRef().isValid());
  QVERIFY(!rules.hides(repo->stashRef()));
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

void TestRefVisibility::navigation() {
  ScratchRepository repo;
  QVERIFY(repo->commit("Initial commit").isValid());
  const git::Branch topic = repo->createBranch("topic");
  QVERIFY(topic.isValid());
  const git::TagRef tag = repo->createTag(repo->head().target(), "v1");
  QVERIFY(tag.isValid());
  QCOMPARE(QProcess::execute(GIT_EXECUTABLE,
                             {"-C", repo->workdir().path(), "update-ref",
                              "refs/remotes/origin/topic",
                              repo->head().target().id().toString()}),
           0);
  repo->appConfig().setValue(ConfigKeys::kRefsKey, false);
  ReferenceWidget navigation(repo);
  auto *model = navigation.findChild<ReferenceModel *>();
  QVERIFY(model);
  model->update();
  navigation.select(topic);
  QCOMPARE(navigation.currentReference().qualifiedName(),
           topic.qualifiedName());
  QSignalSpy changes(&navigation, &ReferenceWidget::referenceChanged);
  auto contains = [](ReferenceModel *model, const QString &name) {
    for (int section = 0; section < model->rowCount(); ++section) {
      const QModelIndex parent = model->index(section, 0);
      for (int row = 0; row < model->rowCount(parent); ++row) {
        const auto ref = model->index(row, 0, parent)
                             .data(Qt::UserRole)
                             .value<git::Reference>();
        if (ref.qualifiedName() == name)
          return true;
      }
    }
    return false;
  };
  QVERIFY(contains(model, topic.qualifiedName()));
  QVERIFY(contains(model, tag.qualifiedName()));
  QVERIFY(contains(model, "refs/remotes/origin/topic"));
  QVERIFY(RefVisibility::save(
      {"refs/heads/*", "refs/tags/*", "refs/remotes/origin/*"}, repo));
  QVERIFY(!contains(model, topic.qualifiedName()));
  QVERIFY(!contains(model, tag.qualifiedName()));
  QVERIFY(!contains(model, "refs/remotes/origin/topic"));
  QVERIFY(contains(model, repo->head().qualifiedName()));
  QCOMPARE(navigation.currentReference().qualifiedName(),
           topic.qualifiedName());
  QCOMPARE(changes.count(), 0);
  const auto labels = navigation.findChildren<QLabel *>();
  QVERIFY(std::any_of(labels.cbegin(), labels.cend(), [](QLabel *label) {
    return label->text().contains("(Hidden reference)");
  }));

  ReferenceModel operations(repo, ReferenceView::AllRefs);
  operations.update();
  QVERIFY(contains(&operations, topic.qualifiedName()));
  QVERIFY(contains(&operations, tag.qualifiedName()));
  QVERIFY(contains(&operations, "refs/remotes/origin/topic"));
  QVERIFY(RefVisibility::save({}, repo));
  QVERIFY(contains(model, topic.qualifiedName()));
  QCOMPARE(navigation.currentReference().qualifiedName(),
           topic.qualifiedName());
  QCOMPARE(changes.count(), 0);
}

void TestRefVisibility::historyAndBadges() {
  ScratchRepository scratch;
  git::Repository repo = scratch;
  const git::Commit base = repo.commit("Initial commit");
  QVERIFY(base.isValid());
  const git::Reference head = repo.head();
  const git::Branch topic = repo.createBranch("topic");
  QVERIFY(repo.setHead(topic));
  const git::Commit extra = repo.commit("Only reachable through hidden refs");
  QVERIFY(extra.isValid());
  QVERIFY(repo.setHead(head));
  QCOMPARE(QProcess::execute(GIT_EXECUTABLE,
                             {"-C", repo.workdir().path(), "update-ref",
                              "refs/agents/task", extra.id().toString()}),
           0);
  QVERIFY(RefVisibility::save({}, repo));
  auto window = std::make_unique<MainWindow>(repo);
  window->show();
  RepoView *view = window->currentView();
  auto *commits = view->findChild<CommitList *>();
  QVERIFY(commits);
  QTRY_VERIFY_WITH_TIMEOUT(!commits->isLoading(), 10000);
  QAbstractItemModel *model = commits->model();
  auto snapshot = [model] {
    QVariantList rows;
    for (int row = 0; row < model->rowCount(); ++row) {
      const QModelIndex index = model->index(row, 0);
      const auto commit =
          index.data(CommitList::CommitRole).value<git::Commit>();
      rows.append(QVariant(
          QVariantList{commit.isValid() ? commit.id().toString() : QString(),
                       index.data(CommitList::GraphRole),
                       index.data(CommitList::GraphColorRole)}));
    }
    return rows;
  };
  const QVariantList before = snapshot();
  QModelIndex extraIndex;
  for (int row = 0; row < model->rowCount(); ++row) {
    const QModelIndex index = model->index(row, 0);
    const auto commit = index.data(CommitList::CommitRole).value<git::Commit>();
    if (commit.isValid() && commit.id() == extra.id())
      extraIndex = index;
  }
  QVERIFY(extraIndex.isValid());
  auto paint = [commits, extraIndex] {
    QImage image(1200, 120, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    QStyleOptionViewItem option;
    option.initFrom(commits);
    option.widget = commits;
    option.rect = image.rect();
    commits->itemDelegate()->paint(&painter, option, extraIndex);
    return image;
  };
  commits->selectReference(repo.lookupRef("refs/agents/task"));
  QTRY_VERIFY_WITH_TIMEOUT(!view->commits().isEmpty(), 10000);
  QTRY_COMPARE_WITH_TIMEOUT(view->commits().first().id(), extra.id(), 10000);
  auto *badge = view->findChild<QWidget *>("commitRefs");
  QVERIFY(badge);
  QTRY_VERIFY_WITH_TIMEOUT(badge->sizeHint().width() > 0, 10000);
  const int widthBefore = badge->sizeHint().width();
  const QImage paintedBefore = paint();
  QSignalSpy resets(model, &QAbstractItemModel::modelReset);
  QSignalSpy loading(commits, &CommitList::loadingChanged);
  QVERIFY(RefVisibility::save({"refs/agents/*", "refs/heads/topic"}, repo));
  QCOMPARE(snapshot(), before);
  QCOMPARE(resets.count(), 0);
  QCOMPARE(loading.count(), 0);
  QVERIFY(badge->sizeHint().width() < widthBefore);
  QVERIFY(paint() != paintedBefore);
  QVERIFY(repo.lookupRef("refs/agents/task").isValid());
  QVERIFY(repo.lookupRef("refs/heads/topic").isValid());
  QVERIFY(RefVisibility::save({}, repo));
  QCOMPARE(snapshot(), before);
  QCOMPARE(badge->sizeHint().width(), widthBefore);
  QCOMPARE(paint(), paintedBefore);
}

void TestRefVisibility::writeFailures() {
  ScratchRepository repo;
  const QStringList before = RefVisibility::patterns();
  QFile file(repo->appDir().filePath("config"));
  QVERIFY(file.open(QFile::WriteOnly));
  QVERIFY(file.write("[invalid\n") > 0);
  file.close();
  QSignalSpy changes(RefVisibility::instance(), &RefVisibility::changed);
  const git::Result save = RefVisibility::save({}, repo);
  QVERIFY(!save);
  QVERIFY(!save.errorString().isEmpty());
  const git::Result reset = RefVisibility::reset(repo);
  QVERIFY(!reset);
  QVERIFY(!reset.errorString().isEmpty());
  QCOMPARE(RefVisibility::patterns(), before);
  QCOMPARE(changes.count(), 0);
}

void TestRefVisibility::cleanupTestCase() {
  QDir configDir(
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
  QVERIFY(configDir.remove("config"));
  if (configDir.exists("user.dic"))
    QVERIFY(configDir.remove("user.dic"));
  QVERIFY(QDir().rmdir(configDir.path()));
}

TEST_MAIN(TestRefVisibility)
#include "RefVisibility.moc"
