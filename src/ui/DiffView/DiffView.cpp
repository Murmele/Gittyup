//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DiffView.h"
#include "HunkWidget.h"
#include "FileWidget.h"
#include "ui/RepoView.h"
#include "DisclosureButton.h"
#include "CommentWidget.h"
#include "ui/DiffTreeModel.h"
#include "ui/DoubleTreeWidget.h"
#include "ui/HotkeyManager.h"
#include "git/Tree.h"
#include <QScrollBar>
#include <QPushButton>
#include <QMimeData>

namespace {

bool copy(const QString &source, const QDir &targetDir) {
  // Disallow copy into self.
  if (source.startsWith(targetDir.path()))
    return false;

  QFileInfo info(source);
  QString name = info.fileName();
  QString target = targetDir.filePath(name);
  if (!info.isDir())
    return QFile::copy(source, target);

  if (!targetDir.mkdir(name))
    return false;

  foreach (const QFileInfo &entry,
           QDir(source).entryInfoList(DiffViewStyle::kFilters)) {
    if (!copy(entry.filePath(), target))
      return false;
  }

  return true;
}

} // namespace

static Hotkey moveHalfPageDownHotKey = HotkeyManager::registerHotkey(
    "d", "diffView/moveHalfPageDownHotKey", "DiffView/Move Half Page Down");

static Hotkey moveHalfPageUpHotKey = HotkeyManager::registerHotkey(
    "u", "diffView/moveHalfPageUpHotKey", "DiffView/Move Half Page Up");

DiffView::DiffView(const git::Repository &repo, QWidget *parent)
    : QScrollArea(parent), mParent(parent) {
  setStyleSheet(DiffViewStyle::kStyleSheet);
  setAcceptDrops(true);
  setWidgetResizable(true);
  setFocusPolicy(Qt::NoFocus);
  setContextMenuPolicy(Qt::ActionsContextMenu);

  mPlugins = Plugin::plugins(repo);

  // Update comments.
  if (Repository *remote = RepoView::parentView(this)->remoteRepo()) {
    connect(remote->account(), &Account::commentsReady, this,
            [this, remote](Repository *repo, const QString &oid,
                           const Account::CommitComments &comments) {
              if (repo != remote)
                return;

              RepoView *view = RepoView::parentView(this);
              QList<git::Commit> commits = view->commits();
              if (commits.size() != 1 || oid != commits.first().id().toString())
                return;

              mComments = comments;

              // Invalidate editors.
              foreach (QWidget *widget, mFiles) {
                foreach (HunkWidget *hunk,
                         static_cast<FileWidget *>(widget)->hunks())
                  hunk->invalidate();
              }

              // Load commit comments.
              if (!canFetchMore())
                fetchMore();
            });
  }

  QShortcut *shortcut = new QShortcut(this);
  moveHalfPageDownHotKey.use(shortcut);
  connect(shortcut, &QShortcut::activated, [this] { moveHalfPageDown(); });

  shortcut = new QShortcut(this);
  moveHalfPageUpHotKey.use(shortcut);
  connect(shortcut, &QShortcut::activated, [this] { moveHalfPageUp(); });
}

DiffView::~DiffView() {}

QWidget *DiffView::file(int index) {
  fetchAll(index);
  return mFiles.at(index);
}

