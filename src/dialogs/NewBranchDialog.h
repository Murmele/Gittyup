//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef NEWBRANCHDIALOG_H
#define NEWBRANCHDIALOG_H

#include "git/Commit.h"
#include <QDialog>

class ReferenceList;
class QLineEdit;
class QCheckBox;

namespace git {
class Reference;
class Repository;
} // namespace git

class NewBranchDialog : public QDialog {
  Q_OBJECT

public:
  NewBranchDialog(const git::Repository &repo,
                  const git::Commit &commit = git::Commit(),
                  QWidget *parent = nullptr);

  QString name() const;
  bool checkout() const;
  git::Commit target() const;
  git::Reference upstream() const;

private:
  QLineEdit *mName;
  ReferenceList *mUpstream;
  ReferenceList *mRefs;
  QCheckBox *mCheckout;
};

#endif
