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
#include "git/Blob.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QDebug>

DiffTool::DiffTool(const QStringList &files, const git::Diff &diff,
                   const git::Repository &repo, QObject *parent)
    : ExternalTool(files, diff, repo, parent) {}

bool DiffTool::isValid() const {
  bool isValid = ExternalTool::isValid();
  foreach (const QString &file, mFiles) {
    git::Blob fileBlob;
    isValid &= DiffTool::getBlob(file, git::Diff::OldFile, fileBlob) &
               fileBlob.isValid();
  };
  return isValid;
}

ExternalTool::Kind DiffTool::kind() const { return Diff; }

QString DiffTool::name() const { return tr("External Diff"); }

bool DiffTool::start() {
  Q_ASSERT(isValid());

  bool shell = false;
  QString command = lookupCommand("diff", shell);
  if (command.isEmpty())
    return false;

  bool isWorkDirDiff = mDiff.isValid() && mDiff.isStatusDiff();

  int numFiles = mFiles.size();
  foreach (const QString &filePathAndName, mFiles) {
    git::Blob filePathOld, filePathNew;
    if (!getBlob(filePathAndName, git::Diff::OldFile, filePathOld) ||
        !getBlob(filePathAndName, git::Diff::NewFile, filePathNew))
      continue;

    // Get the path to the file (either a full or relative path).
    QString otherPathAndName = filePathAndName;
    if (filePathOld.isValid()) {
      otherPathAndName = makeBlobTempFullFilePath(filePathAndName, filePathOld);
      if (otherPathAndName.isEmpty())
        return false;
    }

    // Destroy this after process finishes.
    QProcess *process = new QProcess();
    process->setProcessChannelMode(
        QProcess::ProcessChannelMode::ForwardedChannels);
    auto signal = QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished);
    QObject::connect(process, signal, [this, process, &numFiles] {
      qDebug() << "Merge Process Exited!";
      qDebug() << "Stdout: " << process->readAllStandardOutput();
      qDebug() << "Stderr: " << process->readAllStandardError();
      if (--numFiles == 0) {
        deleteLater();
      }
    });

    // Convert to absolute path.
    QString fullFilePath =
        isWorkDirDiff ? mRepo.workdir().filePath(filePathAndName)
                      : makeBlobTempFullFilePath(filePathAndName, filePathNew);

#if defined(FLATPAK) || defined(DEBUG_FLATPAK)
    QStringList arguments = {"--host", "--env=LOCAL=" + fullFilePath,
                             "--env=REMOTE=" + otherPathAndName,
                             "--env=MERGED=" + filePathAndName, 
                             "--env=BASE=" + filePathAndName};
    arguments.append("sh");
    arguments.append("-c");
    arguments.append(command);
    process->start("flatpak-spawn", arguments);
#else

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LOCAL", fullFilePath);
    env.insert("REMOTE", otherPathAndName);
    env.insert("MERGED", filePathAndName);
    env.insert("BASE", filePathAndName);
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
#endif

    if (!process->waitForStarted()) {
      qDebug() << "DiffTool starting failed";
      return false;
    }
  }

  // Detach from parent.
  setParent(nullptr);

  return true;
}

bool DiffTool::getBlob(const QString &file, const git::Diff::File &version,
                       git::Blob &blob) const {
  int index = mDiff.indexOf(file);
  if (index < 0)
    return false;

  blob = mRepo.lookupBlob(mDiff.id(index, version));
  return true;
}

QString DiffTool::makeBlobTempFullFilePath(const QString &filePathAndName,
                                           const git::Blob &fileBlob) {
  QString blobTmpFullFilePath;

  QFileInfo fileInfo(filePathAndName);
  QString templatePath = QDir::temp().filePath(fileInfo.fileName());
  QTemporaryFile *temp = new QTemporaryFile(templatePath, this);
  if (temp->open()) {
    temp->write(fileBlob.content());
    temp->flush();
    blobTmpFullFilePath = temp->fileName();
  }

  return blobTmpFullFilePath;
}
