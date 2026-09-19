//
//          Copyright (c) 2026, Gittyup
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Anand Hegde
//

#include "Test.h"

#include "dialogs/ConfigDialog.h"
#include "git/CommitSigner.h"
#include "git/Config.h"
#include "git/Index.h"
#include "git/Reference.h"
#include "git/Signature.h"
#include "git2/buffer.h"
#include "git2/commit.h"
#include "git2/errors.h"
#include "git2/oid.h"
#include "git2/reflog.h"
#include "git2/repository.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>

// The fixture has "Initial commit" with "Change on master" on top of it on
// master, and "Change on feature" on the checked out feature branch.
#define INIT_REPO()                                                            \
  QString path = Test::extractRepository("CommitSigning.zip");                 \
  QVERIFY(!path.isEmpty());                                                    \
  git::Repository repo = git::Repository::open(path);                          \
  QVERIFY(repo.isValid());                                                     \
  Test::initRepo(repo);

namespace {

// Signing is inherently done by an external program, so these helpers run
// ssh-keygen and gpg to set up keys and to verify signatures.
bool run(const QString &program, const QStringList &args,
         const QByteArray &input = QByteArray(), QByteArray *out = nullptr) {
  QProcess process;
  process.start(program, args);
  if (!process.waitForStarted())
    return false;
  process.write(input);
  process.closeWriteChannel();
  if (!process.waitForFinished(60000))
    return false;
  if (out)
    *out = process.readAllStandardOutput();
  if (process.exitCode() != 0)
    qWarning() << program << args << process.readAllStandardError();
  return process.exitStatus() == QProcess::NormalExit &&
         process.exitCode() == 0;
}

bool writeFile(const QString &path, const QByteArray &content) {
  QFile file(path);
  return file.open(QIODevice::WriteOnly) && file.write(content) >= 0;
}

// Read the signature and signed data of a commit with libgit2 directly.
bool extractSignature(const QString &path, const git::Id &id,
                      QByteArray &signature, QByteArray &data) {
  git_repository *repo = nullptr;
  if (git_repository_open(&repo, path.toUtf8()))
    return false;

  git_buf sig = GIT_BUF_INIT;
  git_buf signedData = GIT_BUF_INIT;
  git_oid oid;
  git_oid_fromstr(&oid, id.toString().toUtf8());
  bool found =
      !git_commit_extract_signature(&sig, &signedData, repo, &oid, nullptr);
  if (found) {
    signature = QByteArray(sig.ptr, sig.size);
    data = QByteArray(signedData.ptr, signedData.size);
  }

  git_buf_dispose(&sig);
  git_buf_dispose(&signedData);
  git_repository_free(repo);
  return found;
}

QString lastReflogMessage(const QString &path, const char *name) {
  git_repository *repo = nullptr;
  if (git_repository_open(&repo, path.toUtf8()))
    return QString();

  QString message;
  git_reflog *reflog = nullptr;
  if (!git_reflog_read(&reflog, repo, name)) {
    if (const git_reflog_entry *entry = git_reflog_entry_byindex(reflog, 0))
      message = git_reflog_entry_message(entry);
    git_reflog_free(reflog);
  }

  git_repository_free(repo);
  return message;
}

void stageNewFile(git::Repository &repo, const QString &name) {
  QVERIFY(writeFile(repo.workdir().filePath(name), "content\n"));
  repo.index().setStaged({name}, true);
}

} // namespace

class TestCommitSigning : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();

  void unsignedCommit();
  void sshSignedCommit();
  void sshSignedAmend();
  void sshSignedMergeCommit();
  void sshSignedRebase();
  void openPgpSignedCommit();
  void openPgpUnknownKey();
  void failingSignerKeepsHead();
  void unsupportedFormat();
  void signingErrorDialog();
  void settingsDialog();

private:
  void enableSsh(git::Repository &repo);
  void verifySsh(const QString &path, const git::Commit &commit);

  QTemporaryDir mKeyDir;
  QString mSshKeygen;
  QString mSshKey;
  QString mGpg;
  QString mGnupgHome;
};

