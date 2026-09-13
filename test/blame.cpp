#include "Test.h"

#include "git/Blame.h"
#include "git/Command.h"
#include "git/Commit.h"
#include "git/Repository.h"
#include "git/Signature.h"

#include <QProcess>

#ifndef GIT_EXECUTABLE
#error                                                                         \
    "To execute those tests it is neccessary to have git installed on your computer. Turn off tests or exclude this test to build the project"
#endif

#define EXECUTE_GIT_COMMAND(workdir, arguments)                                \
  {                                                                            \
    QProcess p(this);                                                          \
    p.setWorkingDirectory(workdir);                                            \
    QString bash = git::Command::bashPath();                                   \
    QVERIFY(!bash.isEmpty());                                                  \
    QString command = QString(GIT_EXECUTABLE) + " " + arguments;               \
    QStringList a = {"-c", command};                                           \
    p.start(bash, a);                                                          \
    p.waitForStarted();                                                        \
    QCOMPARE(p.waitForFinished(), true);                                       \
    QCOMPARE(p.exitCode(), 0);                                                 \
  }

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
 * dereferencing it. The commit is made through git itself because libgit2
 * refuses to build a signature with an empty email.
 */
void TestBlame::accessorsSurviveHunkWithoutAuthorEmail() {
  Test::ScratchRepository repo;
  const QString workdir = repo->workdir().path();
  const QString name = "hello.txt";

  EXECUTE_GIT_COMMAND(workdir, "init -q -b main .");
  EXECUTE_GIT_COMMAND(workdir, QString("-c core.fsmonitor=false commit -q "
                                       "--allow-empty -m seed"));

  QFile file(repo->workdir().filePath(name));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "first line" << Qt::endl;
  file.close();

  // An empty author email is what makes libgit2 produce a null hunk.
  EXECUTE_GIT_COMMAND(workdir, QString("add %1").arg(name));
  EXECUTE_GIT_COMMAND(
      workdir, QString("-c user.name=Bot -c user.email= commit -q -m \"no "
                       "email\" --author=\"Bot <>\""));

  QVERIFY(file.open(QFile::WriteOnly | QFile::Append));
  QTextStream(&file) << "second line" << Qt::endl;
  file.close();

  EXECUTE_GIT_COMMAND(workdir, QString("add %1").arg(name));
  EXECUTE_GIT_COMMAND(workdir, "commit -q -m \"with email\"");

  git::Blame blame = repo->blame(name, git::Commit());
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
