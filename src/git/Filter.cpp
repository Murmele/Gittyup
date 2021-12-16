//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Filter.h"
#include "Command.h"
#include "Config.h"
#include "git2/errors.h"
#include "git2/filter.h"
#include "git2/repository.h"
#include "git2/sys/filter.h"
#include "git2/deprecated.h"
#include <QMap>
#include <QProcess>

namespace git {

namespace {

QString kFilterFmt = "filter=%1";

struct FilterInfo
{
  FilterInfo()
    : filter(GIT_FILTER_INIT)
  {}

  git_filter filter;

  QString clean;
  QString smudge;
  bool required = false;

  QByteArray name;
  QByteArray attributes;
};

QString quote(const QString &path)
{
  return QString("\"%1\"").arg(path);
}

int apply(
  git_filter *self,
  void **payload,
  git_buf *to,
  const git_buf *from,
  const git_filter_source *src)
{
  FilterInfo *info = reinterpret_cast<FilterInfo *>(self);
  git_filter_mode_t mode = git_filter_source_mode(src);
  QString command = (mode == GIT_FILTER_SMUDGE) ? info->smudge : info->clean;

  // Substitute path.
  command.replace("%f", quote(git_filter_source_path(src)));

  QString bash = Command::bashPath();
  if (bash.isEmpty())
    return info->required ? GIT_EUSER : GIT_PASSTHROUGH;

  QProcess process;
  git_repository *repo = git_filter_source_repo(src);
  process.setWorkingDirectory(git_repository_workdir(repo));

  process.start(bash, {"-c", command});
  if (!process.waitForStarted())
    return info->required ? GIT_EUSER : GIT_PASSTHROUGH;

  process.write(from->ptr, from->size);
  process.closeWriteChannel();

  if (!process.waitForFinished() || process.exitCode()) {
    git_error_set_str(GIT_ERROR_FILTER, process.readAllStandardError());
    return info->required ? GIT_EUSER : GIT_PASSTHROUGH;
  }

  QByteArray data = process.readAll();
  git_buf_set(to, data.constData(), data.length());
  return 0;
}

} // anon. namespace

void Filter::init()
{
  static QMap<QString,FilterInfo> filters;

  // Read global filters.
  Config config = Config::global();
  Config::Iterator it = config.glob("filter\\..*\\..*");
  while (git::Config::Entry entry = it.next()) {
    QString name = entry.name().section('.', 1, 1);
    QString key = entry.name().section('.', 2, 2);
    if (key == "clean") {
      filters[name].clean = entry.value<QString>();
    } else if (key == "smudge") {
      filters[name].smudge = entry.value<QString>();
    } else if (key == "required") {
      filters[name].required = entry.value<bool>();
    }
  }

  // Register filters.
  foreach (const QString &key, filters.keys()) {
    FilterInfo &info = filters[key];
    if (info.clean.isEmpty() || info.smudge.isEmpty())
      continue;

    info.name = key.toUtf8();
    info.attributes = kFilterFmt.arg(key).toUtf8();

    info.filter.apply = &apply;
    info.filter.attributes = info.attributes.constData();
    git_filter_register(
      info.name.constData(), &info.filter, GIT_FILTER_DRIVER_PRIORITY);
  }
}

} // namespace git
