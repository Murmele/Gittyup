//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Index.h"
#include "Commit.h"
#include "Reference.h"
#include "Repository.h"
#include "Signature.h"
#include "Submodule.h"
#include "Tree.h"
#include "git2/commit.h"
#include "git2/ignore.h"
#include "git2/refs.h"
#include "git2/repository.h"
#include "git2/status.h"
#include "git2/tree.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

namespace git {

namespace {

void countDirectoryEntries(const QString &file, int &count) {
  QDir dir(file);
  auto filters = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot;
  foreach (const QString &entry, dir.entryList(filters)) {
    QString file = dir.filePath(entry);
    if (QFileInfo(file).isDir()) {
      countDirectoryEntries(file, count);
    } else {
      // Cut off at 100 entries.
      count = (count == 100) ? -1 : count + 1;
    }

    if (count < 0)
      return;
  }
}

} // namespace

Index::Data::Data(git_index *index) : index(index) {}

Index::Data::~Data() { git_index_free(index); }

Index::Index() {}

Index::Index(git_index *index) : d(index ? new Data(index) : nullptr) {}

Index::operator git_index *() const { return isValid() ? d->index : nullptr; }

Index::Conflict Index::conflict(const QString &path) const {
  const git_index_entry *ancestor, *ours, *theirs;
  if (git_index_conflict_get(&ancestor, &ours, &theirs, d->index,
                             path.toUtf8()))
    return Conflict();

  Id ancestorId = ancestor ? ancestor->id : Id();
  Id oursId = ours ? ours->id : Id();
  Id theirsId = theirs ? theirs->id : Id();
  return {ancestorId, oursId, theirsId};
}

git_filemode_t Index::mode(const QString &path) const {
  const git_index_entry *entry = this->entry(path);
  if (!entry)
    return GIT_FILEMODE_UNREADABLE;

  return static_cast<git_filemode_t>(entry->mode);
}

void Index::setMode(const QString &path, git_filemode_t mode) {
  const git_index_entry *entry = this->entry(path);
  if (!entry)
    return;

  git_index_entry copy = *entry;
  copy.mode = mode;
  git_index_add(d->index, &copy);
}

bool Index::isTracked(const QString &path) const { return entry(path); }

Index::StagedState Index::isStaged(const QString &path) const {
  QMap<QString, StagedState>::const_iterator it = d->stagedCache.find(path);
  if (it != d->stagedCache.end())
    return it.value();

  Repository repo(git_index_owner(d->index));
  Submodule sm = repo.lookupSubmodule(path);

  // Handle untracked directories.
  QFileInfo info(repo.workdir().filePath(path));
  if (!sm.isValid() && !info.isSymLink() && info.isDir())
    return d->stagedCache.insert(path, Unstaged).value();

  uint32_t headMode = GIT_FILEMODE_UNREADABLE;
  uint32_t indexMode = GIT_FILEMODE_UNREADABLE;
  Id head = sm.isValid() ? sm.headId() : headId(path, &headMode);
  Id index = sm.isValid() ? sm.indexId() : indexId(path, &indexMode);

  // Handle untracked files.
  if (!head.isValid() && !index.isValid())
    return d->stagedCache.insert(path, Unstaged).value();

  uint32_t workdirMode = GIT_FILEMODE_UNREADABLE;
  Id workdir = sm.isValid() ? sm.workdirId() : workdirId(path, &workdirMode);

  // Handle filter callback error.
  if (!workdir.isValid())
    return d->stagedCache.insert(path, Unstaged).value();

  // Handle dirty submodules.
  if (sm.isValid() && head == workdir)
    return d->stagedCache.insert(path, Disabled).value();

  if (index == workdir && indexMode == workdirMode)
    return d->stagedCache.insert(path, Staged).value();

  if (head == index && headMode == indexMode)
    return d->stagedCache.insert(path, Unstaged).value();

  if (conflict(path).isValid())
    return d->stagedCache.insert(path, Conflicted).value();

  return d->stagedCache.insert(path, PartiallyStaged).value();
}

void Index::setStaged(const QStringList &files, bool staged, bool yieldFocus) {
  bool dirAdded = false;
  QStringList changedFiles;
  Repository repo(git_index_owner(d->index));
  RepositoryNotifier *notifier = repo.notifier();
  foreach (const QString &file, files) {
    QByteArray path = file.toUtf8();

    // Get the id and mode of the file in the HEAD commit.
    Id fileId;
    bool fileExists = false;
    uint32_t fileMode = GIT_FILEMODE_UNREADABLE;
    if (staged) {
      fileExists = repo.workdir().exists(file);
    } else {
      // Lookup HEAD commit.
      fileId = headId(file, &fileMode);
      fileExists = !fileId.isNull();
    }

    // submodule
    if (Submodule submodule = repo.lookupSubmodule(file)) {
      if (staged) {
        if (git_submodule_add_to_index(submodule, false))
          continue;
      } else {
        if (fileExists) {
          // Reset to HEAD id.
          Id headId = submodule.headId();

          git_index_entry entry;
          memset(&entry, 0, sizeof(git_index_entry));

          entry.path = path;
          entry.mode = fileMode;
          git_oid_cpy(&entry.id, headId);

          // Set timestamps.
          Repository smRepo = submodule.open();
          if (!smRepo.isValid())
            continue;

          Commit smHead = smRepo.lookupCommit(headId);
          if (!smHead.isValid())
            continue;

          git_time_t time = smHead.committer().date().toSecsSinceEpoch();
          entry.ctime.seconds = time;
          entry.ctime.nanoseconds = 0;
          entry.mtime.seconds = time;
          entry.mtime.nanoseconds = 0;

          if (git_index_add(d->index, &entry))
            continue;
        } else {
          // Remove from index.
          if (git_index_remove_bypath(d->index, path))
            continue;
        }
      }

      repo.invalidateSubmoduleCache();
      changedFiles.append(file);
      continue;
    }

    // regular file
    if (staged) {
      if (fileExists) {
        QDir dir = repo.workdir();
        QFileInfo info(dir.filePath(file));
        if (!info.isSymLink() && info.isDir()) {
          // FIXME: This is a workaround.
          QString current = QDir::currentPath();
          QDir::setCurrent(dir.path());

          int count = 0;
          bool allow = true;
          countDirectoryEntries(file, count);
          emit notifier->directoryAboutToBeStaged(file, count, allow);
          bool added = (allow && addDirectory(file));

          QDir::setCurrent(current);

          if (!added)
            continue;

          dirAdded = true;

        } else {
          int size = QFileInfo(repo.workdir(), file).size();
          bool allow = true;
          if (size > 10000000) // 10MB
            emit notifier->largeFileAboutToBeStaged(file, size, allow);

          if (!allow)
            continue;

          if (git_index_add_bypath(d->index, path)) {
            emit notifier->indexStageError(file);
            continue;
          }
        }

      } else {
        // Remove from index.
        if (git_index_remove_bypath(d->index, path)) {
          emit notifier->indexStageError(file);
          continue;
        }
      }

    } else {
      if (fileExists) {
        // Reset to HEAD tree entry.
        git_index_entry entry;
        memset(&entry, 0, sizeof(git_index_entry));

        entry.path = path;
        entry.mode = fileMode;
        git_oid_cpy(&entry.id, fileId);

        if (git_index_add(d->index, &entry)) {
          emit notifier->indexStageError(file);
          continue;
        }
      } else {
        // Remove from index.
        if (git_index_remove_bypath(d->index, path)) {
          emit notifier->indexStageError(file);
          continue;
        }
      }
    }

    changedFiles.append(file);
  }

  if (!changedFiles.isEmpty()) {
    git_index_write(d->index);
    foreach (const QString &changedFile, changedFiles)
      d->stagedCache.remove(changedFile);
    emit notifier->indexChanged(changedFiles, yieldFocus);
  }

  if (dirAdded)
    emit notifier->directoryStaged();
}

void Index::add(const QString &path, const QByteArray &buffer) {
  const git_index_entry *entry = this->entry(path);
  if (!entry) {
    git_index_add_bypath(d->index, path.toUtf8());
    entry = this->entry(path);
  }

  if (!entry)
    return;

  if (git_index_add_from_buffer(d->index, entry, buffer, buffer.size()))
    return;

  git_index_write(d->index);
  d->stagedCache.remove(path);
  git::Repository repo(git_index_owner(d->index));
  emit repo.notifier()->indexChanged({path});
}

void Index::read() { git_index_read(d->index, false); }

Tree Index::writeTree() const {
  // Write the index tree.
  git_oid id;
  if (git_index_write_tree(&id, d->index))
    return Tree();

  git_tree *tree = nullptr;
  git_repository *repo = git_index_owner(d->index);
  git_tree_lookup(&tree, repo, &id);
  return Tree(tree);
}

bool Index::hasConflicts() const { return git_index_has_conflicts(d->index); }

Index Index::create() {
  git_index *index = nullptr;
  git_index_new(&index);
  return Index(index);
}

const git_index_entry *Index::entry(const QString &path, int stage) const {
  return git_index_get_bypath(d->index, path.toUtf8(), stage);
}

bool Index::addDirectory(const QString &file) const {
  QDir dir(file);
  git_repository *repo = git_index_owner(d->index);
  auto filters = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot;
  foreach (const QString &entry, dir.entryList(filters)) {
    QString file = dir.filePath(entry);
    if (QFileInfo(file).isDir()) {
      if (!addDirectory(file))
        return false;
      continue;
    }

    int ignored = 0;
    QByteArray path = file.toUtf8();
    if (git_ignore_path_is_ignored(&ignored, repo, path) || ignored)
      continue;

    if (git_index_add_bypath(d->index, path))
      return false;
  }

  return true;
}

Id Index::headId(const QString &path, uint32_t *mode) const {
  Repository repo(git_index_owner(d->index));
  Reference head = repo.head();
  if (!head.isValid())
    return Id();

  Commit commit = head.target();
  if (!head.isValid())
    return Id();

  git_tree_entry *entry = nullptr;
  const auto tree = commit.tree();
  if (!tree.isValid())
    return Id(); // broken repository?
  if (git_tree_entry_bypath(&entry, tree, path.toUtf8()))
    return Id();

  if (mode)
    *mode = git_tree_entry_filemode_raw(entry);

  Id id(git_tree_entry_id(entry));

  git_tree_entry_free(entry);

  return id;
}

Id Index::indexId(const QString &path, uint32_t *mode) const {
  const git_index_entry *entry = this->entry(path);
  if (!entry)
    return Id();

  if (mode)
    *mode = entry->mode;

  return entry->id;
}

Id Index::workdirId(const QString &path, uint32_t *mode) const {
  Repository repo(git_index_owner(d->index));

  git_oid id;
  if (int error = git_repository_hashfile(&id, repo, path.toUtf8(),
                                          GIT_OBJECT_BLOB, nullptr))
    return (error == GIT_EUSER) ? Id::invalidId() : Id();

  if (mode) {
    *mode = GIT_FILEMODE_BLOB;
    QFileInfo info(repo.workdir().filePath(path));
#ifndef Q_OS_WIN
    if (info.isExecutable())
      *mode = GIT_FILEMODE_BLOB_EXECUTABLE;
#endif
    if (info.isSymLink())
      *mode = GIT_FILEMODE_LINK;
  }

  return id;
}

} // namespace git
