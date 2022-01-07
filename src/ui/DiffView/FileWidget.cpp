
#include "FileWidget.h"
#include "DisclosureButton.h"
#include "EditButton.h"
#include "DiscardButton.h"
#include "LineStats.h"
#include "FileLabel.h"
#include "HunkWidget.h"
#include "Images.h"
#include "conf/Settings.h"
#include "ui/RepoView.h"
#include "ui/Badge.h"
#include "ui/FileContextMenu.h"
#include "git/Repository.h"

#include "git/Buffer.h"


#include <QCheckBox>
#include <QContextMenuEvent>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>

namespace  {
    bool disclosure = false;
}

_FileWidget::Header::Header(
  const git::Diff &diff,
  const git::Patch &patch,
  bool binary,
  bool lfs,
  bool submodule,
  QWidget *parent)
  : QFrame(parent), mDiff(diff), mPatch(patch), mSubmodule(submodule)
{
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  QString name = patch.name();
  mCheck = new QCheckBox(this);
  mCheck->setVisible(diff.isStatusDiff());
  mCheck->setTristate(true);

  mStatusBadge = new Badge({}, this);

  git::Patch::LineStats lineStats;
  lineStats.additions = 0;
  lineStats.deletions = 0;
  mStats = new LineStats(lineStats, this);
  mStats->setVisible(false);

  mFileLabel = new FileLabel(name, submodule, this);

  QHBoxLayout *buttons = new QHBoxLayout;
  buttons->setContentsMargins(0,0,0,0);
  buttons->setSpacing(4);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(4,4,4,4);
  layout->addWidget(mCheck);
  layout->addSpacing(4);
  layout->addWidget(mStatusBadge);
  layout->addWidget(mStats);
  layout->addWidget(mFileLabel, 1);
  layout->addStretch();
  layout->addLayout(buttons);

  // Add LFS buttons.
  if (lfs) {
    Badge *lfsBadge = new Badge({Badge::Label(FileWidget::tr("LFS"), true)}, this);
    buttons->addWidget(lfsBadge);

    QToolButton *lfsLockButton = new QToolButton(this);
    bool locked = patch.repo().lfsIsLocked(patch.name());
    lfsLockButton->setText(locked ? FileWidget::tr("Unlock") : FileWidget::tr("Lock"));
    buttons->addWidget(lfsLockButton);

    connect(lfsLockButton, &QToolButton::clicked, [this, patch] {
      bool locked = patch.repo().lfsIsLocked(patch.name());
      RepoView::parentView(this)->lfsSetLocked({patch.name()}, !locked);
    });

    git::RepositoryNotifier *notifier = patch.repo().notifier();
    connect(notifier, &git::RepositoryNotifier::lfsLocksChanged, this,
    [patch, lfsLockButton] {
      bool locked = patch.repo().lfsIsLocked(patch.name());
      lfsLockButton->setText(locked ? FileWidget::tr("Unlock") : FileWidget::tr("Lock"));
    });

    mLfsButton = new QToolButton(this);
    mLfsButton->setText(FileWidget::tr("Show Object"));
    mLfsButton->setCheckable(true);

    buttons->addWidget(mLfsButton);
    buttons->addSpacing(8);
  }

  // Add edit button.
  mEdit = new EditButton(patch, -1, binary, lfs, this);
  mEdit->setToolTip(FileWidget::tr("Edit File"));
  buttons->addWidget(mEdit);

  // Add discard button.
  mDiscardButton = new DiscardButton(this);
  mDiscardButton->setVisible(false);
  mDiscardButton->setToolTip(FileWidget::tr("Discard File"));
  buttons->addWidget(mDiscardButton);
  connect(mDiscardButton, &QToolButton::clicked, this, &_FileWidget::Header::discard);

  mDisclosureButton = new DisclosureButton(this);
  mDisclosureButton->setToolTip(
    mDisclosureButton->isChecked() ? FileWidget::tr("Collapse File") : FileWidget::tr("Expand File"));
  connect(mDisclosureButton, &DisclosureButton::toggled, [this] {
    mDisclosureButton->setToolTip(
      mDisclosureButton->isChecked() ? FileWidget::tr("Collapse File") : FileWidget::tr("Expand File"));
  });
  mDisclosureButton->setVisible(disclosure);
  buttons->addWidget(mDisclosureButton);

  updatePatch(patch);

  if (!diff.isStatusDiff())
    return;

  // Set initial check state.
  updateCheckState();
}

