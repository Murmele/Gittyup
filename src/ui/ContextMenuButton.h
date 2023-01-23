//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CONTEXTMENUBUTTON_H
#define CONTEXTMENUBUTTON_H

#include <QToolButton>

class ContextMenuButton : public QToolButton {
public:
  ContextMenuButton(QWidget *parent = nullptr);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
};

#endif
