//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef PROGRESSINDICATOR
#define PROGRESSINDICATOR

#include <QWidget>

// FIXME: Implement fully fledged widget?
class ProgressIndicator : public QWidget {
public:
  static QSize size();

  static void paint(QPainter *painter, const QRect &rect, const QColor &color,
                    int progress, const QWidget *widget = nullptr);
};

#endif
