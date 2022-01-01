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
#include "ui/DoubleTreeWidget.h"
#include "git/Tree.h"
#include <QScrollBar>
#include <QPushButton>
#include <QMimeData>

namespace {

bool copy(const QString &source, const QDir &targetDir)
{
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

  foreach (const QFileInfo &entry, QDir(source).entryInfoList(DiffViewStyle::kFilters)) {
    if (!copy(entry.filePath(), target))
      return false;
  }

  return true;
}

} // anon. namespace

DiffView::DiffView(const git::Repository &repo, QWidget *parent)
  : QScrollArea(parent)
{
  setStyleSheet(DiffViewStyle::kStyleSheet);
  setAcceptDrops(true);
  setWidgetResizable(true);
  setFocusPolicy(Qt::NoFocus);
  setContextMenuPolicy(Qt::ActionsContextMenu);

  mPlugins = Plugin::plugins(repo);

  // Update comments.
  if (Repository *remote = RepoView::parentView(this)->remoteRepo()) {
    connect(remote->account(), &Account::commentsReady, this, [this, remote](
      Repository *repo,
      const QString &oid,
      const Account::CommitComments &comments)
    {
      if (repo != remote)
        return;

      RepoView *view = RepoView::parentView(this);
      QList<git::Commit> commits = view->commits();
      if (commits.size() != 1 || oid != commits.first().id().toString())
        return;

      mComments = comments;

      // Invalidate editors.
      foreach (QWidget *widget, mFiles) {
        foreach (HunkWidget *hunk, static_cast<FileWidget *>(widget)->hunks())
          hunk->invalidate();
      }

      // Load commit comments.
      if (!canFetchMore())
        fetchMore();
    });
  }
}

DiffView::~DiffView() {}

QWidget *DiffView::file(int index)
{
  fetchAll(index);
  return mFiles.at(mIndexes.indexOf(index));
}

void DiffView::setDiff(const git::Diff &diff)
{
  RepoView *view = RepoView::parentView(this);
  git::Repository repo = view->repo();

  // Disconnect signals.
  foreach (QMetaObject::Connection connection, mConnections)
    disconnect(connection);
  mConnections.clear();

  // Clear state.
  mFiles.clear();
  mIndexes.clear();
  mStagedPatches.clear();
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
    layout->addSpacerItem(new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Expanding)); // so the file is always starting from top and is not distributed over the hole diff view
    return;
  }

  layout->addLayout(mFileWidgetLayout);
  layout->addSpacerItem(new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Expanding)); // so the file is always starting from top and is not distributed over the hole diff view

  // Generate a diff between the head tree and index.
  if (diff.isStatusDiff()) {
    if (git::Reference head = repo.head()) {
      if (git::Commit commit = head.target()) {
        git::Diff stagedDiff = repo.diffTreeToIndex(commit.tree());
        for (int i = 0; i < stagedDiff.count(); ++i)
          mStagedPatches[stagedDiff.name(i)] = stagedDiff.patch(i);
      }
    }
  }

  // Load patches on demand.
  QScrollBar *scrollBar = verticalScrollBar();
  mConnections.append(
    connect(scrollBar, &QScrollBar::valueChanged, [this](int value) {
      // Start loading (more) files when scrollbar hits the maximum
      if (value >= verticalScrollBar()->maximum() && canFetchMore())
        fetchMore();
    })
  );

  // Request comments for this diff.
  if (Repository *remoteRepo = view->remoteRepo()) {
    QList<git::Commit> commits = view->commits();
    if (commits.size() == 1) {
      QString oid = commits.first().id().toString();
      remoteRepo->account()->requestComments(remoteRepo, oid);
    }
  }

  // Respond to index changes
  connect(repo.notifier(), &git::RepositoryNotifier::indexChanged, this,
          [this](const QStringList &paths) {
    if (mDiff.isValid()) {
      for (auto file : mFiles) {
        if (paths.contains(file->name()))
          file->updateStageState();
      }
    }
  });
}

void DiffView::setFilter(const QStringList &paths)
{
  if (!mDiff.isValid())
    return;

  // Setup patch index list from paths
  QList<int> list;
  for (auto path : paths) {
    int idx = mDiff.indexOf(path);
    if (idx >= 0)
      list.append(idx);
  }

  if (list == mIndexes)
    return;

  mIndexes = list;

  // Remove files
  while (mFiles.count()) {
    FileWidget *file = mFiles.takeFirst();
    mFileWidgetLayout->removeWidget(file);
    delete file;
  }
  mFiles.clear();

  // Reset vertical scrollbar prior to (re)loading files
  verticalScrollBar()->setRange(0,0);
  verticalScrollBar()->setValue(0);

  // Add files
  if (canFetchMore())
    fetchMore();
}

