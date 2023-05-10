//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "MergeTool.h"
#include "git/Command.h"
#include "git/Config.h"
#include "git/Index.h"
#include "git/Repository.h"
#include "Debug.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>

MergeTool::MergeTool(const QStringList &files, const git::Diff &diff,
                     const git::Repository &repo, QObject *parent)
    : ExternalTool(files, diff, repo, parent) {

  if (!mFiles.empty()) {
    foreach (const QString &file, mFiles) {
      if (isConflicted(file)) {
        git::Index::Conflict conflict = repo.index().conflict(file);
        mMerges.append({file, repo.lookupBlob(conflict.ours),
                        repo.lookupBlob(conflict.theirs),
                        repo.lookupBlob(conflict.ancestor)});
      }
    }
  }
}

bool MergeTool::isValid() const {
  if (!ExternalTool::isValid())
    return false;

  for (const FileMerge &fileMerge : mMerges) {
    if (!fileMerge.local.isValid() || !fileMerge.remote.isValid()) {
      return false;
    }
  }
  return true;
}

ExternalTool::Kind MergeTool::kind() const { return Merge; }

QString MergeTool::name() const { return tr("External Merge"); }

bool MergeTool::start() {
  Q_ASSERT(isValid());

  bool shell = false;
  QString command = lookupCommand("merge", shell);
  if (command.isEmpty())
    return false;

  int numMergeFiles = mMerges.size();
  for (const FileMerge &fileMerge : mMerges) {
    // Write temporary files.
    QString templatePath =
        QDir::temp().filePath(QFileInfo(fileMerge.name).fileName());
    QTemporaryFile *local = new QTemporaryFile(templatePath, this);
    if (!local->open())
      return false;

    local->write(fileMerge.local.content());
    local->flush();

    QTemporaryFile *remote = new QTemporaryFile(templatePath, this);
    if (!remote->open())
      return false;

    remote->write(fileMerge.remote.content());
    remote->flush();

    QString basePath;
    if (fileMerge.base.isValid()) {
      QTemporaryFile *base = new QTemporaryFile(templatePath, this);
      if (!base->open())
        return false;

      base->write(fileMerge.base.content());
      base->flush();

      basePath = base->fileName();
    }

    // Make the backup copy.
    QString backupPath = QString("%1.orig").arg(fileMerge.name);
    if (!QFile::copy(fileMerge.name, backupPath)) {
      // FIXME: What should happen if the backup already exists?
    }

    // Destroy this after process finishes.
    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(
        QProcess::ProcessChannelMode::ForwardedChannels);
    git::Repository repo = fileMerge.local.repo();
    auto signal = QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished);
    QObject::connect(
        process, signal,
        [this, repo, fileMerge, backupPath, process, &numMergeFiles] {
          qDebug() << "Merge Process Exited!";
          qDebug() << "Stdout: " << process->readAllStandardOutput();
          qDebug() << "Stderr: " << process->readAllStandardError();

          QFileInfo merged(fileMerge.name);
          QFileInfo backup(backupPath);
          git::Config config = git::Config::global();
          bool modified = (merged.lastModified() > backup.lastModified());
          if (!modified || !config.value<bool>("mergetool.keepBackup"))
            QFile::remove(backupPath);

          if (modified) {
            int length = repo.workdir().path().length();
            repo.index().setStaged({fileMerge.name.mid(length + 1)}, true);
          }

          if (--numMergeFiles == 0) {
            deleteLater();
          }
        });

    QString fullFilePath = mRepo.workdir().filePath(fileMerge.name);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LOCAL", local->fileName());
    env.insert("REMOTE", remote->fileName());
    env.insert("MERGED", fullFilePath);
    env.insert("BASE", basePath);
    process->setProcessEnvironment(env);

#if defined(FLATPAK) || defined(DEBUG_FLATPAK)
    QStringList arguments = {"--host", "--env=LOCAL=" + local->fileName(),
                             "--env=REMOTE=" + remote->fileName(),
                             "--env=MERGED=" + fullFilePath,
                             "--env=BASE=" + basePath};
    arguments.append("sh");
    arguments.append("-c");
    arguments.append(command);
    // Debug("Command: " << "flatpak-spawn");
    process->start("flatpak-spawn", arguments);
    // Debug("QProcess Arguments: " << process->arguments());
    if (!process->waitForStarted()) {
      Debug("MergeTool starting failed");
      return false;
    }
#else
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
#endif
  }

  // Detach from parent.
  setParent(nullptr);

  return true;
}
