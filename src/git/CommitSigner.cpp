//
//          Copyright (c) 2026, Gittyup
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Anand Hegde
//

#include "CommitSigner.h"
#include "git2/commit.h"
#include "git2/config.h"
#include "git2/errors.h"
#include "git2/repository.h"
#include "git2/signature.h"
#include "git2/sys/errors.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>

namespace git {

namespace {

// Generous, since gpg-agent may be waiting for the user to type a
// passphrase into pinentry.
const int kSignTimeout = 120000;

QString sLastError;

QString configString(git_config *config, const char *key) {
  QString value;
  git_buf buf = GIT_BUF_INIT;
  if (!git_config_get_string_buf(&buf, config, key))
    value = QString::fromUtf8(buf.ptr, buf.size);
  git_buf_dispose(&buf);
  return value;
}

// Expand a leading ~/ like git does for path config values.
QString expandPath(const QString &path) {
  if (path.startsWith("~/"))
    return QDir::home().filePath(path.mid(2));
  return path;
}

// Run the signing program. Returns false with the error set if it couldn't
// be run or exited with an error.
bool run(const QString &program, const QStringList &args,
         const QByteArray &input, QByteArray &output, QString &error) {
  QProcess process;
  process.start(program, args);
  if (!process.waitForStarted(kSignTimeout)) {
    error = QObject::tr("Unable to start '%1': %2")
                .arg(program, process.errorString());
    return false;
  }

  process.write(input);
  process.closeWriteChannel();

  if (!process.waitForFinished(kSignTimeout)) {
    process.kill();
    process.waitForFinished();
    error = QObject::tr("'%1' did not finish within %2 seconds")
                .arg(program)
                .arg(kSignTimeout / 1000);
    return false;
  }

  output = process.readAllStandardOutput();
  error = QString::fromUtf8(process.readAllStandardError()).trimmed();
  return process.exitStatus() == QProcess::NormalExit &&
         process.exitCode() == 0;
}

} // namespace

CommitSigner::CommitSigner(git_repository *repo) : mRepo(repo) {
  git_config *config = nullptr;
  if (git_repository_config_snapshot(&config, repo))
    return;

  int enabled = 0;
  if (!git_config_get_bool(&enabled, config, "commit.gpgsign"))
    mEnabled = enabled;

  mFormat = configString(config, "gpg.format").toLower();
  if (mFormat.isEmpty())
    mFormat = "openpgp";

  if (mFormat == "openpgp") {
    mProgram = configString(config, "gpg.openpgp.program");
    if (mProgram.isEmpty())
      mProgram = configString(config, "gpg.program");
    if (mProgram.isEmpty())
      mProgram = "gpg";
  } else if (mFormat == "ssh") {
    mProgram = configString(config, "gpg.ssh.program");
    if (mProgram.isEmpty())
      mProgram = "ssh-keygen";
  }

  mProgram = expandPath(mProgram);
  mKey = configString(config, "user.signingkey");

  git_config_free(config);
}

int CommitSigner::createSignedCommit(git_oid *out, const git_signature *author,
                                     const git_signature *committer,
                                     const char *encoding, const char *message,
                                     const git_tree *tree, size_t parentCount,
                                     const git_commit *parents[]) const {
  git_buf buf = GIT_BUF_INIT;
  if (int error =
          git_commit_create_buffer(&buf, mRepo, author, committer, encoding,
                                   message, tree, parentCount, parents))
    return error;

  QByteArray content(buf.ptr, buf.size);
  git_buf_dispose(&buf);

  QByteArray signature;
  QString error;
  if (!sign(content, committer, signature, error)) {
    sLastError = error;
    git_error_set_str(
        GIT_ERROR_OS,
        QObject::tr("Failed to sign commit: %1").arg(error).toUtf8());
    return -1;
  }

  sLastError.clear();
  return git_commit_create_with_signature(out, mRepo, content.constData(),
                                          signature.constData(), nullptr);
}

QString CommitSigner::takeLastError() {
  QString error = sLastError;
  sLastError.clear();
  return error;
}

bool CommitSigner::sign(const QByteArray &content,
                        const git_signature *committer, QByteArray &signature,
                        QString &error) const {
  bool result = false;
  if (mFormat == "openpgp") {
    // Like git, fall back to the committer identity if no key is set.
    QString key = mKey;
    if (key.isEmpty())
      key = QString("%1 <%2>").arg(QString::fromUtf8(committer->name),
                                   QString::fromUtf8(committer->email));
    result = signOpenPgp(content, key, signature, error);
  } else if (mFormat == "ssh") {
    result = signSsh(content, signature, error);
  } else if (mFormat == "x509") {
    error = QObject::tr("gpg.format 'x509' is not supported");
  } else {
    error = QObject::tr("invalid gpg.format value '%1'").arg(mFormat);
  }

  if (!result)
    return false;

  // The header continuation lines are added by libgit2. A trailing newline
  // would add an extra empty line that git itself doesn't write.
  signature.replace("\r\n", "\n");
  while (signature.endsWith('\n'))
    signature.chop(1);

  if (signature.isEmpty()) {
    error = QObject::tr("'%1' produced an empty signature").arg(mProgram);
    return false;
  }

  return true;
}

bool CommitSigner::signOpenPgp(const QByteArray &content, const QString &key,
                               QByteArray &signature, QString &error) const {
  // The status output goes to stderr. Check that a signature was really
  // created, as git does.
  QStringList args = {"--status-fd=2", "-bsau", key};
  if (run(mProgram, args, content, signature, error) &&
      error.contains("[GNUPG:] SIG_CREATED ")) {
    error.clear();
    return true;
  }

  // Only keep the human readable messages.
  QStringList lines = error.split('\n');
  lines.removeIf(
      [](const QString &line) { return line.startsWith("[GNUPG:]"); });
  error = lines.join('\n').trimmed();
  if (error.isEmpty())
    error = QObject::tr("'%1' failed to sign the data").arg(mProgram);
  return false;
}

bool CommitSigner::signSsh(const QByteArray &content, QByteArray &signature,
                           QString &error) const {
  if (mKey.isEmpty()) {
    error = QObject::tr("user.signingkey needs to be set for ssh signing");
    return false;
  }

  QTemporaryDir dir;
  if (!dir.isValid()) {
    error = dir.errorString();
    return false;
  }

  // The key is either a path to a key file or a literal public key
  // (prefixed with "key::" or starting with "ssh-") whose private key is
  // held by ssh-agent.
  QString keyFile;
  bool literal = false;
  QString key = mKey;
  if (key.startsWith("key::")) {
    key = key.mid(5);
    literal = true;
  } else if (key.startsWith("ssh-")) {
    literal = true;
  }

  if (literal) {
    keyFile = dir.filePath("signing_key.pub");
    QFile file(keyFile);
    if (!file.open(QIODevice::WriteOnly) || file.write(key.toUtf8()) < 0) {
      error = file.errorString();
      return false;
    }
  } else {
    keyFile = expandPath(key);
  }

  QString bufferFile = dir.filePath("signing_buffer");
  QFile buffer(bufferFile);
  if (!buffer.open(QIODevice::WriteOnly) || buffer.write(content) < 0) {
    error = buffer.errorString();
    return false;
  }
  buffer.close();

  QStringList args = {"-Y", "sign", "-n", "git", "-f", keyFile};
  if (literal)
    args.append("-U");
  args.append(bufferFile);

  QByteArray output;
  if (!run(mProgram, args, QByteArray(), output, error)) {
    if (error.contains("usage:"))
      error = QObject::tr("ssh-keygen -Y sign is needed for ssh signing "
                          "(available in openssh version 8.2p1+)");
    if (error.isEmpty())
      error = QObject::tr("'%1' failed to sign the data").arg(mProgram);
    return false;
  }

  QFile sig(bufferFile + ".sig");
  if (!sig.open(QIODevice::ReadOnly)) {
    error = QObject::tr("Unable to read the ssh signature: %1")
                .arg(sig.errorString());
    return false;
  }

  signature = sig.readAll();
  error.clear();
  return true;
}

} // namespace git