void _FileWidget::Header::updatePatch(const git::Patch &patch) {
    char status = git::Diff::statusChar(patch.status());
    mStatusBadge->setLabels({Badge::Label(QChar(status))});

    git::Patch::LineStats lineStats = patch.lineStats();
    mStats->setStats(lineStats);
    mStats->setVisible(lineStats.additions > 0 || lineStats.deletions > 0);

    mFileLabel->setName(patch.name());
    if (patch.status() == GIT_DELTA_RENAMED)
      mFileLabel->setOldName(patch.name(git::Diff::OldFile));

    mEdit->updatePatch(patch, -1);

    mDiscardButton->setVisible(mDiff.isStatusDiff() && !mSubmodule && !patch.isConflicted());

}
QCheckBox *_FileWidget::Header::check() const
{
    return mCheck;
}

DisclosureButton *_FileWidget::Header::disclosureButton() const
{
    return mDisclosureButton;
}

QToolButton *_FileWidget::Header::lfsButton() const
{
    return mLfsButton;
}

void _FileWidget::Header::setStageState(git::Index::StagedState state) {
    if (state == git::Index::Staged)
        mCheck->setCheckState(Qt::Checked);
    else if (state == git::Index::Unstaged)
        mCheck->setCheckState(Qt::Unchecked);
    else
        mCheck->setCheckState(Qt::PartiallyChecked);
}

void _FileWidget::Header::mouseDoubleClickEvent(QMouseEvent *event)
{
  Q_UNUSED(event)

  if (mDisclosureButton->isEnabled())
    mDisclosureButton->toggle();
}

void _FileWidget::Header::contextMenuEvent(QContextMenuEvent *event)
{
  RepoView *view = RepoView::parentView(this);
  FileContextMenu menu(view, {mPatch.name()}, mDiff.index());
  menu.exec(event->globalPos());
}

void _FileWidget::Header::updateCheckState()
{
  bool disabled = false;
  Qt::CheckState state = Qt::Unchecked;
  switch (mDiff.index().isStaged(mPatch.name())) {
    case git::Index::Disabled:
      disabled = true;
      break;

    case git::Index::Unstaged:
      break;

    case git::Index::PartiallyStaged:
      state = Qt::PartiallyChecked;
      break;

    case git::Index::Staged:
      state = Qt::Checked;
      break;

    case git::Index::Conflicted:
      disabled = (mPatch.count() > 0);
      break;
  }

  mCheck->setCheckState(state);
  mCheck->setEnabled(!disabled);
}

//###############################################################################
//###############      FileWidget     ###########################################
//###############################################################################

