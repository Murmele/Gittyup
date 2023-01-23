//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DELETEBRANCHDIALOG_H
#define DELETEBRANCHDIALOG_H

#include <QMessageBox>

namespace git {
class Branch;
}

class DeleteBranchDialog : public QMessageBox {
  Q_OBJECT

public:
  DeleteBranchDialog(const git::Branch &branch, QWidget *parent = nullptr);
};

#endif
