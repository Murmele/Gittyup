//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef FOOTER_H
#define FOOTER_H

#include <QWidget>

class QMenu;
class QPaintEvent;
class QToolButton;

class Footer : public QWidget {
  Q_OBJECT

public:
  Footer(QWidget *parent = nullptr);

  void setPlusMenu(QMenu *menu);
  void setContextMenu(QMenu *menu);

  void setPlusEnabled(bool enabled);
  void setMinusEnabled(bool enabled);

signals:
  void plusClicked();
  void minusClicked();

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QToolButton *mPlus;
  QToolButton *mMinus;
  QToolButton *mGear;
};

#endif
