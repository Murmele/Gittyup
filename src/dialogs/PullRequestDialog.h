//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#ifndef PULLREQUESTDIALOG_H
#define PULLREQUESTDIALOG_H

#include <QDialog>

class RepoView;
class QLineEdit;
class QTextEdit;

namespace git {
class Commit;
}

class PullRequestDialog : public QDialog {
  Q_OBJECT

public:
  PullRequestDialog(RepoView *view);

private:
  QLineEdit *mTitle;
  QTextEdit *mBody;

  void setCommit(const git::Commit &commit);
};

#endif
