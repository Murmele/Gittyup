//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Diff.h"
#include "Patch.h"
#include "Debug.h"
#include "git2/patch.h"
#include <algorithm>

bool containsPath(QString &str, QString &occurence, Qt::CaseSensitivity cs) {
  if (str.contains(occurence, cs)) {
    if (str.count() == occurence.count()) {
      // file/folder matches exactly
      return true;
    } else if (str.count() >= occurence.length() + 1 &&
               str[occurence.length()] == "/") {
      // file or folder in occurence
      return true;
    }
  }
  return false;
}

namespace git {

int Diff::Callbacks::progress(const git_diff *diff, const char *oldPath,
                              const char *newPath, void *payload) {
  Diff::Callbacks *cbs = reinterpret_cast<Diff::Callbacks *>(payload);
  return cbs->progress(oldPath, newPath) ? 0 : -1;
}

Diff::Data::Data(git_diff *diff) : diff(diff) { resetMap(); }

Diff::Data::~Data() { git_diff_free(diff); }

void Diff::Data::resetMap() {
  map.clear();
  int count = git_diff_num_deltas(diff);
  for (int i = 0; i < count; ++i)
    map.append(i);
}

const git_diff_delta *Diff::Data::delta(int index) const {
  return git_diff_get_delta(diff, map.at(index));
}

Diff::Diff() {}

Diff::Diff(git_diff *diff) : d(diff ? new Data(diff) : nullptr) {}

Diff::operator git_diff *() const { return d->diff; }

typedef struct {
  QString header;
  QList<QString> lines;
} _Hunk;

typedef struct {
  QList<_Hunk> hunks;
} _File;

typedef struct {
  QList<_File> files;
} diff_data;

int each_file_cb(const git_diff_delta *delta, float progress, void *payload) {
  diff_data *d = (diff_data *)payload;
  _File file;
  d->files.append(file);
  return 0;
}

int each_hunk_cb(const git_diff_delta *delta, const git_diff_hunk *hunk,
                 void *payload) {
  diff_data *d = (diff_data *)payload;
  _Hunk _hunk;
  _hunk.header = hunk->header;
  d->files.last().hunks.append(_hunk);
  return 0;
}

int each_line_cb(const git_diff_delta *delta, const git_diff_hunk *hunk,
                 const git_diff_line *line, void *payload) {
  diff_data *d = (diff_data *)payload;
  const char *chr = line->content;
  const char origin = line->origin;

  QByteArray ba(chr, line->content_len);
  QString pld(origin);
  pld.append(ba);

  d->files.last().hunks.last().lines.append(pld);

  return 0;
}

QByteArray Diff::print() {
  diff_data data;
  git_diff_foreach(d->diff, each_file_cb, nullptr, each_hunk_cb, each_line_cb,
                   &data);

  QByteArray diff;
  for (auto file : data.files) {
    for (auto hunk : file.hunks) {
      diff.append(hunk.header.toUtf8());

      for (auto line : hunk.lines)
        diff.append(line.toUtf8());
    }
  }
  Debug(QString(diff));
  return diff;
}

bool Diff::isConflicted() const {
  int count = this->count();
  for (int i = 0; i < count; ++i) {
    if (status(i) == GIT_DELTA_CONFLICTED)
      return true;
  }

  return false;
}

bool Diff::isStatusDiff() const { return d->index.isValid(); }

void Diff::setIndex(const Index &index) { d->index = index; }

int Diff::count() const { return git_diff_num_deltas(d->diff); }

Patch Diff::patch(int index) const {
  git_patch *patch = nullptr;
  git_patch_from_diff(&patch, d->diff, d->map.at(index));
  return Patch(patch);
}

QString Diff::name(int index) const { return d->delta(index)->new_file.path; }

bool Diff::isBinary(int index) const {
  return d->delta(index)->flags & GIT_DIFF_FLAG_BINARY;
}

git_delta_t Diff::status(int index) const { return d->delta(index)->status; }

Id Diff::id(int index, File file) const {
  const git_diff_delta *delta = d->delta(index);
  return (file == NewFile) ? delta->new_file.id : delta->old_file.id;
}

int Diff::indexOf(const QString &name) const {
  int count = this->count();
  for (int i = 0; i < count; ++i) {
    if (name == this->name(i))
      return i;
  }

  return -1;
}

void Diff::merge(const Diff &diff) {
  git_diff_merge(d->diff, diff);
  d->resetMap();
}

void Diff::findSimilar(bool untracked) {
  git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
  if (untracked)
    opts.flags = GIT_DIFF_FIND_FOR_UNTRACKED;

  if (!isValid())
    return;

  git_diff_find_similar(d->diff, &opts);
  d->resetMap();
}

void Diff::sort(SortRole role, Qt::SortOrder order) {
  bool ascending = (order == Qt::AscendingOrder);
  std::sort(
      d->map.begin(), d->map.end(), [this, role, ascending](int lhs, int rhs) {
        switch (role) {
          case NameRole: {
            QString lhsName = git_diff_get_delta(d->diff, lhs)->new_file.path;
            QString rhsName = git_diff_get_delta(d->diff, rhs)->new_file.path;
            return ascending ? (lhsName < rhsName) : (rhsName < lhsName);
          }

          case StatusRole: {
            git_delta_t lhsStatus = git_diff_get_delta(d->diff, lhs)->status;
            git_delta_t rhsStatus = git_diff_get_delta(d->diff, rhs)->status;
            return ascending ? (lhsStatus < rhsStatus)
                             : (rhsStatus < lhsStatus);
          }
        }
        throw std::runtime_error("unreachable; value=" +
                                 std::to_string(static_cast<int>(role)));
      });
}

void Diff::setAllStaged(bool staged, bool yieldFocus) {
  QStringList paths;
  for (int i = 0; i < count(); ++i)
    paths.append(name(i));
  index().setStaged(paths, staged, yieldFocus);
}

char Diff::statusChar(git_delta_t status) {
  if (status == GIT_DELTA_CONFLICTED)
    return '!';

  return git_diff_status_char(status);
}

QByteArray Diff::toBuffer(git_diff_format_t format) const {
  git_buf buf = {nullptr, 0, 0};
  if (git_diff_to_buf(&buf, d->diff, format))
    return QByteArray();

  QByteArray text(buf.ptr, buf.size);
  git_buf_dispose(&buf);
  return text;
}

QByteArray Diff::toBuffer(const QStringList &files,
                          git_diff_format_t format) const {
  git_buf buf = {nullptr, 0, 0};
  if (git_diff_to_buf(&buf, d->diff, format))
    return QByteArray();

  QByteArray text(buf.ptr, buf.size);
  git_buf_dispose(&buf);

  if (!files.isEmpty()) {
    QString diff(text);
    QStringList lines(diff.split('\n'));

    for (int i = 0; i < lines.count(); i++) {
      if (lines.at(i).startsWith("diff --git")) {
        bool keep = false;
        for (int f = 0; f < files.count(); f++) {
          if (lines.at(i).endsWith(files.at(f))) {
            keep = true;
            break;
          }
        }
        if (!keep) {
          while (i < lines.count()) {
            lines.removeAt(i);
            if ((i >= lines.count()) || lines.at(i).startsWith("diff --git"))
              break;
          }
          i--;
        }
      }
    }
    text.clear();
    text = lines.join('\n').toUtf8();
  }
  return text;
}

Diff Diff::fromBuffer(const QByteArray &text) {
  git_diff *diff = nullptr;
  git_diff_from_buffer(&diff, text.constData(), text.size());
  return Diff(diff);
}

} // namespace git
