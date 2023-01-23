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

DiffTool::DiffTool(const QString &file, const git::Blob &localBlob,
                   const git::Blob &remoteBlob, QObject *parent)
    : ExternalTool(file, parent), mLocalBlob(localBlob),
      mRemoteBlob(remoteBlob) {}

bool DiffTool::isValid() const {
  return (ExternalTool::isValid() && mLocalBlob.isValid());
}

ExternalTool::Kind DiffTool::kind() const { return Diff; }

QString DiffTool::name() const { return tr("External Diff"); }

bool DiffTool::start() {
  Q_ASSERT(isValid());

  bool shell = false;
  QString command = lookupCommand("diff", shell);
  if (command.isEmpty())
    return false;

  // Write temporary files.
  QString templatePath = QDir::temp().filePath(QFileInfo(mFile).fileName());
  QTemporaryFile *local = new QTemporaryFile(templatePath, this);
  if (!local->open())
    return false;

  local->write(mLocalBlob.content());
  local->flush();

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
  auto signal = QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished);
  QObject::connect(process, signal, this, &ExternalTool::deleteLater);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("LOCAL", local->fileName());
  env.insert("REMOTE", remotePath);
  env.insert("MERGED", mFile);
  env.insert("BASE", mFile);
  process->setProcessEnvironment(env);

  QString bash = git::Command::bashPath();
  if (!bash.isEmpty()) {
    process->start(bash, {"-c", command});
  } else if (!shell) {
    process->start(git::Command::substitute(env, command));
  } else {
    emit error(BashNotFound);
    return false;
  }

  if (!process->waitForStarted())
    return false;

  // Detach from parent.
  setParent(nullptr);

  return true;
}
