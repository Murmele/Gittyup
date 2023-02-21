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
#include <QLineEdit>
#include <QListWidget>

namespace {

const QString kSplitterKey = QString("treesplitter");

const QItemSelectionModel::SelectionFlags kSelectionFlags =
    QItemSelectionModel::Clear | QItemSelectionModel::Current |
    QItemSelectionModel::Select;

} // namespace

TreeWidget::TreeWidget(const git::Repository &repo, QWidget *parent)
    : ContentWidget(parent) {
  mView = new ColumnView(this);
  mModel = new TreeModel(repo, this);
  mView->setModel(mModel);
  connect(mView, &ColumnView::fileSelected, this,
          &TreeWidget::loadEditorContent);

  // Open a new editor window on double-click.
  connect(mView, &ColumnView::doubleClicked, this, &TreeWidget::edit);

  mLabelSearch = new QLabel(tr("Search:"), this);
  mSearch = new QLineEdit(this);
  connect(mSearch, &QLineEdit::textChanged, this, &TreeWidget::search);
  QHBoxLayout *l = new QHBoxLayout();
  l->addWidget(mLabelSearch);
  l->addWidget(mSearch);

  mSearchResults = new QListWidget(this);
  connect(mSearchResults, &QListWidget::currentItemChanged, this,
          &TreeWidget::setFile);

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
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(l);
  layout->addWidget(mSearchResults);
  layout->addWidget(splitter);

  RepoView *view = RepoView::parentView(this);
  connect(mView, &ColumnView::linkActivated, view, &RepoView::visitLink);
  connect(mEditor, &BlameEditor::linkActivated, view, &RepoView::visitLink);
}

QString TreeWidget::selectedFile() const {
  QModelIndexList indexes = mView->selectionModel()->selectedIndexes();
  return indexes.isEmpty() ? QString()
                           : indexes.first().data(Qt::EditRole).toString();
}

QModelIndex TreeWidget::selectedIndex() const {
  QModelIndexList indexes = mView->selectionModel()->selectedIndexes();
  return indexes.isEmpty() ? QModelIndex() : indexes.first();
}

void TreeWidget::setDiff(const git::Diff &diff, const QString &file,
                         const QString &pathspec) {
  // Remember selection.
  QString name = file;
  if (name.isEmpty()) {
    QModelIndexList indexes = mView->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty())
      name = indexes.first().data(Qt::EditRole).toString();
  }

  // Reset model.
  git::Tree tree = RepoView::parentView(this)->tree();
  mModel->setTree(tree, diff);

  // Clear editor.
  mEditor->clear();
  mSearch->clear();
  mSuppressIndexChange = true;
  mSearchResults->clear();
  mSuppressIndexChange = false;

  // Restore selection.
  selectFile(name);

  // Show the tree view.
  const bool search = !mSearch->text().isEmpty();
  mView->setVisible(tree.isValid() && !search);
  mSearchResults->setVisible(tree.isValid() && search);
}

void TreeWidget::find() { mEditor->find(); }

void TreeWidget::findNext() { mEditor->findNext(); }

void TreeWidget::findPrevious() { mEditor->findPrevious(); }

void TreeWidget::contextMenuEvent(QContextMenuEvent *event) {
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

void TreeWidget::cancelBackgroundTasks() { mEditor->cancelBlame(); }

void TreeWidget::edit(const QModelIndex &index) {
  if (!index.isValid() || index.model()->hasChildren(index))
    return;

  RepoView::parentView(this)->edit(index.data(Qt::EditRole).toString());
}

void TreeWidget::search(const QString &pattern) {
  const bool search = !pattern.isEmpty();
  mView->setVisible(!search);
  mSearchResults->setVisible(search);

  if (!search)
    return;

  QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
  mSuppressIndexChange = true;
  mSearchResults->clear();
  searchFiles(re);
  mSuppressIndexChange = false;
}

void TreeWidget::searchFiles(const QRegularExpression &re,
                             const QModelIndex &parent) {
  for (int row = 0; row < mModel->rowCount(parent); row++) {
    const auto index = mModel->index(row, 0, QModelIndex(parent));
    if (mModel->rowCount(index) > 0) {
      // folder
      searchFiles(re, index);
    } else {
      // file
      const QString name = mModel->data(index, Qt::EditRole).toString();
      if (re.match(name).hasMatch()) {
        QListWidgetItem *item = new QListWidgetItem(name, mSearchResults);
        item->setData(Qt::UserRole, index);
        mSearchResults->addItem(item);
      }
    }
  }
}

void TreeWidget::setFile(const QListWidgetItem *item) {
  if (mSuppressIndexChange)
    return;
  const auto index = item->data(Qt::UserRole).value<QModelIndex>();
  loadEditorContent(index);
}

void TreeWidget::loadEditorContent(const QModelIndex &index) {
  QString name = index.data(Qt::EditRole).toString();
  git::Blob blob = index.data(TreeModel::BlobRole).value<git::Blob>();

  QList<git::Commit> commits = RepoView::parentView(this)->commits();
  git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();
  mEditor->load(name, blob, commit);
}

void TreeWidget::selectFile(const QString &file) {
  if (file.isEmpty())
    return;

  QModelIndex index;
  QStringList path = file.split("/");
  QAbstractItemModel *model = mModel;
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
