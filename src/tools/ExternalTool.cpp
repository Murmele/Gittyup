//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ExternalTool.h"
#include "DiffTool.h"
#include "MergeTool.h"
#include "conf/Settings.h"
#include "git/Diff.h"
#include "git/Config.h"
#include "git/Index.h"
#include "git/Repository.h"
#include <QStandardPaths>
#include <QProcess>

namespace {

const QString kGlobFmt = "%1tool\\..*\\.cmd";

QChar safeAt(const QString &string, int i) {
  return (i >= 0 && i < string.length()) ? string.at(i) : QChar();
}

void splitCommand(const QString &command, QString &program, QString &args) {
  bool quoted = command.startsWith('"');
  int index = command.indexOf(quoted ? '"' : ' ', quoted ? 1 : 0);
  if (safeAt(command, index) == '"' && safeAt(command, index + 1) == ' ')
    ++index;

  program = command.mid(0, index);
  if (index >= 0)
    args = command.mid(index + 1);
}

} // namespace

ExternalTool::ExternalTool(const QString &file, QObject *parent)
    : QObject(parent), mFile(file) {}

bool ExternalTool::isValid() const { return !mFile.isEmpty(); }

QString ExternalTool::lookupCommand(const QString &key, bool &shell) {
  git::Config config = git::Config::global();
  QString path = Settings::confDir().filePath("mergetools");
  config.addFile(QDir::toNativeSeparators(path), GIT_CONFIG_LEVEL_PROGRAMDATA);

  QString name = config.value<QString>(QString("%1.tool").arg(key));
  if (name.isEmpty())
    return QString();

  shell = config.value<bool>(QString("%1tool.%2.shell").arg(key, name));
  return config.value<QString>(QString("%1tool.%2.cmd").arg(key, name));
}

QList<ExternalTool::Info> ExternalTool::readGlobalTools(const QString &key) {
  QList<Info> tools;
  git::Config config = git::Config::global();
  git::Config::Iterator it = config.glob(kGlobFmt.arg(key));
  while (git::Config::Entry entry = it.next()) {
    QString program, args;
    QString name = entry.name().section(".", 1, 1);
    splitCommand(entry.value<QString>(), program, args);
    tools.append({name, program, args, true});
  }

  return tools;
}

QList<ExternalTool::Info> ExternalTool::readBuiltInTools(const QString &key) {
  QList<Info> tools;
  QDir conf = Settings::confDir();
  git::Config config = git::Config::open(conf.filePath("mergetools"));
  git::Config::Iterator it = config.glob(kGlobFmt.arg(key));
  while (git::Config::Entry entry = it.next()) {
    QString program, args;
    QString name = entry.name().section(".", 1, 1);
    splitCommand(entry.value<QString>(), program, args);

#define TESTING_PROCESS 0
#if defined(FLATPAK) || TESTING_PROCESS
    QProcess process;
    process.start("flatpak-spawn", {"--host", "which", program});
    process.waitForFinished(-1); // will wait forever until finished
    QString path = process.readAllStandardOutput();
#else
    QString path = QStandardPaths::findExecutable(program);
#endif
    tools.append({name, program, args, !path.isEmpty()});
  }

  return tools;
}

ExternalTool *ExternalTool::create(const QString &file, const git::Diff &diff,
                                   const git::Repository &repo, bool againstWorkingDir,
                                   QObject *parent) {
  if (!diff.isValid())
    return nullptr;

  int index = diff.indexOf(file);
  if (index < 0)
    return nullptr;

  // Convert to absolute path.
  QString path = repo.workdir().filePath(file);

  // Create merge tool.
  if (!againstWorkingDir && diff.status(index) == GIT_DELTA_CONFLICTED) {
    git::Index::Conflict conflict = repo.index().conflict(file);
    git::Blob local = repo.lookupBlob(conflict.ours);
    git::Blob remote = repo.lookupBlob(conflict.theirs);
    git::Blob base = repo.lookupBlob(conflict.ancestor);
    return new MergeTool(path, local, remote, base, parent);
  }

  // Create diff tool.
  git::Blob local = repo.lookupBlob(diff.id(index, git::Diff::OldFile));
  git::Blob remote = repo.lookupBlob(diff.id(index, git::Diff::NewFile));
  return againstWorkingDir ? new DiffTool(path, remote, parent) : new DiffTool(path, local, remote, parent);
}
