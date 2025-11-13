//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DiffTool.h"
#include "git/Command.h"
#include "git/Repository.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QDebug>

DiffTool::DiffTool(const QString &file, const git::Blob &localBlob,
                   const git::Blob &remoteBlob, QObject *parent)
    : ExternalTool(file, parent), mLocalBlob(localBlob),
      mRemoteBlob(remoteBlob) {}

DiffTool::DiffTool(const QString &file, const git::Blob &remoteBlob,
                   QObject *parent)
    : ExternalTool(file, parent), mLocalBlob(remoteBlob) {
  Q_ASSERT(!mRemoteBlob);
} // make local file the right side

bool DiffTool::isValid() const {
  return (ExternalTool::isValid() && mLocalBlob.isValid());
}

ExternalTool::Kind DiffTool::kind() const { return Diff; }

QString DiffTool::name() const {
  return mRemoteBlob ? tr("External Diff")
                     : tr("External Diff to working copy");
}

bool DiffTool::start() {
  Q_ASSERT(isValid());

  bool shell = false;
  QString command = lookupCommand("diff", shell);
  if (command.isEmpty())
    return false;

  // Write temporary files.
  QString templatePath = QDir::temp().filePath(QFileInfo(mFile).fileName());
  QTemporaryFile *local = nullptr;
  if (mLocalBlob.isValid()) {
    local = new QTemporaryFile(templatePath, this);
    if (!local->open())
      return false;

    local->write(mLocalBlob.content());
    local->flush();
  }

  QString remotePath;
  if (!mRemoteBlob.isValid()) {
    remotePath = mFile;
  } else {
    QTemporaryFile *remote = new QTemporaryFile(templatePath, this);
    if (!remote->open())
      return false;

    remote->write(mRemoteBlob.content());
    remote->flush();

    remotePath = remote->fileName();
  }

  // Destroy this after process finishes.
  QProcess *process = new QProcess(this);
  process->setProcessChannelMode(
      QProcess::ProcessChannelMode::ForwardedChannels);
  auto signal = QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished);
  QObject::connect(process, signal, [this, process] {
    qDebug() << "Merge Process Exited!";
    qDebug() << "Stdout: " << process->readAllStandardOutput();
    qDebug() << "Stderr: " << process->readAllStandardError();
    deleteLater();
  });

#if defined(FLATPAK) || defined(DEBUG_FLATPAK)
  QStringList arguments = {"--host",
                           QStringLiteral("--env=LOCAL=") + (local
                               ? local->fileName()
                               : QFileInfo(mFile).absoluteFilePath()),
                           "--env=REMOTE=" + remotePath,
                           "--env=MERGED=" + mFile, "--env=BASE=" + mFile};
  arguments.append("sh");
  arguments.append("-c");
  arguments.append(command);
  process->start("flatpak-spawn", arguments);
#else

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("LOCAL",
             local ? local->fileName() : QFileInfo(mFile).absoluteFilePath());
  env.insert("REMOTE", remotePath);
  env.insert("MERGED", mFile);
  env.insert("BASE", mFile);
  process->setProcessEnvironment(env);

  QString bash = git::Command::bashPath();
  if (!bash.isEmpty()) {
    process->start(bash, {"-c", command});
  } else if (!shell) {
    process->start(git::Command::substitute(env, command), QStringList());
  } else {
    emit error(BashNotFound);
    return false;
  }
#endif

  if (!process->waitForStarted()) {
    qDebug() << "DiffTool starting failed";
    return false;
  }

  // Detach from parent.
  setParent(nullptr);

  return true;
}
