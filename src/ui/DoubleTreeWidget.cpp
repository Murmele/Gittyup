//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "DoubleTreeWidget.h"
#include "BlameEditor.h"
#include "DiffTreeModel.h"
#include "TreeProxy.h"
#include "TreeView.h"
#include "ViewDelegate.h"
#include "StatePushButton.h"
#include "DiffView/DiffView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QButtonGroup>

namespace {

const QString kNameFmt = "<p style='font-size: large'>%1</p>";
const QString kLabelFmt = "<p style='color: gray; font-weight: bold'>%1</p>";
QString kExpandAll = QString(QObject::tr("Expand all"));
QString kCollapseAll = QString(QObject::tr("Collapse all"));
QString kUnstagedFiles = QString(QObject::tr("Unstaged Files"));

class SegmentedButton : public QWidget
{
public:
  SegmentedButton(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    mLayout = new QHBoxLayout(this);
    mLayout->setContentsMargins(0,0,0,0);
    mLayout->setSpacing(0);
  }

  void addButton(
    QAbstractButton *button,
    const QString &text = QString(),
    bool checkable = false)
  {
    button->setToolTip(text);
    button->setCheckable(checkable);

    mLayout->addWidget(button);
    mButtons.addButton(button, mButtons.buttons().size());

    if (mButtons.buttons().size() > 1) {
      mButtons.buttons().first()->setObjectName("first");
      mButtons.buttons().last()->setObjectName("last");
    }

    for (int i = 1; i < mButtons.buttons().size() - 1; ++i)
      mButtons.buttons().at(i)->setObjectName("middle");
  }

  const QButtonGroup *buttonGroup() const
  {
    return &mButtons;
  }

private:
  QHBoxLayout *mLayout;
  QButtonGroup mButtons;
};

} // anon. namespace

