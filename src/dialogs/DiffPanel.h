//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DIFFPANEL_H
#define DIFFPANEL_H

#include "git/Config.h"
#include <QWidget>

class QHBoxLayout;

class DiffPanel : public QWidget {
  Q_OBJECT

public:
  DiffPanel(const git::Repository &repo, QWidget *parent = nullptr);

private:
  git::Config mConfig;
};

#endif
