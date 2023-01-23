//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef COLUMNVIEW_H
#define COLUMNVIEW_H

#include <QColumnView>

class QItemDelegate;

class ColumnView : public QColumnView {
  Q_OBJECT

public:
  ColumnView(QWidget *parent = nullptr);

  void setModel(QAbstractItemModel *model) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

signals:
  void linkActivated(const QString &link);
  void fileSelected(const QModelIndex &index);

protected:
  QAbstractItemView *createColumn(const QModelIndex &index) override;

private:
  void handleSelectionChange(const QItemSelection &selected,
                             const QItemSelection &deselected);

  QItemDelegate *mSharedDelegate;
};

#endif