void DiffView::setDiff(const git::Diff &diff) {
  RepoView *view = RepoView::parentView(this);
  git::Repository repo = view->repo();

  // Disconnect signals.
  foreach (QMetaObject::Connection connection, mConnections)
    disconnect(connection);
  mConnections.clear();

  // Clear state.
  mFiles.clear();
  mComments = Account::CommitComments();

  // Set data.
  mDiff = diff;

  // Create a new widget.
  QWidget *widget = new QWidget(this);
  setWidget(widget);

  // Disable painting the background.
  // This allows drawing content over the border shadow.
  widget->setStyleSheet(".QWidget {background-color: transparent}");

  // Begin layout.
  QVBoxLayout *layout = new QVBoxLayout(widget);
  layout->setSpacing(4);
  layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

  mFileWidgetLayout = new QVBoxLayout();

  if (!diff.isValid()) {
    if (repo.isHeadUnborn()) {
      QPushButton *button =
          new QPushButton(QIcon(":/file.png"), tr("Add new file"));
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      button->setStyleSheet("color: #484848");
      button->setIconSize(QSize(32, 32));
      button->setFlat(true);

      QFont buttonFont = button->font();
      buttonFont.setPointSize(24);
      button->setFont(buttonFont);

      // Open new editor associated with this view.
      connect(button, &QPushButton::clicked, view, &RepoView::newEditor);

      QLabel *label =
          new QLabel(tr("Or drag files here to copy into the repository"));
      label->setStyleSheet("color: #696969");
      label->setAlignment(Qt::AlignHCenter);

      QFont labelFont = label->font();
      labelFont.setPointSize(18);
      label->setFont(labelFont);

      layout->addStretch();
      layout->addWidget(button, 0, Qt::AlignHCenter);
      layout->addWidget(label, 0, Qt::AlignHCenter);
      layout->addStretch();
    }

    layout->addLayout(mFileWidgetLayout);
    layout->addSpacerItem(new QSpacerItem(
        0, 0, QSizePolicy::Expanding,
        QSizePolicy::Expanding)); // so the file is always starting from top and
                                  // is not distributed over the hole diff view
    return;
  }

  layout->addLayout(mFileWidgetLayout);
  layout->addSpacerItem(new QSpacerItem(
      0, 0, QSizePolicy::Expanding,
      QSizePolicy::Expanding)); // so the file is always starting from top and
                                // is not distributed over the hole diff view

  // Generate a diff between the head tree and index.
  loadStagedPatches();

  if (canFetchMore())
    fetchMore();

  // Load patches on demand.
  QScrollBar *scrollBar = verticalScrollBar();
  mConnections.append(
      connect(scrollBar, &QScrollBar::valueChanged, [this](int value) {
        if (value > verticalScrollBar()->maximum() / 2 && canFetchMore())
          fetchMore();
      }));

  mConnections.append(
      connect(scrollBar, &QScrollBar::rangeChanged, [this](int min, int max) {
        if (max - min < this->widget()->height() / 2 && canFetchMore())
          fetchMore();
      }));

  // Request comments for this diff.
  if (Repository *remoteRepo = view->remoteRepo()) {
    QList<git::Commit> commits = view->commits();
    if (commits.size() == 1) {
      QString oid = commits.first().id().toString();
      remoteRepo->account()->requestComments(remoteRepo, oid);
    }
  }

  // connect(repo.notifier(), &git::RepositoryNotifier::indexChanged, this,
  // &DiffView::indexChanged);
}

bool DiffView::scrollToFile(int index) {
  // Ensure that the given index is loaded.
  fetchAll(index);

  // Finish layout by processing events. May cause a new diff to
  // be loaded. In that case the scroll widget will be different.
  QWidget *ptr = widget();
  QCoreApplication::processEvents();
  if (widget() != ptr)
    return false;

  // Scroll to the widget.
  verticalScrollBar()->setValue(mFiles.at(index)->y());
  return true;
}

void DiffView::enable(bool enable) { mEnabled = enable; }

void DiffView::setModel(DiffTreeModel *model) {
  if (mDiffTreeModel)
    disconnect(mDiffTreeModel, nullptr, this, nullptr);

  mDiffTreeModel = model;
  connect(mDiffTreeModel, &DiffTreeModel::dataChanged, this,
          &DiffView::diffTreeModelDataChanged);
}

void DiffView::loadStagedPatches() {
  RepoView *view = RepoView::parentView(this);
  git::Repository repo = view->repo();
  mStagedPatches.clear();
  // Generate a diff between the head tree and index.
  if (mDiff.isStatusDiff()) {
    if (git::Reference head = repo.head()) {
      if (git::Commit commit = head.target()) {
        mStagedDiff = repo.diffTreeToIndex(commit.tree());
        for (int i = 0; i < mStagedDiff.count(); ++i)
          mStagedPatches[mStagedDiff.name(i)] = i;
      }
    }
  }
}

void DiffView::diffTreeModelDataChanged(const QModelIndex &topLeft,
                                        const QModelIndex &bottomRight,
                                        const QVector<int> &roles) {
  assert(topLeft == bottomRight);

  if (!topLeft.isValid())
    return;
  if (roles[0] != Qt::CheckStateRole)
    return;

  QString modelName = topLeft.data(Qt::DisplayRole).toString();

  git::Index::StagedState stageState = static_cast<git::Index::StagedState>(
      topLeft.data(Qt::CheckStateRole).toInt());

  for (auto file : mFiles) {
    if (file->modelIndex().internalPointer() == topLeft.internalPointer()) {

      // Respond to index changes only when file is visible in the diffview
      loadStagedPatches();

      QString filename = file->name();
      git::Patch stagedPatch;
      const int index = mStagedPatches.value(file->name(), -1);
      if (index != -1)
        stagedPatch = mStagedDiff.patch(index);
      file->updateHunks(stagedPatch);
      file->setStageState(stageState);

      return;
    }
  }
}