FileWidget::FileWidget(DiffView *view,
  const git::Diff &diff,
  const git::Patch &patch,
  const git::Patch &staged,
  QWidget *parent)
  : QWidget(parent), mView(view), mDiff(diff), mPatch(patch)
{
  setObjectName("FileWidget");
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);

  git::Repository repo = RepoView::parentView(this)->repo();

  QString name = patch.name();
  QString path = repo.workdir().filePath(name);
  bool submodule = repo.lookupSubmodule(name).isValid();

  bool binary = patch.isBinary();
  if (patch.isUntracked()) {
    QFile dev(path);
    if (dev.open(QFile::ReadOnly)) {
      QByteArray content = dev.readAll();
      git::Buffer buffer(content.constData(), content.length());
      binary = buffer.isBinary();
    }
  }

  bool lfs = patch.isLfsPointer();
  mHeader = new _FileWidget::Header(diff, patch, binary, lfs, submodule, parent);

  // Respond to check changes
  connect(mHeader->check(), &QCheckBox::clicked, this,
          [this](bool checked) {
    if (mDiff.isValid()) {
      git::Index index = mDiff.index();
      mDiff.index().setStaged({mPatch.name()}, checked);
    }
  });
  connect(mHeader, &_FileWidget::Header::discard, this, &FileWidget::discard);
  layout->addWidget(mHeader);

  DisclosureButton *disclosureButton = mHeader->disclosureButton();
  if (disclosure)
      connect(disclosureButton, &DisclosureButton::toggled, [this](bool visible) {

        if (mHeader->lfsButton() && !visible) {
          mHunks.first()->setVisible(false);
          if (!mImages.isEmpty())
            mImages.first()->setVisible(false);
          return;
        }

        if (mHeader->lfsButton() && visible) {
          bool checked = mHeader->lfsButton()->isChecked();
          mHunks.first()->setVisible(!checked);
          if (!mImages.isEmpty())
            mImages.first()->setVisible(checked);
          return;
        }

        foreach (HunkWidget *hunk, mHunks)
          hunk->setVisible(visible);
      });

  if (diff.isStatusDiff()) {
    // Collapse on check.
    if (disclosure)
        connect(mHeader->check(), &QCheckBox::stateChanged, [this](int state) {
                mHeader->disclosureButton()->setChecked(state != Qt::Checked);
    });
  }

  // Try to load an image from the file.
  if (binary) {
    layout->addWidget(addImage(disclosureButton, mPatch));
    return;
  }

  mHunkLayout = new QVBoxLayout();
  layout->addLayout(mHunkLayout);
  layout->addSpacerItem(new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Expanding)); // so the hunkwidget is always starting from top and is not distributed over the hole filewidget

  updatePatch(patch, staged);
  updateStageState();

  // LFS
  if (QToolButton *lfsButton = mHeader->lfsButton()) {
    connect(lfsButton, &QToolButton::clicked,
    [this, layout, disclosureButton, lfsButton](bool checked) {
      lfsButton->setText(checked ? tr("Show Pointer") : tr("Show Object"));
      mHunks.first()->setVisible(!checked);

      // Image already loaded.
      if (!mImages.isEmpty()) {
        mImages.first()->setVisible(checked);
        return;
      }

      // Load image.
      layout->addWidget(addImage(disclosureButton, mPatch, true));
    });
  }

  // Start hidden when the file is checked.
  bool expand = (mHeader->check()->checkState() == Qt::Unchecked);

  if (Settings::instance()->value("collapse/added").toBool() == true &&
      patch.status() == GIT_DELTA_ADDED)
    expand = false;

  if (Settings::instance()->value("collapse/deleted").toBool() == true &&
      patch.status() == GIT_DELTA_DELETED)
    expand = false;

  disclosureButton->setChecked(expand);
}

bool FileWidget::isEmpty()
{
  return (mHunks.isEmpty() && mImages.isEmpty());
}

void FileWidget::updateStageState()
{
  if (mDiff.isValid() && mDiff.isStatusDiff()) {
    git::Index::StagedState stageState = mDiff.index().isStaged(name());
    mHeader->setStageState(stageState);

    for (auto hunk : mHunks)
      hunk->updateStageState(stageState);
  }
}

void FileWidget::updatePatch(const git::Patch &patch,
                             const git::Patch &staged)
{
  mPatch = patch;
  mStaged = staged;

  mHeader->updatePatch(patch);

  // Add untracked file content.
  if (patch.isUntracked()) {
    git::Repository repo = RepoView::parentView(this)->repo();

    QString name = patch.name();
    QString path = repo.workdir().filePath(name);
    bool submodule = repo.lookupSubmodule(name).isValid();
    bool lfs = patch.isLfsPointer();

    // Remove all hunks
    QLayoutItem *child;
    while ((child = mHunkLayout->takeAt(0)) != 0)
      delete child;

    mHunks.clear();

    if (!QFileInfo(path).isDir())
      mHunkLayout->addWidget(addHunk(mDiff, mPatch, mStaged, -1, lfs, submodule));
  } else {
    for (auto hunk : mHunks)
      hunk->updatePatch(patch, staged);
  }
}

bool FileWidget::canFetchMore()
{
  return mHunks.count() < mPatch.count();
}

/*!
 * \brief DiffView::fetchMore
 * Fetch count more patches
 * use a while loop with canFetchMore() to get all
 */
