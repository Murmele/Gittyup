//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>

class TabWidget : public QTabWidget {
  Q_OBJECT

public:
  TabWidget(QWidget *parent = nullptr);

signals:
  void tabAboutToBeInserted();
  void tabAboutToBeRemoved();
  void tabInserted();
  void tabRemoved();

protected:
  void resizeEvent(QResizeEvent *event) override;
  void tabInserted(int index) override;
  void tabRemoved(int index) override;

private:
  QWidget *mDefaultWidget;
};

#endif
