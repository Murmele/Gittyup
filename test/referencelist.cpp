//
//          Copyright (c) 2022, Gittyup Contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "Test.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/ReferenceList.h"
#include "ui/ReferenceModel.h"
#include "dialogs/CloneDialog.h"
#include <QMainWindow>

using namespace Test;
using namespace QTest;

class TestReferenceList : public QObject {
  Q_OBJECT

private slots:
  void test();
  void testIndexCalculation();

private:
};

void TestReferenceList::test() {
  CloneDialog *d = new CloneDialog(CloneDialog::Kind::Clone);

  RepoView *view = nullptr;

  bool cloneFinished = false;
  QObject::connect(d, &CloneDialog::accepted, [d, &view, &cloneFinished] {
    cloneFinished = true;
    if (MainWindow *window = MainWindow::open(d->path())) {
      view = window->currentView();
    }
  });

  QTemporaryDir tempdir;
  QVERIFY(tempdir.isValid());
  const auto repoPath = tempdir.path();
  d->setField("url", "https://github.com/Murmele/GittyupTestRepo.git");
  d->setField("name", "GittyupTestRepo");
  d->setField("path", repoPath);
  d->setField("bare", "false");
  d->page(2)->initializePage(); // start clone

  {
    auto timeout = Timeout(1000e3, "Failed to clone");
    while (!cloneFinished)
      qWait(300);
  }
  QVERIFY(view);
  git::Repository repo = view->repo();
  QVERIFY(repo.isValid());
  initRepo(repo);

  const ReferenceView::Kinds kRefKinds =
      ReferenceView::InvalidRef | ReferenceView::LocalBranches |
      ReferenceView::RemoteBranches | ReferenceView::Tags;
  ReferenceList *rl = new ReferenceList(repo, kRefKinds);

  {
    // Only one tag
    git::Commit commit =
        repo.lookupCommit("99219268e1f838b0da616761fd7a184676965a69");
    QVERIFY(commit.isValid());
    rl->setCommit(commit);
    QVERIFY(rl->target().isValid());
    QCOMPARE(rl->currentReference().name(), "Tag");
  }

  {
    // Only one remote
    git::Commit commit =
        repo.lookupCommit("79f4bee33320391fa99a8ef3f504b2ba229a8181");
    QVERIFY(commit.isValid());
    rl->setCommit(commit);
    QVERIFY(rl->target().isValid());
    QCOMPARE(rl->currentReference().name(), "origin/Branch");
  }

  {
    // Only one branch
    git::Commit commit =
        repo.lookupCommit("54ecb63965b50287ceb73095c72f344c1611d94a");
    QVERIFY(commit.isValid());
    rl->setCommit(commit);
    QVERIFY(rl->target().isValid());
    QCOMPARE(rl->currentReference().name(), "main");
  }

  {
    // No branch, no remote, no tag
    git::Commit commit =
        repo.lookupCommit("63460da2b069250c34506249516029f2ba7c6057");
    QVERIFY(commit.isValid());
    rl->setCommit(commit);
    QVERIFY(rl->target().isValid());
    QCOMPARE(rl->currentReference().name(), "");
  }
}

