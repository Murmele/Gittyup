//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REMOTEDIALOG_H
#define REMOTEDIALOG_H

#include <QDialog>
#include "git/Repository.h"

class ReferenceList;
class RepoView;
class QCheckBox;
class QComboBox;
class QLineEdit;

namespace git {
class Reference;
class Remote;
} // namespace git

class RemoteDialog : public QDialog {
  Q_OBJECT

public:
  enum Kind { Fetch, Pull, Push };

  RemoteDialog(Kind kind, RepoView *parent);

private:
  QComboBox *mRemotes;
  ReferenceList *mRefs = nullptr;
  QComboBox *mAction = nullptr;
  QCheckBox *mTags = nullptr;
  QCheckBox *mSetUpstream = nullptr;
  QCheckBox *mForce = nullptr;
  QLineEdit *mRemoteRef = nullptr;
};

#endif