void FileWidget::fetchMore(int count)
{
  RepoView *view = RepoView::parentView(this);
  git::Repository repo = view->repo();

  // Add widgets.
  int patchCount = mPatch.count();
  int hunksCount = mHunks.count();

  // Fetch all hunks
  if (count < 0)
    count = patchCount;

  bool lfs = mPatch.isLfsPointer();
  QString name = mPatch.name();
  bool submodule = repo.lookupSubmodule(name).isValid();

  for (int i = hunksCount; i < patchCount && i < (hunksCount + count); ++i) {
    HunkWidget *hunk = addHunk(mDiff, mPatch, mStaged, i, lfs, submodule);
    mHunkLayout->addWidget(hunk);
  }
}

void FileWidget::fetchAll(int index)
{
  // Load all patches up to and including index.
  int hunksCount = mHunks.count();
  while ((index < 0 || hunksCount <= index) && canFetchMore())
    fetchMore();
}

_FileWidget::Header *FileWidget::header() const
{
    return mHeader;
}

QWidget *FileWidget::addImage(
  DisclosureButton *button,
  const git::Patch patch,
  bool lfs)
{
  Images *images = new Images(patch, lfs, this);

  // Hide on file collapse.
  if (!lfs && disclosure)
    connect(button, &DisclosureButton::toggled, images, &QLabel::setVisible);

  // Remember image.
  mImages.append(images);

  return images;
}

HunkWidget *FileWidget::addHunk(
  const git::Diff &diff,
  const git::Patch &patch,
  const git::Patch &staged,
  int index,
  bool lfs,
  bool submodule)
{
  HunkWidget *hunk =
    new HunkWidget(mView, diff, patch, staged, index, lfs, submodule, this);

  connect(hunk, &HunkWidget::stageStateChanged,
          [this, hunk](git::Index::StagedState stageState) {
    stageHunks(hunk, stageState);
  });
  connect(hunk, &HunkWidget::discard, this, &FileWidget::discardHunk);
  TextEditor* editor = hunk->editor(false);

  // Respond to editor diagnostic signal.
  connect(editor, &TextEditor::diagnosticAdded,
  [this](int line, const TextEditor::Diagnostic &diag) {
    Q_UNUSED(line)

    emit diagnosticAdded(diag.kind);
  });

  // Remember hunk.
  mHunks.append(hunk);

  return hunk;
}

void FileWidget::stageHunks(HunkWidget *hunk,
                            git::Index::StagedState stageState)
{
  git::Index index = mDiff.index();
  if (!index.isValid())
    return;

  bool completeHunk = stageState == git::Index::PartiallyStaged ? false : true;

  if (completeHunk) {
    if (stageState == git::Index::Unstaged) {
      if (mStaged.count() <= 0)
        return;

      // Index of removed hunk
      int hidx = mHunks.indexOf(hunk);
      const git_diff_hunk *unstageHeader = mPatch.header_struct(hidx);

      // Remove staged patch
      QBitArray stageHunks(mStaged.count(), true);
      for (int i = 0; i < mStaged.count(); i++) {
        const git_diff_hunk *stageHeader = mStaged.header_struct(i);
        int old_start = stageHeader->old_start - unstageHeader->old_start;
        if ((old_start >= 0 && old_start <= unstageHeader->old_lines) &&
            (stageHeader->old_lines <= unstageHeader->old_lines))
          stageHunks[i] = false;
      }

      QByteArray stageBuffer = mStaged.apply(stageHunks);

      // Add the buffer to the index.
      index.add(mPatch.name(), stageBuffer);
    } else {
      QByteArray stageBuffer;
      if (mStaged.count() <= 0) {

        // Add first hunk
        int hidx = mHunks.indexOf(hunk);
        QBitArray stageHunks(mPatch.count(), false);
        stageHunks[hidx] = true;
        stageBuffer = mPatch.apply(stageHunks);
      } else {

        // Create image
        QList<QList<QByteArray>> image;
        QByteArray buffer = mPatch.apply(QBitArray(mPatch.count(), false));
        mPatch.populatePreimage(image, buffer);

        // Index of new hunk
        int hidx = mHunks.indexOf(hunk);
        const git_diff_hunk *unstageHeader = mPatch.header_struct(hidx);

        // Add staged hunks to image, skip already staged patch
        for (int i = 0; i < mStaged.count(); i++) {
          const git_diff_hunk *stageHeader = mStaged.header_struct(i);
          int old_start = stageHeader->old_start - unstageHeader->old_start;
          if ((old_start >= 0 && old_start <= unstageHeader->old_lines) &&
              (stageHeader->old_lines <= unstageHeader->old_lines))
            continue;

          mStaged.apply(image, i, -1, -1);
        }

        // Add new hunk
        mPatch.apply(image, hidx, -1, -1);

        stageBuffer = mPatch.generateResult(image);
      }
      // Add the buffer to the index.
      index.add(mPatch.name(), stageBuffer);
    }
  } else {

    // Create image
    QList<QList<QByteArray>> image;
    QByteArray buffer = mPatch.apply(QBitArray(mPatch.count(), false));
    mPatch.populatePreimage(image, buffer);

    // Index of changed hunk
    int hidx = mHunks.indexOf(hunk);
    const git_diff_hunk *unstageHeader = mPatch.header_struct(hidx);

    // Add staged hunks to image, skip already staged patch
    for (int i = 0; i < mStaged.count(); i++) {
      const git_diff_hunk *stageHeader = mStaged.header_struct(i);
      int old_start = stageHeader->old_start - unstageHeader->old_start;
      if ((old_start >= 0 && old_start <= unstageHeader->old_lines) &&
          (stageHeader->old_lines <= unstageHeader->old_lines))
        continue;

      mStaged.apply(image, i, -1, -1);
    }

    // Apply changed hunk data
    QList<int> ignore_lines = hunk->unstagedLines();
    mPatch.apply(image, hidx, -1, -1, ignore_lines);

    // Add the buffer to the index.
    buffer = mPatch.generateResult(image);
    index.add(mPatch.name(), buffer);
  }

  // Update hunk stage state
  hunk->header()->setStageState(stageState);
}

