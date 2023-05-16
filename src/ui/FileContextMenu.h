//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef FILECONTEXTMENU_H
#define FILECONTEXTMENU_H

#include "git/Id.h"
#include "git/Index.h"
#include "git/Commit.h"
#include <QMenu>
#include <QList>

class ExternalTool;
class RepoView;
class Node;

class AccumRepoFiles {
public:
  AccumRepoFiles(bool staged = false, bool statusDiff = false);
  AccumRepoFiles(const QString &file);
  AccumRepoFiles(const QStringList &files);
  AccumRepoFiles(const AccumRepoFiles &) = default;
  AccumRepoFiles &operator=(const AccumRepoFiles &) = default;
  ~AccumRepoFiles() = default;

  // Get all files that were accumulated individually, not as part of a
  // directory being accumulated.  The files contain the file name with any
  // path provided (relative or absolute).
  const QStringList &getFiles() const;

  // Get all directory paths that we accumulated.
  QStringList getAccumulatedDirs() const;

  // Get a list of files from all accumulated directories or just those from
  // the requested ones.
  QStringList getFilesInDirs(const QStringList &dirs = {}) const;

  // Get a list of all accumulated files with paths.  These include those
  // accumulated individually as well as those that were added as part of
  // accumulating a directory.
  QStringList getAllFiles() const;

  // Accumulate the given node as either an individual file or as a directory.
  // For directories, all the files in the directory will be enumeracted and
  // accumulated, recursively.
  void add(const git::Index &index, const Node *node);

private:
  void addToFileList(QStringList &files, const git::Index &index,
                     const Node *node);
  void addDirFiles(const git::Index &index, const Node *node);
  void addFile(QStringList &files, const git::Index &index,
               const Node *node) const;

  AccumRepoFiles() = delete;
  AccumRepoFiles(AccumRepoFiles &&) = delete;
  AccumRepoFiles(const AccumRepoFiles &&) = delete;
  AccumRepoFiles &&operator=(const AccumRepoFiles &&) = delete;

  typedef QMap<QString, QStringList>::const_iterator ConstQMapIterator;
  QMap<QString, QStringList> mFilesInDirMap;
  QStringList mFiles;
  bool mStaged = false;
  bool mStatusDiff = false;
};

class FileContextMenu : public QMenu {
  Q_OBJECT

public:
  FileContextMenu(RepoView *view, const AccumRepoFiles &accumFiles,
                  const git::Index &index = git::Index(),
                  QWidget *parent = nullptr);
private slots:
  void ignoreFile();

private:
  void addExternalToolsAction(const QList<ExternalTool *> &tools);
  bool exportFile(const RepoView *view, const QString &folder,
                  const QString &file);
  void handleUncommittedChanges(const git::Index &index,
                                const QStringList &files);
  void handleCommits(const QList<git::Commit> &commits,
                     const QStringList &files);
  void attachTool(ExternalTool *tool, QList<ExternalTool *> &tools);

  RepoView *mView;
  AccumRepoFiles mAccumFiles;

  friend class TestTreeView;
};
#endif
