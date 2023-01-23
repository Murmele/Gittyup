//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef COMMITTOOLBAR_H
#define COMMITTOOLBAR_H

#include <QToolBar>

class CommitToolBar : public QToolBar {
  Q_OBJECT

public:
  CommitToolBar(QWidget *parent = nullptr);

signals:
  void settingsChanged();
};

#endif