void TestCommitSigning::initTestCase() {
  QVERIFY(mKeyDir.isValid());

  // Throwaway ssh key.
  mSshKeygen = QStandardPaths::findExecutable("ssh-keygen");
  if (!mSshKeygen.isEmpty()) {
    mSshKey = mKeyDir.filePath("id_ed25519");
    QVERIFY(run(mSshKeygen, {"-q", "-t", "ed25519", "-N", "", "-C", "test@user",
                             "-f", mSshKey}));
  }

  // Throwaway GnuPG home with a key without passphrase. Keep the path short,
  // gpg-agent's socket has to fit into a unix socket path.
  mGpg = QStandardPaths::findExecutable("gpg");
  if (!mGpg.isEmpty()) {
    mGnupgHome = mKeyDir.filePath("gnupg");
    QVERIFY(QDir().mkpath(mGnupgHome));
    QFile::setPermissions(mGnupgHome, QFile::ReadOwner | QFile::WriteOwner |
                                          QFile::ExeOwner);
    qputenv("GNUPGHOME", mGnupgHome.toUtf8());
    if (!run(mGpg, {"--batch", "--pinentry-mode", "loopback", "--passphrase",
                    "", "--quick-gen-key", "testuser <test@user>", "ed25519",
                    "sign", "never"}))
      mGpg.clear(); // Skip the test instead of failing.
  }
}

void TestCommitSigning::cleanupTestCase() {
  if (!mGnupgHome.isEmpty())
    run("gpgconf", {"--kill", "gpg-agent"});
}

void TestCommitSigning::enableSsh(git::Repository &repo) {
  git::Config config = repo.gitConfig();
  config.setValue("commit.gpgsign", true);
  config.setValue("gpg.format", QString("ssh"));
  config.setValue("gpg.ssh.program", mSshKeygen);
  config.setValue("user.signingkey", mSshKey);
}

void TestCommitSigning::verifySsh(const QString &path,
                                  const git::Commit &commit) {
  QByteArray signature, data;
  QVERIFY(extractSignature(path, commit.id(), signature, data));
  QVERIFY(signature.startsWith("-----BEGIN SSH SIGNATURE-----"));

  QFile pub(mSshKey + ".pub");
  QVERIFY(pub.open(QIODevice::ReadOnly));
  QString allowed = mKeyDir.filePath("allowed_signers");
  QVERIFY(writeFile(allowed, "test@user " + pub.readAll()));

  QString sigFile = mKeyDir.filePath("commit.sig");
  QVERIFY(writeFile(sigFile, signature));

  QVERIFY(run(mSshKeygen,
              {"-Y", "verify", "-f", allowed, "-I", "test@user", "-n", "git",
               "-s", sigFile},
              data));
}

void TestCommitSigning::unsignedCommit() {
  INIT_REPO();
  repo.gitConfig().setValue("commit.gpgsign", false);

  git::Commit parent = repo.head().target();
  stageNewFile(repo, "unsigned.txt");
  git::Commit commit = repo.commit("Unsigned commit");
  QVERIFY(commit.isValid());
  QCOMPARE(repo.head().target().id(), commit.id());
  QCOMPARE(commit.parents().first().id(), parent.id());

  QByteArray signature, data;
  QVERIFY(!extractSignature(path, commit.id(), signature, data));
  QCOMPARE(lastReflogMessage(path, "HEAD"), QString("commit: Unsigned commit"));
}

void TestCommitSigning::sshSignedCommit() {
  if (mSshKeygen.isEmpty())
    QSKIP("ssh-keygen is not available");

  INIT_REPO();
  enableSsh(repo);

  git::Commit parent = repo.head().target();
  stageNewFile(repo, "signed.txt");
  git::Commit commit = repo.commit("Signed commit");
  QVERIFY(commit.isValid());
  QCOMPARE(repo.head().name(), QString("feature"));
  QCOMPARE(repo.head().target().id(), commit.id());
  QCOMPARE(commit.parents().first().id(), parent.id());
  QCOMPARE(commit.message(), QString("Signed commit"));
  QVERIFY(commit.blob("signed.txt").isValid());

  verifySsh(path, commit);

  // Same reflog messages as an unsigned commit.
  QCOMPARE(lastReflogMessage(path, "refs/heads/feature"),
           QString("commit: Signed commit"));
  QCOMPARE(lastReflogMessage(path, "HEAD"), QString("commit: Signed commit"));
}

