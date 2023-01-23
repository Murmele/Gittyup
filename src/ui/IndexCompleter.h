//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef INDEXCOMPLETER_H
#define INDEXCOMPLETER_H

#include <QCompleter>

class MainWindow;
class QLineEdit;

class IndexCompleter : public QCompleter {
public:
  IndexCompleter(MainWindow *window, QLineEdit *parent);
  IndexCompleter(QAbstractItemModel *model, QLineEdit *parent);

  bool eventFilter(QObject *watched, QEvent *event) override;
  QString pathFromIndex(const QModelIndex &index) const override;
  QStringList splitPath(const QString &path) const override;

private:
  mutable int mPos = -1;
};

#endif
