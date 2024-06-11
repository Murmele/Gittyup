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
#include "git2/sys/errors.h"
#include <QMap>
#include <QProcess>

namespace git {

namespace {

QString kFilterFmt = "filter=%1";

struct FilterInfo {
  FilterInfo() : filter(GIT_FILTER_INIT) {}

  git_filter filter;

  QString clean;
  QString smudge;
  bool required = false;

  QByteArray name;
  QByteArray attributes;
};

QString quote(const QString &path) { return QString("\"%1\"").arg(path); }

int apply(const git_filter *self, QByteArray &to, const char *from,
          const size_t from_length, const git_filter_source *src) {
  const FilterInfo *info = reinterpret_cast<const FilterInfo *>(self);
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

  process.write(from);
  process.closeWriteChannel();

  if (!process.waitForFinished() || process.exitCode()) {
    git_error_set_str(GIT_ERROR_FILTER, process.readAllStandardError());
    return info->required ? GIT_EUSER : GIT_PASSTHROUGH;
  }

  to = process.readAll();
  return 0;
}

struct Stream {
  git_writestream parent; // must be the first. No pointer!
  git_writestream *next;
  git_filter_mode_t mode;
  const git_filter *filter;
  const git_filter_source *filter_source;
};

static void stream_free(git_writestream *stream) { free(stream); }

static int stream_close(git_writestream *s) {
  struct Stream *stream = (struct Stream *)s;
  stream->next->close(stream->next);
  return 0;
}

static int stream_write(git_writestream *s, const char *buffer, size_t len) {

  struct Stream *stream = (struct Stream *)s;
  QByteArray to;
  apply(stream->filter, to, buffer, len, stream->filter_source);
  stream->next->write(stream->next, to.data(), to.length());
  return 0;
}

// Called for every new stream
static int stream_init(git_writestream **out, git_filter *self, void **payload,
                       const git_filter_source *src, git_writestream *next) {

  struct Stream *stream =
      static_cast<struct Stream *>(calloc(1, sizeof(struct Stream)));
  if (!stream)
    return -1;

  stream->parent.write = stream_write;
  stream->parent.close = stream_close;
  stream->parent.free = stream_free;
  stream->next = next;
  stream->filter_source = src;
  stream->filter = self;

  *out = (git_writestream *)stream;
  return 0;
}

} // namespace

void Filter::init() {
  static QMap<QString, FilterInfo> filters;

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

    info.filter.stream = stream_init;
    info.filter.attributes = info.attributes.constData();
    git_filter_register(info.name.constData(), &info.filter,
                        GIT_FILTER_DRIVER_PRIORITY);
  }
}

} // namespace git
