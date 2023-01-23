//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#ifndef MERGEDIALOG_H
#define MERGEDIALOG_H

#include "git/Repository.h"
#include "ui/RepoView.h"
#include <QDialog>

class ReferenceList;
class QComboBox;
class QPushButton;

namespace git {
class Reference;
}

class MergeDialog : public QDialog {
  Q_OBJECT

public:
  MergeDialog(RepoView::MergeFlags flags, const git::Repository &repo,
              QWidget *parent = nullptr);

  git::Commit target() const;
  git::Reference reference() const;
  RepoView::MergeFlags flags() const;

  void setCommit(const git::Commit &commit);
  void setReference(const git::Reference &ref);

private:
  void update();
  QString labelText() const;
  QString buttonText() const;

  git::Repository mRepo;
  QPushButton *mAccept;
  ReferenceList *mRefs;
  QComboBox *mAction;
};

#endif
