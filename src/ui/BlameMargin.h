//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef BLAMEMARGIN_H
#define BLAMEMARGIN_H

#include "git/Id.h"
#include "git/Blame.h"
#include <QTimer>
#include <QWidget>

class TextEditor;

namespace git {
class Repository;
}

class BlameMargin : public QWidget {
  Q_OBJECT

public:
  BlameMargin(TextEditor *editor, QWidget *parent = nullptr);

  void startBlame(const QString &name);
  void setBlame(const git::Repository &repo, const git::Blame &blame);
  void clear();

  QSize minimumSizeHint() const override;

signals:
  void linkActivated(const QString &link);

protected:
  bool event(QEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  void updateBlame();

  int index(int y) const;
  QString name(int index) const;

  QString mName;
  TextEditor *mEditor;

  git::Blame mBlame;
  git::Blame mSource;

  int mIndex;
  git::Id mSelection;

  int mProgress;
  QTimer mTimer;

  int mMinTime = -1;
  int mMaxTime = -1;
};

#endif
