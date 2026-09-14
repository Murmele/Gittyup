#include "Test.h"

#include "git/Blame.h"
#include "git/Commit.h"
#include "git/Repository.h"
#include "git/Signature.h"

using namespace QTest;

class TestBlame : public QObject {
  Q_OBJECT

private slots:
  void accessorsSurviveHunkWithoutAuthorEmail();
};

/*!
 * \brief TestBlame::accessorsSurviveHunkWithoutAuthorEmail
 * libgit2 hands back a null hunk for commits whose author has no email address
 * (libgit2#7180), so every git::Blame accessor has to cope with one instead of
 * dereferencing it. BlameAuthorWithoutEmail.zip holds three commits: an empty
 * seed, one by "Bot <>" adding the first line of hello.txt, and one with a
 * normal author adding the second line.
 */
void TestBlame::accessorsSurviveHunkWithoutAuthorEmail() {
  QString path = Test::extractRepository("BlameAuthorWithoutEmail.zip");
  QVERIFY(!path.isEmpty());
  git::Repository repo = git::Repository::open(path);
  QVERIFY(repo.isValid());

  git::Blame blame = repo.blame("hello.txt", git::Commit());
  QVERIFY(blame.isValid());
  QVERIFY(blame.count() > 0);

  // Each of these dereferences the hunk; a null one must not crash.
  for (int i = 0; i < blame.count(); ++i) {
    blame.line(i);
    blame.id(i);
    blame.message(i);
    blame.signature(i);
    blame.isCommitted(i);
  }

  // A hunk libgit2 could not build carries no signature, so callers fall back
  // to their "invalid signature" path rather than showing a half-read one.
  for (int i = 0; i < blame.count(); ++i) {
    if (!blame.signature(i).isValid()) {
      QCOMPARE(blame.message(i), QString());
      QCOMPARE(blame.isCommitted(i), false);
    }
  }
}

TEST_MAIN(TestBlame)
#include "blame.moc"
