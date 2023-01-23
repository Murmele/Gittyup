#include "Test.h"

#include "git/Signature.h"
#include "git/Reference.h"
#include "git/Tree.h"
#include "ui/MainWindow.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/RepoView.h"
#include "ui/TreeView.h"
#include "dialogs/AmendDialog.h"

#include <QTextEdit>
#include <QRadioButton>
#include <QDateTimeEdit>
#include <QLineEdit>

#define INIT_REPO(repoPath, /* bool */ useTempDir)                             \
  QString path = Test::extractRepository(repoPath, useTempDir);                \
  QVERIFY(!path.isEmpty());                                                    \
  git::Repository repo = git::Repository::open(path);                          \
  QVERIFY(repo.isValid());                                                     \
  Test::initRepo(repo);

class TestAmend : public QObject {
  Q_OBJECT

private slots:
  void testAmend();
  void testAmendAddFile();
  void testAmendDialog();
  void testAmendDialog2();
  void testAmendDialogNewLineInMessage();
};

using namespace git;

void TestAmend::testAmend() {
  INIT_REPO("CherryPickAuthorEmail.zip", true);

  git::Reference master = repo.lookupRef(QString("refs/heads/master"));
  QVERIFY(master.isValid());
  auto c = master.annotatedCommit().commit();
  QCOMPARE(c.message(), "changes");
  QCOMPARE(c.author().email(), "martin.marmsoler@gmail.com");
  QCOMPARE(c.author().name(), "Martin Marmsoler");
  QCOMPARE(
      c.author().date(),
      QDateTime::fromString("Sun May 22 10:36:26 2022 +0200", Qt::RFC2822Date));
  QCOMPARE(c.committer().name(), "Martin Marmsoler");
  QCOMPARE(c.committer().email(), "martin.marmsoler@gmail.com");
  QCOMPARE(
      c.committer().date(),
      QDateTime::fromString("Sun May 22 10:36:26 2022 +0200", Qt::RFC2822Date));

  const QString commitMessage = "New commit message";

  auto authorSignature = repo.signature(
      "New Author", "New Author Email",
      QDateTime::fromString("Sun May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  auto committerSignature = repo.signature(
      "New Committer", "New Committer Email",
      QDateTime::fromString("Sun May 23 11:36:26 2022 +0200", Qt::RFC2822Date));

  Tree tree;
  c.amend(authorSignature, committerSignature, commitMessage, tree);

  master = repo.lookupRef(QString("refs/heads/master"));
  QVERIFY(master.isValid());
  c = master.annotatedCommit().commit();
  QCOMPARE(c.message(), "New commit message");
  QCOMPARE(c.author().email(), "New Author Email");
  QCOMPARE(c.author().name(), "New Author");
  QCOMPARE(
      c.author().date(),
      QDateTime::fromString("Sun May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  QCOMPARE(c.committer().name(), "New Committer");
  QCOMPARE(c.committer().email(), "New Committer Email");
  QCOMPARE(
      c.committer().date(),
      QDateTime::fromString("Sun May 23 11:36:26 2022 +0200", Qt::RFC2822Date));
}

void TestAmend::testAmendAddFile() {

  // Create repo
  Test::ScratchRepository mRepo;
  auto mMainBranch = mRepo->unbornHeadName();
  auto mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(QTest::qWaitForWindowExposed(mWindow));
  RepoView *view = mWindow->currentView();

  {
    // Add file and refresh.
    QFile file(mRepo->workdir().filePath("test"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "This will be a test." << endl;

    Test::refresh(view);

    auto doubleTree = view->findChild<DoubleTreeWidget *>();
    QVERIFY(doubleTree);

    auto files = doubleTree->findChild<TreeView *>("Unstaged");
    QVERIFY(files);

    QAbstractItemModel *model = files->model();
    QCOMPARE(model->rowCount(), 1);

    // Click on the check box.
    QModelIndex index = model->index(0, 0);
    QTest::mouseClick(files->viewport(), Qt::LeftButton,
                      Qt::KeyboardModifiers(),
                      files->checkRect(index).center());

    // Commit and refresh.
    QTextEdit *editor = view->findChild<QTextEdit *>("MessageEditor");
    QVERIFY(editor);

    editor->setText("base commit");
    view->commit();
    Test::refresh(view, false);

    // Create branch and stage changes
    git::Branch branch2 =
        mRepo->createBranch("branch2", mRepo->head().target());
    QVERIFY(branch2.isValid());

    view->checkout(branch2);
    QCOMPARE(mRepo->head().name(), QString("branch2"));

    // Check if file has correct content
    QVERIFY(branch2.isValid());
    auto c = branch2.annotatedCommit().commit();
    QCOMPARE(c.blob("test").content(), "This will be a test.\n");
  }

  // Stage file with changes
  {
    QFile file(mRepo->workdir().filePath("test"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "Changes made" << endl;

    Test::refresh(view);

    auto doubleTree = view->findChild<DoubleTreeWidget *>();
    QVERIFY(doubleTree);

    // Staging the file
    auto files = doubleTree->findChild<TreeView *>("Unstaged");
    QVERIFY(files);

    QAbstractItemModel *model = files->model();
    QCOMPARE(model->rowCount(), 1);

    // Click on the check box. to stage file
    QModelIndex index = model->index(0, 0);
    QTest::mouseClick(files->viewport(), Qt::LeftButton,
                      Qt::KeyboardModifiers(),
                      files->checkRect(index).center());
  }

  // Check that changes applied after amending
  {
    // Amend changes
    git::Reference branch2 = mRepo->lookupRef(QString("refs/heads/branch2"));
    QVERIFY(branch2.isValid());
    auto c = branch2.annotatedCommit().commit();
    auto authorSignature = mRepo->signature("New Author", "New Author Email");
    auto committerSignature =
        mRepo->signature("New Committer", "New Committer Email");
    QCOMPARE(mRepo->amend(c, authorSignature, committerSignature,
                          "New commit message"),
             true);

    {
      branch2 = mRepo->lookupRef(QString("refs/heads/branch2"));
      QVERIFY(branch2.isValid());
      c = branch2.annotatedCommit().commit();
      QCOMPARE(c.message(), "New commit message");
      QCOMPARE(c.author().email(), "New Author Email");
      QCOMPARE(c.author().name(), "New Author");
      QCOMPARE(c.committer().name(), "New Committer");
      QCOMPARE(c.committer().email(), "New Committer Email");

      QCOMPARE(c.blob("test").content(), "Changes made\n");
    }
  }
}

void TestAmend::testAmendDialog() {
  // Checking that original information is passed to the return function
  // correctly Name and email will not be changed. Only different datetypes are
  // tested
  Test::ScratchRepository repo;
  auto authorSignature = repo->signature(
      "New Author", "New Author Email",
      QDateTime::fromString("Sun May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  auto committerSignature = repo->signature(
      "New Committer", "New Committer Email",
      QDateTime::fromString("Sun May 23 11:36:26 2022 +0200", Qt::RFC2822Date));

  {
    AmendDialog d(authorSignature, committerSignature, "Test commit message");
    d.show();

    {
      const auto *authorCommitDateTimeTypeSelection =
          d.findChild<QWidget *>(tr("Author") + "CommitDateType");
      QVERIFY(authorCommitDateTimeTypeSelection);
      auto *authorCurrent =
          authorCommitDateTimeTypeSelection->findChild<QRadioButton *>(
              "Current");
      QVERIFY(authorCurrent);
      auto *authorOriginal =
          authorCommitDateTimeTypeSelection->findChild<QRadioButton *>(
              "Original");
      QVERIFY(authorOriginal);
      auto *authorManual =
          authorCommitDateTimeTypeSelection->findChild<QRadioButton *>(
              "Manual");
      QVERIFY(authorManual);
      auto *authorCommitDate =
          d.findChild<QDateTimeEdit *>(tr("Author") + "CommitDate");
      QVERIFY(authorCommitDate);
      authorCommitDate->setDateTime(
          QDateTime(QDate(2012, 7, 6), QTime(8, 30, 5)));

      // current
      authorCurrent->click();
      auto info = d.getInfo();
      QCOMPARE(info.authorInfo.commitDateType,
               ContributorInfo::SelectedDateTimeType::Current);
      QCOMPARE(authorCommitDate->isVisible(), false);

      // original
      authorOriginal->click();
      info = d.getInfo();
      QCOMPARE(info.authorInfo.commitDateType,
               ContributorInfo::SelectedDateTimeType::Original);
      QCOMPARE(authorCommitDate->isVisible(), false);
      QCOMPARE(info.authorInfo.commitDate,
               QDateTime::fromString("Sun May 23 10:36:26 2022 +0200",
                                     Qt::RFC2822Date));

      // manual
      authorManual->click();
      info = d.getInfo();
      QCOMPARE(info.authorInfo.commitDateType,
               ContributorInfo::SelectedDateTimeType::Manual);
      QCOMPARE(authorCommitDate->isVisible(), true);
      QCOMPARE(info.authorInfo.commitDate,
               QDateTime(QDate(2012, 7, 6), QTime(8, 30, 5)));
    }

    {
      const auto *committerCommitDateTimeTypeSelection =
          d.findChild<QWidget *>(tr("Committer") + "CommitDateType");
      QVERIFY(committerCommitDateTimeTypeSelection);
      auto *committerCurrent =
          committerCommitDateTimeTypeSelection->findChild<QRadioButton *>(
              "Current");
      QVERIFY(committerCurrent);
      auto *committerOriginal =
          committerCommitDateTimeTypeSelection->findChild<QRadioButton *>(
              "Original");
      QVERIFY(committerOriginal);
      auto *committerManual =
          committerCommitDateTimeTypeSelection->findChild<QRadioButton *>(
              "Manual");
      QVERIFY(committerManual);
      auto *committerCommitDate =
          d.findChild<QDateTimeEdit *>(tr("Committer") + "CommitDate");
      QVERIFY(committerCommitDate);
      committerCommitDate->setDateTime(
          QDateTime(QDate(2013, 5, 2), QTime(11, 22, 7)));

      // current
      committerCurrent->click();
      auto info = d.getInfo();
      QCOMPARE(info.committerInfo.commitDateType,
               ContributorInfo::SelectedDateTimeType::Current);
      QCOMPARE(committerCommitDate->isVisible(), false);

      // original
      committerOriginal->click();
      info = d.getInfo();
      QCOMPARE(info.committerInfo.commitDateType,
               ContributorInfo::SelectedDateTimeType::Original);
      QCOMPARE(committerCommitDate->isVisible(), false);
      QCOMPARE(info.committerInfo.commitDate,
               QDateTime::fromString("Sun May 23 11:36:26 2022 +0200",
                                     Qt::RFC2822Date));

      // manual
      committerManual->click();
      info = d.getInfo();
      QCOMPARE(info.committerInfo.commitDateType,
               ContributorInfo::SelectedDateTimeType::Manual);
      QCOMPARE(committerCommitDate->isVisible(), true);
      QCOMPARE(info.committerInfo.commitDate,
               QDateTime(QDate(2013, 5, 2), QTime(11, 22, 7)));
    }

    auto info = d.getInfo();
    QCOMPARE(info.authorInfo.name, "New Author");
    QCOMPARE(info.authorInfo.email, "New Author Email");
    QCOMPARE(info.committerInfo.name, "New Committer");
    QCOMPARE(info.committerInfo.email, "New Committer Email");
    QCOMPARE(info.commitMessage, "Test commit message");
  }
}

void TestAmend::testAmendDialog2() {
  // Test changing author name, author email, committer name, committer email

  Test::ScratchRepository repo;
  auto authorSignature = repo->signature(
      "New Author", "New Author Email",
      QDateTime::fromString("Sun May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  auto committerSignature = repo->signature(
      "New Committer", "New Committer Email",
      QDateTime::fromString("Sun May 23 11:36:26 2022 +0200", Qt::RFC2822Date));

  AmendDialog d(authorSignature, committerSignature, "Test commit message");

  auto commitMessageEditor =
      d.findChild<QTextEdit *>("Textlabel Commit Message");
  QVERIFY(commitMessageEditor);
  commitMessageEditor->setText("Changing the commit message");

  // Author
  {
    const auto *author = d.findChild<QWidget *>(tr("Author"));
    QVERIFY(author);

    auto *name = author->findChild<QLineEdit *>("Name");
    QVERIFY(name);
    auto *email = author->findChild<QLineEdit *>("Email");
    QVERIFY(email);

    name->setText("Another author name");
    email->setText("Another author email address");
  }

  // Committer
  {
    const auto *committer = d.findChild<QWidget *>(tr("Committer"));
    QVERIFY(committer);

    auto *name = committer->findChild<QLineEdit *>("Name");
    QVERIFY(name);
    auto *email = committer->findChild<QLineEdit *>("Email");
    QVERIFY(email);

    name->setText("Another committer name");
    email->setText("Another committer email address");
  }

  const auto info = d.getInfo();

  QCOMPARE(info.authorInfo.name, "Another author name");
  QCOMPARE(info.authorInfo.email, "Another author email address");
  QCOMPARE(info.committerInfo.name, "Another committer name");
  QCOMPARE(info.committerInfo.email, "Another committer email address");
  QCOMPARE(info.commitMessage, "Changing the commit message");
}

void TestAmend::testAmendDialogNewLineInMessage() {
  // Test changing author name, author email, committer name, committer email

  Test::ScratchRepository repo;
  auto authorSignature = repo->signature(
      "New Author", "New Author Email",
      QDateTime::fromString("Sun May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  auto committerSignature = repo->signature(
      "New Committer", "New Committer Email",
      QDateTime::fromString("Sun May 23 11:36:26 2022 +0200", Qt::RFC2822Date));

  AmendDialog d(authorSignature, committerSignature,
                "Test commit message\nNewLine");

  QCOMPARE(d.getInfo().commitMessage, "Test commit message\nNewLine");
}

TEST_MAIN(TestAmend)
#include "amend.moc"