void TestCommitSigning::sshSignedAmend() {
  if (mSshKeygen.isEmpty())
    QSKIP("ssh-keygen is not available");

  INIT_REPO();
  enableSsh(repo);

  git::Commit original = repo.head().target();
  stageNewFile(repo, "amended.txt");
  git::Signature author = repo.signature("New Author", "new@author");
  git::Signature committer = repo.signature("testuser", "test@user");
  QVERIFY(repo.amend(original, author, committer, "Amended commit"));

  git::Commit commit = repo.head().target();
  QVERIFY(commit.id() != original.id());
  QCOMPARE(commit.message(), QString("Amended commit"));
  QCOMPARE(commit.author().name(), QString("New Author"));
  QCOMPARE(commit.parents().first().id(), original.parents().first().id());
  QVERIFY(commit.blob("amended.txt").isValid());

  verifySsh(path, commit);
  // libgit2 doesn't mark amends in the reflog, unlike git.
  QCOMPARE(lastReflogMessage(path, "HEAD"), QString("commit: Amended commit"));
}

void TestCommitSigning::sshSignedMergeCommit() {
  if (mSshKeygen.isEmpty())
    QSKIP("ssh-keygen is not available");

  INIT_REPO();
  enableSsh(repo);

  git::AnnotatedCommit master =
      repo.lookupRef("refs/heads/master").annotatedCommit();
  git::Signature sig = repo.signature("testuser", "test@user");
  git::Commit commit = repo.commit(sig, sig, "Merge master", master);
  QVERIFY(commit.isValid());
  QCOMPARE(commit.parents().size(), 2);
  QCOMPARE(repo.head().target().id(), commit.id());

  verifySsh(path, commit);
  QCOMPARE(lastReflogMessage(path, "HEAD"),
           QString("commit (merge): Merge master"));
}

void TestCommitSigning::sshSignedRebase() {
  if (mSshKeygen.isEmpty())
    QSKIP("ssh-keygen is not available");

  INIT_REPO();
  enableSsh(repo);

  git::Reference master = repo.lookupRef("refs/heads/master");
  repo.rebase(master.annotatedCommit());
  QVERIFY(!repo.rebaseOngoing());

  git::Commit commit = repo.head().target();
  QCOMPARE(repo.head().name(), QString("feature"));
  QCOMPARE(commit.message(), QString("Change on feature\n"));
  QCOMPARE(commit.parents().first().id(), master.target().id());

  verifySsh(path, commit);
}

void TestCommitSigning::openPgpSignedCommit() {
  if (mGpg.isEmpty())
    QSKIP("gpg is not available");

  INIT_REPO();
  git::Config config = repo.gitConfig();
  config.setValue("commit.gpgsign", true);
  config.setValue("gpg.program", mGpg);
  // No user.signingkey: the committer identity selects the key, as in git.

  stageNewFile(repo, "gpg.txt");
  git::Commit commit = repo.commit("GPG signed commit");
  QVERIFY(commit.isValid());
  QCOMPARE(repo.head().target().id(), commit.id());

  QByteArray signature, data;
  QVERIFY(extractSignature(path, commit.id(), signature, data));
  QVERIFY(signature.startsWith("-----BEGIN PGP SIGNATURE-----"));

  QString sigFile = mKeyDir.filePath("commit.asc");
  QString dataFile = mKeyDir.filePath("commit.data");
  QVERIFY(writeFile(sigFile, signature));
  QVERIFY(writeFile(dataFile, data));
  QVERIFY(run(mGpg, {"--batch", "--verify", sigFile, dataFile}));
}

void TestCommitSigning::openPgpUnknownKey() {
  if (mGpg.isEmpty())
    QSKIP("gpg is not available");

  INIT_REPO();
  git::Config config = repo.gitConfig();
  config.setValue("commit.gpgsign", true);
  config.setValue("gpg.program", mGpg);
  config.setValue("user.signingkey", QString("nobody@example.com"));

  git::Commit head = repo.head().target();
  stageNewFile(repo, "gpg.txt");
  QVERIFY(!repo.commit("Not signed").isValid());
  QCOMPARE(repo.head().target().id(), head.id());

  // gpg's messages are kept, its machine readable status lines aren't.
  QString error = git::CommitSigner::takeLastError();
  QVERIFY2(error.contains("nobody@example.com"), qPrintable(error));
  QVERIFY2(!error.contains("[GNUPG:]"), qPrintable(error));
}