void FileWidget::discardHunk()
{
  HunkWidget* hunk = static_cast<HunkWidget*>(QObject::sender());
  git::Repository repo = mPatch.repo();

  if (mPatch.isUntracked()) {
    repo.workdir().remove(mPatch.name());
      return;
  }

  // Unstage (partially staged) hunk before deletion
  stageHunks(hunk, git::Index::Unstaged);

  // Discard selected hunk
  int hidx = mHunks.indexOf(hunk);
  QBitArray stageHunks(mPatch.count(), true);
  stageHunks[hidx] = false;
  QByteArray buffer = mPatch.apply(stageHunks);

  // Create image
  QList<QList<QByteArray>> image;
  mPatch.populatePreimage(image, buffer);

  // Apply changed hunk data
  QList<int> ignore_lines = hunk->discardedLines();
  mPatch.apply(image, hidx, -1, -1, ignore_lines);

  // Write buffer to disk
  buffer = mPatch.generateResult(image);
  QSaveFile file(repo.workdir().filePath(mPatch.name()));
  if (!file.open(QFile::WriteOnly))
    return;

  file.write(buffer);
  if (!file.commit())
    return;

  // Workdir changed
  RepoView::parentView(this)->refresh();
}

void FileWidget::discard() {
  QString name = mPatch.name();
  bool untracked = mPatch.isUntracked();
  QString path = mPatch.repo().workdir().filePath(name);
  QString type = QFileInfo(path).isDir() ? FileWidget::tr("Directory") : FileWidget::tr("File");

  QString title = untracked ? FileWidget::tr("Remove %1?").arg(type) :
                              FileWidget::tr("Discard Changes?");
  QString text = untracked ?  FileWidget::tr("Are you sure you want to remove '%1'?") :
                              FileWidget::tr("Are you sure you want to discard all changes in '%1'?");
  QMessageBox *dialog = new QMessageBox(QMessageBox::Warning,
                                        title, text.arg(name),
                                        QMessageBox::Cancel, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setInformativeText(FileWidget::tr("This action cannot be undone."));

  QPushButton *discard = dialog->addButton(untracked ? FileWidget::tr("Remove %1").arg(type) :
                                                       FileWidget::tr("Discard Changes"),
                                           QMessageBox::AcceptRole);
  connect(discard, &QPushButton::clicked, [this] {
    emit discarded(mPatch.name());
  });

  dialog->exec();
}
