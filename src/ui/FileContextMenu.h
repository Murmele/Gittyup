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

class ExternalTool;
class RepoView;

class FileContextMenu : public QMenu {
  Q_OBJECT

public:
  FileContextMenu(RepoView *view, const QStringList &files,
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

  RepoView *mView;
  QStringList mFiles;

  friend class TestTreeView;
};

#endif