void TestCommitSigning::failingSignerKeepsHead() {
  if (mSshKeygen.isEmpty())
    QSKIP("ssh-keygen is not available");

  INIT_REPO();
  enableSsh(repo);
  QString missing = mKeyDir.filePath("missing_key");
  repo.gitConfig().setValue("user.signingkey", missing);

  git::Commit head = repo.head().target();
  stageNewFile(repo, "failing.txt");
  QVERIFY(!repo.commit("Not signed").isValid());
  QCOMPARE(repo.head().target().id(), head.id());

  QVERIFY(git::Repository::lastError().startsWith("Failed to sign commit"));
  QString error = git::CommitSigner::takeLastError();
  QVERIFY2(error.contains(missing), qPrintable(error));
  QVERIFY(git::CommitSigner::takeLastError().isEmpty());

  // Amending must not replace the commit either.
  git::Signature sig = repo.signature("testuser", "test@user");
  QVERIFY(!repo.amend(head, sig, sig, "Not signed"));
  QCOMPARE(repo.head().target().id(), head.id());
  QVERIFY(!git::CommitSigner::takeLastError().isEmpty());
}

void TestCommitSigning::unsupportedFormat() {
  INIT_REPO();
  git::Config config = repo.gitConfig();
  config.setValue("commit.gpgsign", true);
  config.setValue("gpg.format", QString("x509"));

  git::Commit head = repo.head().target();
  stageNewFile(repo, "x509.txt");
  QVERIFY(!repo.commit("Not signed").isValid());
  QCOMPARE(repo.head().target().id(), head.id());
  QVERIFY(git::CommitSigner::takeLastError().contains("x509"));
}

void TestCommitSigning::signingErrorDialog() {
  INIT_REPO();
  git::Config config = repo.gitConfig();
  config.setValue("commit.gpgsign", true);
  config.setValue("gpg.format", QString("x509"));

  git::Commit head = repo.head().target();
  stageNewFile(repo, "dialog.txt");

  MainWindow window(repo);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  RepoView *view = window.currentView();
  Test::refresh(view);

  QVERIFY(!view->commit(QString("Not signed")));
  QCOMPARE(repo.head().target().id(), head.id());

  auto dialog = view->findChild<QMessageBox *>();
  QVERIFY(dialog);
  QVERIFY(dialog->text().contains("could not be signed"));
  QVERIFY(dialog->detailedText().contains("x509"));
  dialog->close();
}

void TestCommitSigning::settingsDialog() {
  INIT_REPO();

  MainWindow window(repo);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  ConfigDialog *dialog =
      window.currentView()->configureSettings(ConfigDialog::General);
  QVERIFY(QTest::qWaitForWindowExposed(dialog));

  auto sign = dialog->findChild<QCheckBox *>("SignCommits");
  auto format = dialog->findChild<QComboBox *>("SigningFormat");
  auto key = dialog->findChild<QLineEdit *>("SigningKey");
  QVERIFY(sign && format && key);
  QVERIFY(!sign->isChecked());
  QVERIFY(!key->isEnabled());

  sign->setChecked(true);
  QVERIFY(key->isEnabled());
  format->setCurrentIndex(format->findData("ssh"));
  key->setText("~/.ssh/id_ed25519.pub");

  git::Config config = repo.gitConfig();
  QCOMPARE(config.value<bool>("commit.gpgsign"), true);
  QCOMPARE(config.value<QString>("gpg.format"), QString("ssh"));
  QCOMPARE(config.value<QString>("user.signingkey"),
           QString("~/.ssh/id_ed25519.pub"));

  key->clear();
  sign->setChecked(false);
  QCOMPARE(config.value<bool>("commit.gpgsign", true), false);
  QVERIFY(config.value<QString>("user.signingkey").isEmpty());

  dialog->close();
}

TEST_MAIN(TestCommitSigning)

#include "CommitSigning.moc"
