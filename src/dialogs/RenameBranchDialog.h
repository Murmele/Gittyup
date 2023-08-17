//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef RENAMEBRANCHDIALOG_H
#define RENAMEBRANCHDIALOG_H

#include "git/Branch.h"
#include <QDialog>

class QLineEdit;

namespace git {
class Reference;
class Repository;
} // namespace git

class RenameBranchDialog : public QDialog {
  Q_OBJECT

public:
  RenameBranchDialog(const git::Repository &repo,
                  const git::Branch &branch,
                  QWidget *parent = nullptr);

  QString name() const;

private:
  QLineEdit *mName;
};

#endif
