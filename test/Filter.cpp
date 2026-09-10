//
//          Copyright (c) 2026, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"

#include "git/Commit.h"
#include "git/Reference.h"
#include "git/Repository.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

namespace {

// Registers a throwaway global filter driver, scoped to a fake $HOME so
// the developer's real ~/.gitconfig is never touched. git::Filter::init()
// reads global filters once, during Application's constructor, before any
// QTest slot runs - so $HOME must be redirected during static
// initialization, which is why this lives in a namespace-scope static.
struct FakeHome {
  QTemporaryDir dir;
  QString argsOutPath;

  FakeHome() {
    argsOutPath = dir.filePath("filter_args.txt");

    QFile gitconfig(dir.filePath(".gitconfig"));
    bool opened = gitconfig.open(QIODevice::WriteOnly | QIODevice::Truncate |
                                 QIODevice::Text);
    Q_ASSERT(opened);
    Q_UNUSED(opened);

    QTextStream out(&gitconfig);
    // "clean" is unused but must be non-empty for the driver to register.
    out << "[filter \"gittyupsecuritytest\"]\n";
    out << "    clean = cat\n";
    out << "    smudge = printf 'ARG=[%s]' %f > \"" << argsOutPath << "\"\n";
    gitconfig.close();

    qputenv("HOME", dir.path().toLocal8Bit());
    qputenv("USERPROFILE", dir.path().toLocal8Bit());
  }
};

FakeHome &fakeHome() {
  static FakeHome home;
  return home;
}

// Force construction (and therefore the $HOME override) before main().
FakeHome &gFakeHomeInit = fakeHome();

} // namespace

class TestFilter : public QObject {
  Q_OBJECT

private slots:
  void rejectsShellInjectionInFilename();
};

// Regression test: a filename embedding a single quote, combined with a
// .gitattributes rule assigning it to any globally-registered filter (e.g.
// Git LFS), could break out of the quoted %f argument and run arbitrary
// shell commands during smudge.
void TestFilter::rejectsShellInjectionInFilename() {
  FakeHome &home = fakeHome();
  QFile::remove(home.argsOutPath);

  // FilterCommandInjection.zip's single commit contains one file, matching
  // this name, whose .gitattributes assigns it to the "gittyupsecuritytest"
  // filter registered above. If %f isn't escaped correctly, the embedded
  // "'" ends the quoted shell argument early and the ';'s that follow start
  // new, attacker-controlled statements.
  QString maliciousName = "evil';>injected_marker;echo'safe";

  QString path = Test::extractRepository("FilterCommandInjection.zip", true);
  QVERIFY(!path.isEmpty());
  git::Repository repo = git::Repository::open(path);
  QVERIFY(repo.isValid());
  QDir workdir = repo.workdir();

  // The injected ">injected_marker" redirection, if it runs, creates this
  // file in the checkout's working directory.
  QString markerPath = workdir.filePath("injected_marker");
  QFile::remove(markerPath);

  // Delete the working copy so checkout has to rewrite it from the repo,
  // which is what actually triggers the smudge filter.
  QVERIFY(QFile::remove(workdir.filePath(maliciousName)));

  // libgit2's own checkout implementation invokes the registered smudge
  // filter driver as it writes each file to the working directory.
  git::Reference head = repo.head();
  QVERIFY(head.isValid());
  QVERIFY(
      repo.checkout(head.target(), nullptr, QStringList(), GIT_CHECKOUT_FORCE));

  QVERIFY2(!QFile::exists(markerPath),
           "filter command injection via crafted filename was not blocked");

  // Confirms the filter actually ran (not just failed to start).
  QFile args(home.argsOutPath);
  QVERIFY2(args.exists(), "smudge filter did not run");
  QVERIFY(args.open(QIODevice::ReadOnly | QIODevice::Text));
  QString content = QString::fromUtf8(args.readAll());
  args.close();

  QCOMPARE(content, QString("ARG=[%1]").arg(maliciousName));
}

TEST_MAIN(TestFilter)

#include "Filter.moc"
