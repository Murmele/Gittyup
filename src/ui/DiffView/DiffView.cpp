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
  return mFiles.at(index);
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

    return;
  }

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

  if (canFetchMore())
    fetchMore();

  // Load patches on demand.
  QScrollBar *scrollBar = verticalScrollBar();
  mConnections.append(
    connect(scrollBar, &QScrollBar::valueChanged, [this](int value) {
      if (value > verticalScrollBar()->maximum() / 2 && canFetchMore())
        fetchMore();
    })
  );

  mConnections.append(
    connect(scrollBar, &QScrollBar::rangeChanged, [this](int min, int max) {
      if (max - min < this->widget()->height() / 2 && canFetchMore())
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

  connect(repo.notifier(), &git::RepositoryNotifier::indexChanged, this, &DiffView::indexChanged);
}

bool DiffView::scrollToFile(int index)
{
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

void DiffView::enable(bool enable)
{
    mEnabled = enable;
    setFilter(QStringList());
}

void DiffView::setFilter(const QStringList &paths)
{
  fetchAll();
  QSet<QString> set = QSet<QString>::fromList(paths);
  foreach (QWidget *widget, mFiles) {
    FileWidget *file = static_cast<FileWidget *>(widget);
    file->setVisible(mEnabled && (set.isEmpty() || set.contains(file->name())));
  }
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
  return (mDiff.isValid() && mFiles.size() < mDiff.count());
}

void DiffView::fetchMore()
{
  QVBoxLayout *layout = static_cast<QVBoxLayout *>(widget()->layout());

  // Add widgets.
  int init = mFiles.size();
  int patchCount = mDiff.count();
  RepoView *view = RepoView::parentView(this);
  for (int pidx = init; pidx < patchCount && pidx - init < 8; ++pidx) {
    git::Patch patch = mDiff.patch(pidx);
    if (!patch.isValid()) {
      // This diff is stale. Refresh the view.
      QTimer::singleShot(0, view, &RepoView::refresh);
      return;
    }

    git::Patch staged = mStagedPatches.value(patch.name());
    FileWidget *file = new FileWidget(this, mDiff, patch, staged, widget());
    layout->addWidget(file);

    mFiles.append(file);

    if (file->isEmpty()) {
      DisclosureButton *button = file->header()->disclosureButton();
      button->setChecked(false);
      button->setEnabled(false);
    }

    // Respond to diagnostic signal.
    connect(file, &FileWidget::diagnosticAdded,
            this, &DiffView::diagnosticAdded);
    connect(file, &FileWidget::stageStateChanged,
            [this] (git::Index::StagedState state) {emit fileStageStateChanged(state);});
  }

  // Finish layout.
  if (mFiles.size() == mDiff.count()) {
    // Add comments widget.
    if (!mComments.comments.isEmpty())
      layout->addWidget(new CommentWidget(mComments.comments, widget()));

    layout->addStretch();
  }
}

void DiffView::fetchAll(int index)
{
  // Load all patches up to and including index.
  while ((index < 0 || mFiles.size() <= index) && canFetchMore())
    fetchMore();
}

void DiffView::indexChanged(const QStringList &paths) {
    // Respond to index changes.
    RepoView *view = RepoView::parentView(this);
    git::Repository repo = view->repo();

    mStagedPatches.clear();
    // Generate a diff between the head tree and index.
    if (mDiff.isStatusDiff()) {
      if (git::Reference head = repo.head()) {
        if (git::Commit commit = head.target()) {
          git::Diff stagedDiff = repo.diffTreeToIndex(commit.tree());
          for (int i = 0; i < stagedDiff.count(); ++i)
            mStagedPatches[stagedDiff.name(i)] = stagedDiff.patch(i);
        }
      }
    }

    for (auto* file : mFiles) {
        for (auto path : paths) {
            git::Patch stagedPatch = mStagedPatches[path];
            if (file->name() == path)
                file->updateHunks(stagedPatch);
        }
    }
}
