
#include "EditButton.h"
#include "ui/RepoView.h"
#include <QPainterPath>

EditButton::EditButton(
  const git::Patch &patch,
  int index,
  bool binary,
  bool lfs,
  QWidget *parent)
  : Button(parent)
{
  setObjectName("EditButton");

  // Add edit button menu.
  QMenu *menu = new QMenu(this);
  mEditWorkingCopyAction = menu->addAction(tr("Edit Working Copy"));
  mEditNewRevisionAction = menu->addAction(tr("Edit New Revision"));
  mEditOldRevisionAction = menu->addAction(tr("Edit Old Revision"));
  setMenu(menu);

  setPopupMode(QToolButton::InstantPopup);

  // Update button actions.
  updatePatch(patch, index, true);

  // Enable edit button.
  setVisible(!binary && !lfs);
}

void EditButton::updatePatch(const git::Patch &patch, int index, bool init)
{
  if ((!isEnabled() || !isVisible()) && !init)
    return;

  RepoView *view = RepoView::parentView(this);
  QString name = patch.name();

  // Calculate starting line numbers.
  int oldLine = -1;
  int newLine = -1;
  if (index >= 0 && patch.lineCount(index) > 0) {
    oldLine = patch.lineNumber(index, 0, git::Diff::OldFile);
    newLine = patch.lineNumber(index, 0, git::Diff::NewFile);
  }

  // Working copy action.
  mEditWorkingCopyAction->disconnect();
  if (view->repo().workdir().exists(name)) {
    mEditWorkingCopyAction->setVisible(true);
    connect(mEditWorkingCopyAction, &QAction::triggered, this,
      [view, name, newLine] {
      view->edit(name, newLine);
    });
  } else
    mEditWorkingCopyAction->setVisible(false);

  // Revision edit actions.
  QList<git::Commit> commits = view->commits();
  git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();
  git::Blob newBlob = patch.blob(git::Diff::NewFile);
  git::Blob oldBlob = patch.blob(git::Diff::OldFile);

  mEditNewRevisionAction->disconnect();
  if (newBlob.isValid()) {
    mEditNewRevisionAction->setVisible(true);
    connect(mEditNewRevisionAction, &QAction::triggered, this,
      [view, name, newLine, newBlob, commit] {
      view->openEditor(name, newLine, newBlob, commit);
    });
  } else
    mEditNewRevisionAction->setVisible(false);

  mEditOldRevisionAction->disconnect();
  if (oldBlob.isValid()) {
    mEditOldRevisionAction->setVisible(true);
    if (commit.isValid()) {
      QList<git::Commit> parents = commit.parents();
      if (!parents.isEmpty())
        commit = parents.first();
    }

    connect(mEditOldRevisionAction, &QAction::triggered, this,
      [view, name, oldLine, oldBlob, commit] {
      view->openEditor(name, oldLine, oldBlob, commit);
    });
  } else
    mEditOldRevisionAction->setVisible(false);
}

void EditButton::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event)

  QPainter painter(this);
  initButtonPainter(&painter);

  qreal r = 6;
  qreal x = width() / 2;
  qreal y = height() / 2;

  QPainterPath path;
  path.moveTo(x - r, y + r);
  path.lineTo(x - r, y + r - 4);
  path.lineTo(x + r - 4, y - r);
  path.lineTo(x + r, y - r + 4);
  path.lineTo(x - r + 4, y + r);
  path.lineTo(x - r, y + r);
  painter.drawPath(path);

  painter.setPen(QPen(painter.pen().color(), 1, Qt::SolidLine, Qt::FlatCap));
  painter.drawLine(x - r + 1, y + r - 5, x - r + 5, y + r - 1);
  painter.drawLine(x + r - 6, y - r + 2, x + r - 2, y - r + 6);
}
