#ifndef FILEWIDGET_H
#define FILEWIDGET_H

#include "../../editor/TextEditor.h"

#include <QWidget>
#include <QFrame>

#include "git/Diff.h"
#include "git/Patch.h"

class QCheckBox;
class QToolButton;
class QVBoxLayout;

class DisclosureButton;
class DiscardButton;
class EditButton;
class LineStats;
class Badge;
class FileLabel;
class DiffView;
class HunkWidget;

namespace _FileWidget {
class Header : public QFrame
{
    Q_OBJECT

public:
  Header(
    const git::Diff &diff,
    const git::Patch &patch,
    bool binary,
    bool lfs,
    bool submodule,
    QWidget *parent = nullptr);
  void updatePatch(const git::Patch &patch);
  QCheckBox *check() const;

  DisclosureButton *disclosureButton() const;

  QToolButton *lfsButton() const;
  void setStageState(git::Index::StagedState state);

signals:
  void stageStageChanged(int stageState);
  void discard();
protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  void updateCheckState();

  git::Diff mDiff;
  git::Patch mPatch;
  bool mSubmodule;

  QCheckBox *mCheck{nullptr};
  QToolButton *mLfsButton = nullptr;
  EditButton *mEdit{nullptr};
  DiscardButton *mDiscardButton{nullptr};
  DisclosureButton *mDisclosureButton{nullptr};
  LineStats* mStats{nullptr};
  Badge *mStatusBadge{nullptr};
  FileLabel *mFileLabel{nullptr};
};

}

/*!
 * Represents the diff of a single file
 * \brief The FileWidget class
 */
class FileWidget : public QWidget
{
  Q_OBJECT

public:

  FileWidget(
    DiffView *view,
    const git::Diff &diff,
    const git::Patch &patch,
    const git::Patch &staged,
    QWidget *parent = nullptr);
  bool isEmpty();
  void updatePatch(const git::Patch &patch, const git::Patch &staged);
  /*!
   * Update hunks after index change and emits the current stage state of the hunks
   * \brief updateHunks
   */
  void updateHunks(git::Patch stagedPatch);
  _FileWidget::Header *header() const;
  QString name() const;

  QList<HunkWidget *> hunks() const;

  QWidget *addImage(
    DisclosureButton *button,
    const git::Patch patch,
    bool lfs = false);
  HunkWidget *addHunk(
    const git::Diff &diff,
    const git::Patch &patch,
    const git::Patch &staged,
    int index,
    bool lfs,
    bool submodule);
public slots:
  void headerCheckStateChanged(int state);
  /*!
   * Stages the changes of the hunk
   * emits signal "stageStateChanged" with the
   * current state of the hunk as parameter
   * \brief stageHunks
   */
  void stageHunks();
  /*!
   * Discard specific hunk
   * Emitted by the hunk it self
   * \brief discardHunk
   */
  void discardHunk();

signals:
  void diagnosticAdded(TextEditor::DiagnosticKind kind);
  void stageStateChanged(git::Index::StagedState state);

private:
  void discard();


  DiffView *mView{nullptr};

  git::Diff mDiff;
  git::Patch mPatch;

  _FileWidget::Header *mHeader{nullptr};
  QList<QWidget *> mImages;
  QList<HunkWidget *> mHunks;
  QVBoxLayout* mHunkLayout{nullptr};
  bool mSuppressUpdate{false};
  bool mSupressStaging{false};
};

#endif // FILEWIDGET_H

