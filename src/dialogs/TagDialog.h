//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TAGDIALOG_H
#define TAGDIALOG_H

#include <QDialog>
#include "git/Remote.h"

class QCheckBox;
class QLineEdit;
class QTextEdit;
class QListWidget;

namespace git {
class Repository;
}

class TagDialog : public QDialog {
  Q_OBJECT

public:
  TagDialog(const git::Repository &repo, const QString &id,
            const git::Remote &remote = git::Remote(),
            QWidget *parent = nullptr);

  bool force() const;
  git::Remote remote() const;
  QString name() const;
  QString message() const;

private:
  git::Remote mRemote;
  QLineEdit *mNameField;
  QCheckBox *mForce;
  QCheckBox *mPush;
  QTextEdit *mMessage;
  QListWidget *mListWidget{nullptr};
  QStringList mExistingTags;
  QStringList mFilteredTags;
  QString mOldTagname;
};

#endif
