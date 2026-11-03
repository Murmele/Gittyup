#include "Test.h"

#include "git/Blame.h"
#include "git/Commit.h"
#include "git/Repository.h"
#include "git/Signature.h"

using namespace QTest;

class TestBlame : public QObject {
  Q_OBJECT

private slots:
  void authorWithoutEmailFailsCleanly();
};

/*!
 * \brief TestBlame::authorWithoutEmailFailsCleanly
 * libgit2 can't build a blame hunk for a commit whose author has no email
 * address, since its mailmap lookup rejects empty emails. Since libgit2#7180
 * was fixed the whole blame fails instead of returning a null hunk, so callers
 * must get an invalid blame rather than a half-built one.
 * BlameAuthorWithoutEmail.zip holds three commits: an empty seed, one by
 * "Bot <>" adding the first line of hello.txt, and one with a normal author
 * adding the second line.
 */
void TestBlame::authorWithoutEmailFailsCleanly() {
  QString path = Test::extractRepository("BlameAuthorWithoutEmail.zip");
  QVERIFY(!path.isEmpty());
  git::Repository repo = git::Repository::open(path);
  QVERIFY(repo.isValid());

  git::Blame blame = repo.blame("hello.txt", git::Commit());
  QVERIFY(!blame.isValid());
}

TEST_MAIN(TestBlame)
#include "blame.moc"