QList<TextEditor *> DiffView::editors()
{
  fetchAll();
  QList<TextEditor *> editors;
  foreach (QWidget *widget, mFiles) {
    foreach (HunkWidget *hunk, static_cast<FileWidget *>(widget)->hunks())
      editors.append(hunk->editor());
  }

  return editors;
}

void DiffView::ensureVisible(TextEditor *editor, int pos)
{
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

void DiffView::dropEvent(QDropEvent *event)
{
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

void DiffView::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

bool DiffView::canFetchMore()
{
  if (!mFiles.isEmpty()) {
    FileWidget *lastFile = mFiles.last();
    if (lastFile->canFetchMore())
      return true;
  }

  return mDiff.isValid() && mFiles.size() < mIndexes.count();
}

/*!
 * \brief DiffView::fetchMore
 * Fetch maxNewFiles more patches
 * use a while loop with canFetchMore() to get all
 */
void DiffView::fetchMore(int count)
{
  // Add widgets.
  RepoView *view = RepoView::parentView(this);

  // Fetch all files
  bool fetchAll = count < 0 ? true : false;
  if (count < 0)
    count = 4;

  // First load all hunks of last file before loading new files
  if (!mFiles.isEmpty()) {
    FileWidget *lastFile = mFiles.last();
    while (lastFile->canFetchMore() &&
           ((verticalScrollBar()->maximum() - verticalScrollBar()->value() < height() / 2) ||
            fetchAll)) {
      lastFile->fetchMore(fetchAll ? -1 : 1);

      // Load hunk(s) and update scrollbar
      QApplication::processEvents();
    }

    // Stop loading files
    if (verticalScrollBar()->maximum() - verticalScrollBar()->value() > height() / 2)
      count = fetchAll ? 4 : 0;
  }

  for (int i = mFiles.count(); i < mIndexes.count() && i < (mFiles.count() + count); i++) {
    int pidx = mIndexes.at(i);
    git::Patch patch = mDiff.patch(pidx);
    if (!patch.isValid()) {
      // This diff is stale. Refresh the view.
      QTimer::singleShot(0, view, &RepoView::refresh);
      return;
    }

    git::Patch staged = mStagedPatches.value(patch.name());
    FileWidget *file = new FileWidget(this, mDiff, patch, staged, widget());

    mFileWidgetLayout->addWidget(file);
    mFiles.append(file);

    if (file->isEmpty()) {
      DisclosureButton *button = file->header()->disclosureButton();
      button->setChecked(false);
      button->setEnabled(false);
    }

    // Respond to diagnostic signal.
    connect(file, &FileWidget::diagnosticAdded,
            this, &DiffView::diagnosticAdded);
    // Respond to stage changes.
    connect(file, &FileWidget::stageStateChanged,
            this, &DiffView::stageStateChanged);
    // Respond to discard signal.
    connect(file, &FileWidget::discarded,
            this, &DiffView::discarded);

    // Load hunk(s) of file
    while (file->canFetchMore() &&
           ((verticalScrollBar()->maximum() - verticalScrollBar()->value() < height() / 2) ||
            fetchAll)) {
      file->fetchMore(fetchAll ? -1 : 1);

      // Load hunk(s) and update scrollbar
      QApplication::processEvents();
    }

    // Stop loading files
    if (verticalScrollBar()->maximum() - verticalScrollBar()->value() > height() / 2)
      count = fetchAll ? 4 : 0;
  }

  // Finish layout.
  if (mFiles.size() == mDiff.count()) {
    QVBoxLayout *layout = static_cast<QVBoxLayout *>(widget()->layout());
    // Add comments widget.
    if (!mComments.comments.isEmpty())
      layout->addWidget(new CommentWidget(mComments.comments, widget()));

    layout->addStretch();
  }
}

void DiffView::fetchAll(int index)
{
  // Load all patches up to and including index.
  while ((index < 0 || mFiles.size() <= mIndexes.indexOf(index)) && canFetchMore())
    fetchMore(-1);
}
