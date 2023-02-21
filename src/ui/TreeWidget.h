//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include "DetailView.h"

#include <QModelIndex>

class BlameEditor;
class ColumnView;
class TreeModel;
class QLineEdit;
class QListWidget;
class QLabel;
class QListWidgetItem;

namespace git {
class Diff;
class Repository;
} // namespace git

class TreeWidget : public ContentWidget {
  Q_OBJECT

public:
  TreeWidget(const git::Repository &repo, QWidget *parent = nullptr);

  QString selectedFile() const override;
  virtual QModelIndex selectedIndex() const override;

  void setDiff(const git::Diff &diff, const QString &file = QString(),
               const QString &pathspec = QString()) override;

  void cancelBackgroundTasks() override;

  void find() override;
  void findNext() override;
  void findPrevious() override;

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
  void search(const QString &pattern);
  void setFile(const QListWidgetItem *);

private:
  void edit(const QModelIndex &index);
  void searchFiles(const QRegularExpression &re,
                   const QModelIndex &parent = QModelIndex());
  void loadEditorContent(const QModelIndex &index);

  void selectFile(const QString &name);

  QLabel *mLabelSearch;
  QLineEdit *mSearch;
  QListWidget *mSearchResults;
  ColumnView *mView;
  TreeModel *mModel;
  BlameEditor *mEditor;

  bool mSuppressIndexChange{false};
};
#endif
