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
#include "TreeModel.h"
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
    assert(mFileView->addWidget(mEditor) == DoubleTreeWidget::Blame);
    assert(mFileView->addWidget(mDiffView) == DoubleTreeWidget::Diff);

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
	stagedFiles = new TreeView(this);
    mTreeModel = new TreeModel(repo, this);
	TreeProxy* treewrapperStaged = new TreeProxy(true, this);
    treewrapperStaged->setSourceModel(mTreeModel);
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
	unstagedFiles = new TreeView(this);
	TreeProxy* treewrapperUnstaged = new TreeProxy(false, this);
    treewrapperUnstaged->setSourceModel(mTreeModel);
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

    connect(viewGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), [this] (int idx) {
        mFileView->setCurrentIndex(idx);
    });

    connect(mTreeModel, &TreeModel::checkStateChanged, this, &DoubleTreeWidget::treeModelStateChanged);

    connect(mDiffView, &DiffView::fileStageStateChanged, this, &DoubleTreeWidget::updateTreeModel);

    connect(stagedFiles, &TreeView::fileSelected, this, &DoubleTreeWidget::fileSelected);
    connect(stagedFiles, &TreeView::collapseCountChanged, this, &DoubleTreeWidget::collapseCountChanged);


    connect(unstagedFiles, &TreeView::fileSelected, this, &DoubleTreeWidget::fileSelected);
    connect(unstagedFiles, &TreeView::collapseCountChanged, this, &DoubleTreeWidget::collapseCountChanged);

	connect(collapseButtonStagedFiles, &StatePushButton::clicked, this, &DoubleTreeWidget::toggleCollapseStagedFiles);
	connect(collapseButtonUnstagedFiles, &StatePushButton::clicked, this, &DoubleTreeWidget::toggleCollapseUnstagedFiles);
}

QString DoubleTreeWidget::selectedFile() const {

}

/*!
 * \brief DoubleTreeWidget::setDiff
 * \param diff
 * \param file
 * \param pathspec
 */
void DoubleTreeWidget::setDiff(const git::Diff &diff,
									const QString &file,
									const QString &pathspec) {
	// Remember selection.
	QString name = file;
	if (name.isEmpty()) {
	  QModelIndexList indexes = stagedFiles->selectionModel()->selectedIndexes();
	  if (!indexes.isEmpty())
		name = indexes.first().data(Qt::EditRole).toString();
	}

    // Reset model.
    git::Tree tree = RepoView::parentView(this)->tree();
    // because of this, the content in the view is shown.
    TreeProxy* proxy = static_cast<TreeProxy *>(unstagedFiles->model());
    TreeModel* model = static_cast<TreeModel*>(proxy->sourceModel());
    model->setTree(tree, diff);
    unstagedFiles->expandAll();

    if (!diff.isValid() || diff.isStatusDiff()) {
        mUnstagedCommitedFiles->setText(kUnstagedFiles);
        stagedFiles->expandAll();
        mStagedWidget->setVisible(true);

    } else {
        mUnstagedCommitedFiles->setText(tr("Committed Files"));
        mStagedWidget->setVisible(false);
    }

	// Clear editor.
	mEditor->clear();

    mDiffView->setDiff(diff);

	// Restore selection.
	//selectFile(name);
}

void DoubleTreeWidget::updateTreeModel(git::Index::StagedState state)
{
    // the selected index must be the file which is visible in the diffView!
    QModelIndexList indexes = stagedFiles->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
        static_cast<TreeProxy*>(stagedFiles->model())->setData(indexes.first(), state, Qt::CheckStateRole, true);
      return;
    }

    indexes = unstagedFiles->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
      static_cast<TreeProxy*>(unstagedFiles->model())->setData(indexes.first(), state, Qt::CheckStateRole, true);
      return;
    }
}

void DoubleTreeWidget::treeModelStateChanged(const QModelIndex& index, int checkState) {
    // clear editor and disable diffView when no item is selected
    Qt::CheckState cs = static_cast<Qt::CheckState>(checkState);
    QModelIndexList stagedSelections = stagedFiles->selectionModel()->selectedIndexes();
    if (stagedSelections.count())
        return;

    QModelIndexList unstagedSelections = unstagedFiles->selectionModel()->selectedIndexes();
    if (unstagedSelections.count())
        return;

    mDiffView->enable(false);
    mEditor->clear();


}

void DoubleTreeWidget::selectFile(const QString &file)
{
//  if (file.isEmpty())
//	return;

//  QModelIndex index;
//  QStringList path = file.split("/");
//  QAbstractItemModel *model = mView->model();
//  while (!path.isEmpty()) {
//	QString elem = path.takeFirst();
//	for (int row = 0; row < model->rowCount(index); ++row) {
//	  QModelIndex current = model->index(row, 0, index);
//	  if (model->data(current, Qt::DisplayRole).toString() == elem) {
//		mView->selectionModel()->setCurrentIndex(current, kSelectionFlags);
//		index = current;
//		break;
//	  }
//	}
//  }

//  if (index.isValid())
//	loadEditorContent(index);

//  // FIXME: Selection does not draw correctly in the last column.
//  // Scrolling down to an invisible index is also broken.
}

void DoubleTreeWidget::collapseCountChanged(int count)
{
    TreeView* view = static_cast<TreeView*>(QObject::sender());


    if (view == stagedFiles)
      collapseButtonStagedFiles->setState(count == 0);
    else
      collapseButtonUnstagedFiles->setState(count == 0);
}

void DoubleTreeWidget::fileSelected(const QModelIndex &index) {

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
  git::Blob blob = index.data(TreeModel::BlobRole).value<git::Blob>();

  QList<git::Commit> commits = RepoView::parentView(this)->commits();
  git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();
  mEditor->load(name, blob, commit);
  mDiffView->enable(true);
  mDiffView->setFilter(QStringList(name));
}

void DoubleTreeWidget::toggleCollapseStagedFiles() {

    if (collapseButtonStagedFiles->toggleState())
		stagedFiles->expandAll();
    else
		stagedFiles->collapseAll();
}

void DoubleTreeWidget::toggleCollapseUnstagedFiles() {

    if (collapseButtonUnstagedFiles->toggleState())
        unstagedFiles->expandAll();
    else
		unstagedFiles->collapseAll();
}
