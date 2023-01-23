//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QTreeView>

class LogEntry;

class LogView : public QTreeView {
  Q_OBJECT

public:
  LogView(LogEntry *root, QWidget *parent = nullptr);

  QSize minimumSizeHint() const override;

  void copy();
  void setCollapseEnabled(bool collapse);

  void setEntryExpanded(LogEntry *entry, bool expanded);

signals:
  void linkActivated(const QString &link);
  void operationCanceled(const QModelIndex &index);

protected:
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  QString linkAt(const QModelIndex &index, const QPoint &pos);
  bool isDecoration(const QModelIndex &index, const QPoint &pos);

  QString mLink;
  QModelIndex mCancel;
  bool mCollapse = true;
};

#endif