void DiffView::updateFiles() {
  while (mFiles.count()) {
    auto file = mFiles.takeFirst();
    mFileWidgetLayout->removeWidget(file);
    file->deleteLater();
  }
  mFiles.clear();

  if (canFetchMore())
    fetchMore();
}

QList<TextEditor *> DiffView::editors() {
  fetchAll();
  QList<TextEditor *> editors;
  foreach (QWidget *widget, mFiles) {
    foreach (HunkWidget *hunk, static_cast<FileWidget *>(widget)->hunks())
      editors.append(hunk->editor());
  }

  return editors;
}

void DiffView::ensureVisible(TextEditor *editor, int pos) {
  HunkWidget *hunk = static_cast<HunkWidget *>(editor->parentWidget());
  hunk->header()->button()->setChecked(true);

  FileWidget *file = static_cast<FileWidget *>(hunk->parentWidget());
  file->header()->disclosureButton()->setChecked(true);

  int fileY = hunk->parentWidget()->y();
  int y = fileY + hunk->y() + editor->y() + editor->pointFromPosition(pos).y();

  QScrollBar *scrollBar = verticalScrollBar();
  int val = scrollBar->value();
  int step = scrollBar->pageStep();
  if (y < val) {
    scrollBar->setValue(y);
  } else if (y >= val + step) {
    scrollBar->setValue(y - step + editor->textHeight(0));
  }
}

void DiffView::dropEvent(QDropEvent *event) {
  if (event->dropAction() != Qt::CopyAction)
    return;

  event->acceptProposedAction();

  // Copy files into the workdir.
  RepoView *view = RepoView::parentView(this);
  git::Repository repo = view->repo();
  foreach (const QUrl &url, event->mimeData()->urls()) {
    if (url.isLocalFile())
      copy(url.toString(DiffViewStyle::kUrlFormat), repo.workdir());
  }

  // FIXME: Work dir changed?
  view->refresh();
}

void DiffView::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

bool DiffView::canFetchMore() {
  if (!mFiles.isEmpty()) {
    FileWidget *lastFile = mFiles.last();
    if (lastFile->canFetchMore())
      return true;
  }

  auto dtw = dynamic_cast<DoubleTreeWidget *>(
      mParent); // for an unknown reason parent() and p are not the same
  assert(dtw);
  return mDiff.isValid() &&
         mFiles.size() < mDiffTreeModel->fileCount(dtw->selectedIndex());
}

/*!
 * \brief DiffView::fetchMore
 * Fetch maxNewFiles more patches
 * use a while loop with canFetchMore() to get all
 */
