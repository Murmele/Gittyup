//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef GIT_INDEX_H
#define GIT_INDEX_H

#include "Id.h"
#include "git2/index.h"
#include <QMap>
#include <QSet>
#include <QSharedPointer>

namespace git {

class Tree;

class Index {
public:
  enum StagedState {
    Disabled = -1,
    Unstaged,
    PartiallyStaged,
    Staged,
    Conflicted
  };

  struct Conflict {
    Id ancestor;
    Id ours;
    Id theirs;

    bool isValid() const {
      return !ancestor.isNull() && !ours.isNull() && !theirs.isNull();
    }
  };

  Index();

  bool isValid() const { return !d.isNull(); }

  Conflict conflict(const QString &path) const;

  git_filemode_t mode(const QString &path) const;
  void setMode(const QString &path, git_filemode_t mode);

  bool isTracked(const QString &path) const;
  StagedState isStaged(const QString &path) const;
  void setStaged(const QStringList &paths, bool staged, bool yieldFocus = true);

  void add(const QString &path, const QByteArray &buffer);

  void read();
  Tree writeTree() const;

  bool hasConflicts() const;

  static Index create();

private:
  struct Data {
    Data(git_index *index);
    ~Data();

    git_index *index;

    QMap<QString, StagedState> stagedCache;
  };

  Index(git_index *index);
  operator git_index *() const;

  const git_index_entry *entry(const QString &path, int stage = 0) const;

  bool addDirectory(const QString &path) const;

  Id headId(const QString &path, uint32_t *mode = nullptr) const;
  Id indexId(const QString &path, uint32_t *mode = nullptr) const;
  Id workdirId(const QString &path, uint32_t *mode = nullptr) const;

  QSharedPointer<Data> d;

  friend class Repository;
  friend class Tree;
};

} // namespace git

#endif
