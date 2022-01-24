//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TreeWidget.h"
#include "BlameEditor.h"
#include "ColumnView.h"
#include "FileContextMenu.h"
#include "RepoView.h"
#include "ToolBar.h"
#include "TreeModel.h"
#include "git/Blob.h"
#include "git/Commit.h"
#include "git/Diff.h"
#include "git/Index.h"
#include "git/Reference.h"
#include "git/Submodule.h"
#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>

namespace {

const QString kSplitterKey = QString("treesplitter");

const QItemSelectionModel::SelectionFlags kSelectionFlags =
  QItemSelectionModel::Clear |
  QItemSelectionModel::Current |
  QItemSelectionModel::Select;

} // anon. namespace

TreeWidget::TreeWidget(const git::Repository &repo, QWidget *parent)
  : ContentWidget(parent)
{
  mView = new ColumnView(this);
  mView->setModel(new TreeModel(repo, this));
  connect(mView, &ColumnView::fileSelected,
          this, &TreeWidget::loadEditorContent);

  // Open a new editor window on double-click.
  connect(mView, &ColumnView::doubleClicked, this, &TreeWidget::edit);

  mEditor = new BlameEditor(repo, this);

  QSplitter *splitter = new QSplitter(Qt::Vertical, this);
  splitter->setHandleWidth(0);
  splitter->addWidget(mView);
  splitter->addWidget(mEditor);
  splitter->setStretchFactor(1, 1);
  connect(splitter, &QSplitter::splitterMoved, this, [splitter] {
    QSettings().setValue(kSplitterKey, splitter->saveState());
  });

  // Restore splitter state.
  splitter->restoreState(QSettings().value(kSplitterKey).toByteArray());

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(splitter);

  RepoView *view = RepoView::parentView(this);
  connect(mView, &ColumnView::linkActivated, view, &RepoView::visitLink);
  connect(mEditor, &BlameEditor::linkActivated, view, &RepoView::visitLink);
}

QString TreeWidget::selectedFile() const
{
  QModelIndexList indexes = mView->selectionModel()->selectedIndexes();
  return indexes.isEmpty() ? QString() :
         indexes.first().data(Qt::EditRole).toString();
}

QModelIndex TreeWidget::selectedIndex() const {
    QModelIndexList indexes = mView->selectionModel()->selectedIndexes();
    return indexes.isEmpty() ? QModelIndex(): indexes.first();
}

void TreeWidget::setDiff(
  const git::Diff &diff,
  const QString &file,
  const QString &pathspec)
{
  // Remember selection.
  QString name = file;
  if (name.isEmpty()) {
    QModelIndexList indexes = mView->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty())
      name = indexes.first().data(Qt::EditRole).toString();
  }

  // Reset model.
  git::Tree tree = RepoView::parentView(this)->tree();
  TreeModel *model = static_cast<TreeModel *>(mView->model());
  model->setTree(tree, diff);

  // Clear editor.
  mEditor->clear();

  // Restore selection.
  selectFile(name);

  // Show the tree view.
  mView->setVisible(tree.isValid());
}

void TreeWidget::find()
{
  mEditor->find();
}

void TreeWidget::findNext()
{
  mEditor->findNext();
}

void TreeWidget::findPrevious()
{
  mEditor->findPrevious();
}

void TreeWidget::contextMenuEvent(QContextMenuEvent *event)
{
  QStringList files;
  QModelIndexList indexes = mView->selectionModel()->selectedIndexes();
  foreach (const QModelIndex &index, indexes)
    files.append(index.data(Qt::EditRole).toString());

  if (files.isEmpty())
    return;

  RepoView *view = RepoView::parentView(this);
  FileContextMenu menu(view, files);
  menu.exec(event->globalPos());
}

void TreeWidget::cancelBackgroundTasks()
{
  mEditor->cancelBlame();
}

void TreeWidget::edit(const QModelIndex &index)
{
  if (!index.isValid() || index.model()->hasChildren(index))
    return;

  RepoView::parentView(this)->edit(index.data(Qt::EditRole).toString());
}

void TreeWidget::loadEditorContent(const QModelIndex &index)
{
  QString name = index.data(Qt::EditRole).toString();
  git::Blob blob = index.data(TreeModel::BlobRole).value<git::Blob>();

  QList<git::Commit> commits = RepoView::parentView(this)->commits();
  git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();
  mEditor->load(name, blob, commit);
}

void TreeWidget::selectFile(const QString &file)
{
  if (file.isEmpty())
    return;

  QModelIndex index;
  QStringList path = file.split("/");
  QAbstractItemModel *model = mView->model();
  while (!path.isEmpty()) {
    QString elem = path.takeFirst();
    for (int row = 0; row < model->rowCount(index); ++row) {
      QModelIndex current = model->index(row, 0, index);
      if (model->data(current, Qt::DisplayRole).toString() == elem) {
        mView->selectionModel()->setCurrentIndex(current, kSelectionFlags);
        index = current;
        break;
      }
    }
  }

  if (index.isValid())
    loadEditorContent(index);

  // FIXME: Selection does not draw correctly in the last column.
  // Scrolling down to an invisible index is also broken.
}