void DiffView::fetchMore(int fetchWidgets) {
  QVBoxLayout *layout = static_cast<QVBoxLayout *>(widget()->layout());

  // Add widgets.
  RepoView *view = RepoView::parentView(this);
  int addedWidgets = 0;

  // Fetch all files
  bool fetchAll = fetchWidgets < 0 ? true : false;
  if (fetchWidgets < 0)
    fetchWidgets = 4;

  // First load all hunks of last file before loading new files
  bool fetchFiles = true;
  if (!mFiles.isEmpty()) {
    FileWidget *lastFile = mFiles.last();
    while (lastFile->canFetchMore() &&
           ((verticalScrollBar()->maximum() - verticalScrollBar()->value() <
             height() / 2) ||
            fetchAll)) {
      addedWidgets += lastFile->fetchMore(fetchAll ? -1 : 1);

      // Load hunk(s) and update scrollbar
      QApplication::processEvents();

      // Running the eventloop may trigger a view refresh
      if (mFiles.isEmpty())
        return;
    }

    // Stop loading files
    if (verticalScrollBar()->maximum() - verticalScrollBar()->value() >
        height() / 2)
      fetchFiles = false;
  }

  if (fetchFiles) {
    auto dtw = dynamic_cast<DoubleTreeWidget *>(mParent);
    // QList<int> patchIndices =
    // mDiffTreeModel->patchIndices(dtw->selectedIndex());
    QList<QModelIndex> indexList = dtw->selectedIndices();
    QList<QModelIndex> indices;
    for (auto index : indexList) {
      QList<QModelIndex> addList = mDiffTreeModel->modelIndices(index);
      for (auto add : addList) {
        if (!indices.contains(add))
          indices.append(add);
      }
    }
    int count = indices.count();

    for (int i = mFiles.count(); i < count && addedWidgets < fetchWidgets;
         ++i) {

      int pidx = indices[i].data(DiffTreeModel::PatchIndexRole).toInt();
      git::Patch patch = mDiff.patch(pidx);
      if (!patch.isValid()) {
        // This diff is stale. Refresh the view.
        QTimer::singleShot(0, [view]() { view->refresh(); });
        return;
      }

      auto state = static_cast<git::Index::StagedState>(
          indices[i].data(Qt::CheckStateRole).toInt());

      git::Patch staged;
      const int index = mStagedPatches.value(patch.name(), -1);
      if (index != -1)
        staged = mStagedDiff.patch(index);

      git::Repository repo = view->repo();
      QString name = patch.name();
      QString path = repo.workdir().filePath(name);
      bool submodule = repo.lookupSubmodule(name).isValid();
      FileWidget *file = new FileWidget(this, mDiff, patch, staged, indices[i],
                                        name, path, submodule, widget());
      file->setStageState(state);
      mFileWidgetLayout->addWidget(file);
      addedWidgets += file->hunks().count();

      mFiles.append(file);

      if (file->isEmpty()) {
        DisclosureButton *button = file->header()->disclosureButton();
        button->setChecked(false);
        button->setEnabled(false);
      }

      // Respond to diagnostic signal.
      connect(file, &FileWidget::diagnosticAdded, this,
              &DiffView::diagnosticAdded);
      connect(file, &FileWidget::stageStateChanged,
              [this](const QModelIndex index, int state) {
                /*emit fileStageStateChanged(state);*/
                mDiffTreeModel->setData(index, state, Qt::CheckStateRole);
              });
      connect(file, &FileWidget::discarded, [this](const QModelIndex &index) {
        RepoView *view = RepoView::parentView(this);
        if (!mDiffTreeModel->discard(index)) {
          QString name = index.data(Qt::DisplayRole).toString();
          LogEntry *parent = view->addLogEntry(name, FileWidget::tr("Discard"));
          view->error(parent, FileWidget::tr("discard"), name);
        }
        view->refresh();
      });
    }
  }

  // Finish layout.
  if (mFiles.size() == mDiff.count()) {
    // Add comments widget.
    if (!mComments.comments.isEmpty())
      layout->addWidget(new CommentWidget(mComments.comments, widget()));

    layout->addStretch();
  }
}

void DiffView::fetchAll(int index) {
  // Load all patches up to and including index.
  while ((index < 0 || mFiles.size() <= index) && canFetchMore())
    fetchMore();
}

void DiffView::indexChanged(const QStringList &paths) {
  //    // Respond to index changes.
  //    RepoView *view = RepoView::parentView(this);
  //    git::Repository repo = view->repo();

  //    mStagedPatches.clear();
  //    // Generate a diff between the head tree and index.
  //    if (mDiff.isStatusDiff()) {
  //      if (git::Reference head = repo.head()) {
  //        if (git::Commit commit = head.target()) {
  //          git::Diff stagedDiff = repo.diffTreeToIndex(commit.tree());
  //          for (int i = 0; i < stagedDiff.count(); ++i)
  //            mStagedPatches[stagedDiff.name(i)] = stagedDiff.patch(i);
  //        }
  //      }
  //    }

  //    for (auto* file : mFiles) {
  //        for (auto path : paths) {
  //            git::Patch stagedPatch = mStagedPatches[path];
  //            if (file->name() == path)
  //                file->updateHunks(stagedPatch);
  //        }
  //    }
}

void DiffView::moveHalfPageDown() { moveRelative(height() / 2); }

void DiffView::moveHalfPageUp() { moveRelative(-height() / 2); }

void DiffView::moveRelative(int pixelsDown) {
  int oldPosition = verticalScrollBar()->sliderPosition();
  int newPosition = oldPosition + pixelsDown;
  verticalScrollBar()->setSliderPosition(newPosition);
}
