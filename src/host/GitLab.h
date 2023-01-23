//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef GITLAB_H
#define GITLAB_H

#include "Account.h"

class GitLab : public Account {
  Q_OBJECT

public:
  GitLab(const QString &username);

  Kind kind() const override;
  QString name() const override;
  QString host() const override;
  void connect(const QString &password = QString()) override;

  static QString defaultUrl();
};

#endif
