//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Kas
//

#ifndef DELETETAGDIALOG_H
#define DELETETAGDIALOG_H

#include <QMessageBox>

namespace git {
class TagRef;
}

class DeleteTagDialog : public QMessageBox {
  Q_OBJECT

public:
  DeleteTagDialog(const git::TagRef &tag, QWidget *parent = nullptr);
};

#endif
