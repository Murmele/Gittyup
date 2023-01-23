//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

class RepoView;
class QActionGroup;
class QStackedWidget;

class ConfigDialog : public QDialog {
  Q_OBJECT

public:
  enum Index {
    General,
    Diff,
    Remotes,
    Branches,
    Submodules,
    Search,
    Plugins,
    Lfs
  };

  ConfigDialog(RepoView *view, Index index = General);

  void addRemote(const QString &name);
  void editBranch(const QString &name);

protected:
  void showEvent(QShowEvent *event) override;

  QStackedWidget *mStack;
  QActionGroup *mActions;
};

#endif