DoubleTreeWidget::DoubleTreeWidget(const git::Repository &repo, QWidget *parent)
  : ContentWidget(parent)
{
  // first column
  // top (Buttons to switch between Blame editor and DiffView)
  SegmentedButton* segmentedButton = new SegmentedButton(this);
  QPushButton* blameView = new QPushButton("Blame", this);
  segmentedButton->addButton(blameView, "Blame", true);
  QPushButton* diffView = new QPushButton("Diff", this);
  segmentedButton->addButton(diffView, "Diff", true);
  diffView->setChecked(true);
  // bottom (Stacked widget with Blame editor and DiffView)
  QVBoxLayout* fileViewLayout = new QVBoxLayout();
  mFileView = new QStackedWidget(this);
  mEditor = new BlameEditor(repo, this);
  mDiffView = new DiffView(repo, this);
  int index = mFileView->addWidget(mEditor);
  assert(index == DoubleTreeWidget::Blame);
  index = mFileView->addWidget(mDiffView);
  assert(index == DoubleTreeWidget::Diff);
  mEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  mDiffView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  const QButtonGroup* viewGroup = segmentedButton->buttonGroup();
  QHBoxLayout* buttonLayout = new QHBoxLayout();
  buttonLayout->addItem(new QSpacerItem(279, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
  buttonLayout->addWidget(segmentedButton);
  buttonLayout->addItem(new QSpacerItem(279, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

  fileViewLayout->addLayout(buttonLayout);
  fileViewLayout->addWidget(mFileView);
  mFileView->setCurrentIndex(1);
  mFileView->show();
  QWidget* fileView = new QWidget(this);
  fileView->setLayout(fileViewLayout);

  // second column
  // staged files
  QVBoxLayout* vBoxLayout = new QVBoxLayout();
  QLabel* label = new QLabel(tr("Staged Files"));
  stagedFiles = new TreeView(this, "Staged");
  stagedFiles->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  mDiffTreeModel = new DiffTreeModel(repo, this);
  mDiffView->setModel(mDiffTreeModel);
  TreeProxy* treewrapperStaged = new TreeProxy(true, this);
  treewrapperStaged->setSourceModel(mDiffTreeModel);
  stagedFiles->setModel(treewrapperStaged);
  stagedFiles->setHeaderHidden(true);
  stagedFiles->setItemDelegateForColumn(0, new ViewDelegate());

  QHBoxLayout* hBoxLayout = new QHBoxLayout();
  collapseButtonStagedFiles = new StatePushButton(kCollapseAll, kExpandAll, this);
  hBoxLayout->addWidget(collapseButtonStagedFiles);
  hBoxLayout->addItem(new QSpacerItem(40,20, QSizePolicy::Expanding, QSizePolicy::Minimum));

  vBoxLayout->addWidget(label);
  vBoxLayout->addLayout(hBoxLayout);
  vBoxLayout->addWidget(stagedFiles);
  mStagedWidget = new QWidget();
  mStagedWidget->setLayout(vBoxLayout);

  // unstaged files
  vBoxLayout = new QVBoxLayout();
  mUnstagedCommitedFiles = new QLabel(kUnstagedFiles);
  unstagedFiles = new TreeView(this, "Unstaged");
  unstagedFiles->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  TreeProxy* treewrapperUnstaged = new TreeProxy(false, this);
  treewrapperUnstaged->setSourceModel(mDiffTreeModel);
  unstagedFiles->setModel(treewrapperUnstaged);
  unstagedFiles->setHeaderHidden(true);
  unstagedFiles->setItemDelegateForColumn(0, new ViewDelegate());

  hBoxLayout = new QHBoxLayout();
  collapseButtonUnstagedFiles = new StatePushButton(kCollapseAll, kExpandAll, this);
  hBoxLayout->addWidget(collapseButtonUnstagedFiles);
  hBoxLayout->addItem(new QSpacerItem(40,20, QSizePolicy::Expanding, QSizePolicy::Minimum));

  vBoxLayout->addWidget(mUnstagedCommitedFiles);
  vBoxLayout->addLayout(hBoxLayout);
  vBoxLayout->addWidget(unstagedFiles);
  QWidget* unstagedWidget = new QWidget();
  unstagedWidget->setLayout(vBoxLayout);

  // splitter between the staged and unstaged section
  QSplitter *treeViewSplitter = new QSplitter(Qt::Vertical, this);
  treeViewSplitter->setHandleWidth(10);
  treeViewSplitter->addWidget(mStagedWidget);
  treeViewSplitter->addWidget(unstagedWidget);
  treeViewSplitter->setStretchFactor(1, 1);

  // splitter between editor/diffview and TreeViews
  QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setHandleWidth(0);
  splitter->addWidget(fileView);
  splitter->addWidget(treeViewSplitter);
  splitter->setStretchFactor(1, 1);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(splitter);

  setLayout(layout);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  connect(viewGroup, QOverload<int>::of(&QButtonGroup::idClicked), [this] (int id) {
    mFileView->setCurrentIndex(id);
  });
#else
  connect(viewGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
          [this, viewGroup](QAbstractButton *button)
  {
    mFileView->setCurrentIndex(viewGroup->id(button));
  });
#endif

  connect(mDiffTreeModel, &DiffTreeModel::checkStateChanged, this, &DoubleTreeWidget::treeModelStateChanged);
  connect(mDiffView, &DiffView::fileStageStateChanged, this, &DoubleTreeWidget::updateTreeModel);

  connect(stagedFiles, &TreeView::fileSelected, this, &DoubleTreeWidget::fileSelected);
  connect(stagedFiles, &TreeView::collapseCountChanged, this, &DoubleTreeWidget::collapseCountChanged);

  connect(unstagedFiles, &TreeView::fileSelected, this, &DoubleTreeWidget::fileSelected);
  connect(unstagedFiles, &TreeView::collapseCountChanged, this, &DoubleTreeWidget::collapseCountChanged);

  connect(collapseButtonStagedFiles, &StatePushButton::clicked, this, &DoubleTreeWidget::toggleCollapseStagedFiles);
  connect(collapseButtonUnstagedFiles, &StatePushButton::clicked, this, &DoubleTreeWidget::toggleCollapseUnstagedFiles);
}

QModelIndex DoubleTreeWidget::selectedIndex() const
{
  TreeProxy* proxy = static_cast<TreeProxy *>(stagedFiles->model());
  QModelIndexList indexes = stagedFiles->selectionModel()->selectedIndexes();
  if (!indexes.isEmpty()) {
    return proxy->mapToSource(indexes.first());
  }

  indexes = unstagedFiles->selectionModel()->selectedIndexes();
  proxy = static_cast<TreeProxy *>(unstagedFiles->model());
  if (!indexes.isEmpty()) {
    return  proxy->mapToSource(indexes.first());
  }
  return QModelIndex();
}

QString DoubleTreeWidget::selectedFile() const
{
  QModelIndexList indexes = stagedFiles->selectionModel()->selectedIndexes();
  if (!indexes.isEmpty()) {
    return indexes.first().data(Qt::DisplayRole).toString();
  }

  indexes = unstagedFiles->selectionModel()->selectedIndexes();
  if (!indexes.isEmpty()) {
    return indexes.first().data(Qt::DisplayRole).toString();
  }
  return "";
}

/*!
 * \brief DoubleTreeWidget::setDiff
 * \param diff
 * \param file
 * \param pathspec
 */
void DoubleTreeWidget::setDiff(const git::Diff &diff,
                               const QString &file,
                               const QString &pathspec)
{
  Q_UNUSED(pathspec);
  // Remember selection.
  storeSelection();

  // Reset model.
  // because of this, the content in the view is shown.
  TreeProxy* proxy = static_cast<TreeProxy *>(unstagedFiles->model());
  DiffTreeModel* model = static_cast<DiffTreeModel*>(proxy->sourceModel());
  model->setDiff(diff);
  // do not expand if to many files exist, it takes really long
  // So do it only when there are less than 100
  if (diff.isValid() && diff.count() < 100)
    unstagedFiles->expandAll();
  else
    unstagedFiles->collapseAll();

  // If statusDiff, there exist no staged/unstaged, but only
  // the commited files must be shown
  if (!diff.isValid() || diff.isStatusDiff()) {
    mUnstagedCommitedFiles->setText(kUnstagedFiles);
    if (diff.isValid() && diff.count() < 100)
      stagedFiles->expandAll();
    else
      stagedFiles->collapseAll();

    mStagedWidget->setVisible(true);
  } else {
    mUnstagedCommitedFiles->setText(tr("Committed Files"));
    mStagedWidget->setVisible(false);
  }

  // Clear editor.
  mEditor->clear();

  mDiffView->setDiff(diff);

  // Restore selection.
  loadSelection();
}

void DoubleTreeWidget::updateTreeModel(git::Index::StagedState state)
{
//  // the selected index must be the file which is visible in the diffView!
//  QModelIndexList indexes = stagedFiles->selectionModel()->selectedIndexes();
//  if (!indexes.isEmpty()) {
//    static_cast<TreeProxy*>(stagedFiles->model())->setData(indexes.first(), state, Qt::CheckStateRole, true);
//    return;
//  }

//  indexes = unstagedFiles->selectionModel()->selectedIndexes();
//  if (!indexes.isEmpty()) {
//    static_cast<TreeProxy*>(unstagedFiles->model())->setData(indexes.first(), state, Qt::CheckStateRole, true);
//    return;
//  }
}

void DoubleTreeWidget::storeSelection()
{
  QModelIndexList indexes = stagedFiles->selectionModel()->selectedIndexes();
  if (!indexes.isEmpty()) {
    mSelectedFile.filename = indexes.first().data(Qt::EditRole).toString();
    mSelectedFile.stagedModel = true;
    return;
  }

  indexes = unstagedFiles->selectionModel()->selectedIndexes();
  if (!indexes.isEmpty()) {
    mSelectedFile.filename = indexes.first().data(Qt::EditRole).toString();
    mSelectedFile.stagedModel = false;
    return;
  }
  mSelectedFile.filename = "";
}

void DoubleTreeWidget::loadSelection()
{
  if (mSelectedFile.filename == "")
    return;

  QModelIndex index = mDiffTreeModel->index(mSelectedFile.filename);
  if (!index.isValid())
    return;

  mIgnoreSelectionChange = true;
  if (mSelectedFile.stagedModel) {
    TreeProxy* proxy = static_cast<TreeProxy *>(stagedFiles->model());
    index = proxy->mapFromSource(index);
    stagedFiles->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
  } else {
    TreeProxy* proxy = static_cast<TreeProxy *>(unstagedFiles->model());
    index = proxy->mapFromSource(index);
    unstagedFiles->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
  }
  mIgnoreSelectionChange = false;
}

void DoubleTreeWidget::treeModelStateChanged(const QModelIndex& index, int checkState)
{
  Q_UNUSED(index);
  Q_UNUSED(checkState);
  // clear editor and disable diffView when no item is selected
  QModelIndexList stagedSelections = stagedFiles->selectionModel()->selectedIndexes();
  if (stagedSelections.count())
    return;

  QModelIndexList unstagedSelections = unstagedFiles->selectionModel()->selectedIndexes();
  if (unstagedSelections.count())
    return;

  mDiffView->enable(false);
  mEditor->clear();
}

void DoubleTreeWidget::collapseCountChanged(int count)
{
  TreeView* view = static_cast<TreeView*>(QObject::sender());

  if (view == stagedFiles)
    collapseButtonStagedFiles->setState(count == 0);
  else
    collapseButtonUnstagedFiles->setState(count == 0);
}

void DoubleTreeWidget::fileSelected(const QModelIndex &index)
{
  if (!index.isValid())
    return;

  QObject* obj = QObject::sender();
  if (obj) {
    TreeView* treeview = static_cast<TreeView*>(obj);
    if (treeview == stagedFiles) {
      unstagedFiles->deselectAll();
      stagedFiles->setFocus();
    } else if (treeview == unstagedFiles) {
      stagedFiles->deselectAll();
      unstagedFiles->setFocus();
    }
  }
  loadEditorContent(index);
}

void DoubleTreeWidget::loadEditorContent(const QModelIndex &index)
{
  QString name = index.data(Qt::EditRole).toString();
  QList<git::Commit> commits = RepoView::parentView(this)->commits();
  git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();
  git::Tree tree = commit.tree();

  // searching for the correct blob
  auto list = name.split("/");
  bool found = false;
  git::Blob blob;
  for (int path_depth = 0; path_depth < list.count(); path_depth++) {
    auto element = list[path_depth];
    found = false;
    for (int i = 0; i < tree.count(); ++i) {
      auto n = tree.name(i);
      if (n == element) {
        if (path_depth >= list.count() -1)
          blob = tree.object(i);
        else
          tree = tree.object(i);
        found = true;
        break;
      }
    }
    if (!found)
      break;
  }

  if (found)
    mEditor->load(name, blob, commit);

  mDiffView->enable(true);
  mDiffView->updateFiles();
}

void DoubleTreeWidget::toggleCollapseStagedFiles()
{
  if (collapseButtonStagedFiles->toggleState())
    stagedFiles->expandAll();
  else
    stagedFiles->collapseAll();
}

void DoubleTreeWidget::toggleCollapseUnstagedFiles()
{
  if (collapseButtonUnstagedFiles->toggleState())
    unstagedFiles->expandAll();
  else
    unstagedFiles->collapseAll();
}