void TestReferenceList::testIndexCalculation() {
  {
    const ReferenceView::Kinds kRefKinds =
        ReferenceView::InvalidRef | ReferenceView::LocalBranches |
        ReferenceView::RemoteBranches | ReferenceView::Tags;
    ReferenceModel model(git::Repository(), kRefKinds);

    QCOMPARE(model.indexToReferenceType(0),
             ReferenceModel::ReferenceType::Branches);
    QCOMPARE(
        model.referenceTypeToIndex(ReferenceModel::ReferenceType::Branches), 0);

    QCOMPARE(model.indexToReferenceType(1),
             ReferenceModel::ReferenceType::Remotes);
    QCOMPARE(model.referenceTypeToIndex(ReferenceModel::ReferenceType::Remotes),
             1);

    QCOMPARE(model.indexToReferenceType(2),
             ReferenceModel::ReferenceType::Tags);
    QCOMPARE(model.referenceTypeToIndex(ReferenceModel::ReferenceType::Tags),
             2);
  }

  {
    // No local branches
    const ReferenceView::Kinds kRefKinds = ReferenceView::InvalidRef |
                                           ReferenceView::RemoteBranches |
                                           ReferenceView::Tags;
    ReferenceModel model(git::Repository(), kRefKinds);

    QCOMPARE(model.indexToReferenceType(0),
             ReferenceModel::ReferenceType::Remotes);
    QCOMPARE(model.referenceTypeToIndex(ReferenceModel::ReferenceType::Remotes),
             0);

    QCOMPARE(model.indexToReferenceType(1),
             ReferenceModel::ReferenceType::Tags);
    QCOMPARE(model.referenceTypeToIndex(ReferenceModel::ReferenceType::Tags),
             1);
  }

  {
    // No remote branches
    const ReferenceView::Kinds kRefKinds = ReferenceView::InvalidRef |
                                           ReferenceView::LocalBranches |
                                           ReferenceView::Tags;
    ReferenceModel model(git::Repository(), kRefKinds);

    QCOMPARE(model.indexToReferenceType(0),
             ReferenceModel::ReferenceType::Branches);
    QCOMPARE(
        model.referenceTypeToIndex(ReferenceModel::ReferenceType::Branches), 0);

    QCOMPARE(model.indexToReferenceType(1),
             ReferenceModel::ReferenceType::Tags);
    QCOMPARE(model.referenceTypeToIndex(ReferenceModel::ReferenceType::Tags),
             1);
  }

  {
    // No tags
    const ReferenceView::Kinds kRefKinds = ReferenceView::InvalidRef |
                                           ReferenceView::LocalBranches |
                                           ReferenceView::RemoteBranches;
    ReferenceModel model(git::Repository(), kRefKinds);

    QCOMPARE(model.indexToReferenceType(0),
             ReferenceModel::ReferenceType::Branches);
    QCOMPARE(
        model.referenceTypeToIndex(ReferenceModel::ReferenceType::Branches), 0);

    QCOMPARE(model.indexToReferenceType(1),
             ReferenceModel::ReferenceType::Remotes);
    QCOMPARE(model.referenceTypeToIndex(ReferenceModel::ReferenceType::Remotes),
             1);
  }

  {
    // Only tags. No local/remote branches
    const ReferenceView::Kinds kRefKinds =
        ReferenceView::InvalidRef | ReferenceView::Tags;
    ReferenceModel model(git::Repository(), kRefKinds);

    QCOMPARE(model.indexToReferenceType(0),
             ReferenceModel::ReferenceType::Tags);
    QCOMPARE(model.referenceTypeToIndex(ReferenceModel::ReferenceType::Tags),
             0);
  }

  {
    // Only local branches. No tag/remote
    const ReferenceView::Kinds kRefKinds =
        ReferenceView::InvalidRef | ReferenceView::LocalBranches;
    ReferenceModel model(git::Repository(), kRefKinds);

    QCOMPARE(model.indexToReferenceType(0),
             ReferenceModel::ReferenceType::Branches);
    QCOMPARE(
        model.referenceTypeToIndex(ReferenceModel::ReferenceType::Branches), 0);
  }

  {
    // Only remote branches. No tag/local
    const ReferenceView::Kinds kRefKinds =
        ReferenceView::InvalidRef | ReferenceView::RemoteBranches;
    ReferenceModel model(git::Repository(), kRefKinds);

    QCOMPARE(model.indexToReferenceType(0),
             ReferenceModel::ReferenceType::Remotes);
    QCOMPARE(model.referenceTypeToIndex(ReferenceModel::ReferenceType::Remotes),
             0);
  }
}

TEST_MAIN(TestReferenceList)

#include "referencelist.moc"
