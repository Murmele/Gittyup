//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef ICONLABEL_H
#define ICONLABEL_H

#include <QIcon>
#include <QWidget>

class IconLabel : public QWidget {
public:
  IconLabel(const QIcon &icon, int width, int height,
            QWidget *parent = nullptr);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QIcon mIcon;
  int mWidth;
  int mHeight;
};

#endif
