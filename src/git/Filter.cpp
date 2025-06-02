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
#include "git2/errors.h"
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

struct Stream {
  int init(git_filter* self, const git_filter_source *src, git_writestream *);
  git_writestream parent; // must be the first. No pointer!
  git_writestream *next;
  git_filter_mode_t mode;
  const git_filter *filter;
  const git_filter_source *filter_source;
  QProcess process;
};

static void stream_free(git_writestream *stream) {
  auto *s = reinterpret_cast<struct Stream *>(stream);
  delete s;
}

static int stream_close(git_writestream *s) {
  struct Stream *stream = reinterpret_cast<struct Stream *>(s);
  stream->process.closeWriteChannel();

  const FilterInfo *info = reinterpret_cast<const FilterInfo *>(stream->filter);

  int error = GIT_OK;
  if (!stream->process.waitForFinished() || stream->process.exitCode()) {
    git_error_set_str(GIT_ERROR_FILTER, stream->process.readAllStandardError());
    error = info->required ? GIT_EUSER : GIT_PASSTHROUGH;
  }

  QByteArray writeBuf;
  if (error == GIT_PASSTHROUGH) {
    //writeBuf = &stream->input;
    // TODO: must be implemented!!!!
    // Problem is that we don't have the complete data.
    // Shall we really store the complete data, or maybe just reading the file again?
    assert(false);
    return error;
  } else if (error == 0) {
    writeBuf = stream->process.readAll();
  } else {
    /* close stream before erroring out taking care
     * to preserve the original error */
    // git_error *last_error;
    // git_error_save(&last_error);
    stream->next->close(stream->next);
    // git_error_restore(last_error);
    return error;
  }

  error = stream->next->write(stream->next, writeBuf.data(), writeBuf.length());
  if (error == GIT_OK) {
    error = stream->next->close(stream->next);
  }
  return error;
}

static int stream_write(git_writestream *s, const char *buffer, size_t len) {
  struct Stream *stream = reinterpret_cast<struct Stream *>(s);
  const auto res = stream->process.write(buffer, len);
  if (res != len)
    return -1;
  return 0;
}

int Stream::init(git_filter* self, const git_filter_source * src, git_writestream * next) {
  filter_source = src;
  filter = self;
  this->next = next;
  parent.write = stream_write;
  parent.close = stream_close;
  parent.free = stream_free;

  const FilterInfo *info = reinterpret_cast<const FilterInfo *>(self);
  git_filter_mode_t mode = git_filter_source_mode(src);
  QString command = (mode == GIT_FILTER_SMUDGE) ? info->smudge : info->clean;

         // Substitute path.
  command.replace("%f", quote(git_filter_source_path(src)));

  QString bash = Command::bashPath();
  if (bash.isEmpty())
    return info->required ? GIT_EUSER : GIT_PASSTHROUGH;

  git_repository *repo = git_filter_source_repo(src);
  process.setWorkingDirectory(git_repository_workdir(repo));

  process.start(bash, {"-c", command});
  if (!process.waitForStarted())
    return info->required ? GIT_EUSER : GIT_PASSTHROUGH;
  return 0;
}

// Called for every new stream
static int stream_init(git_writestream **out, git_filter *self, void **payload,
                       const git_filter_source *src, git_writestream *next) {

  struct Stream *stream = new Stream;
  if (!stream)
    return -1;

  int error = stream->init(self, src, next);
  if (error != 0) {
    delete stream;
    return error;
  }

  *out = (git_writestream *)stream;
  return 0;
}

} // namespace

static void shutdown(git_filter *) {

}

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
    info.filter.shutdown = shutdown;
    info.filter.attributes = info.attributes.constData();
    // &info.filter can be used and reinterpret cast afterwards works, because filter is the
    // first member of FilterInfo. This is also proposed in filter.h of libgit2
    git_filter_register(info.name.constData(), &info.filter,
                        GIT_FILTER_DRIVER_PRIORITY);
  }
}

} // namespace git
