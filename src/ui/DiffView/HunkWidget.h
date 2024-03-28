#ifndef HUNKWIDGET_H
#define HUNKWIDGET_H

#include <QFrame>
#include <QCheckBox>
#include <QWidget>
#include <QLabel>

#include "../../editor/TextEditor.h"
#include "../../git/Patch.h"
#include "host/Account.h"

#include "git/Diff.h"

class DiffView;
class Header;
class QToolButton;
class DisclosureButton;
class Line;

namespace _HunkWidget {
class HunkLabel : public QWidget {
public:
  HunkLabel(const QString &name, bool submodule, QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QString mName;
  QString mOldName;
};

class Header : public QFrame {
  Q_OBJECT
public:
  Header(const git::Diff &diff, const git::Patch &patch, int index, bool lfs,
         bool submodule, QWidget *parent = nullptr);
  QCheckBox *check() const;

  DisclosureButton *button() const;

  QToolButton *saveButton() const;
  QToolButton *undoButton() const;
  QToolButton *oursButton() const;
  QToolButton *theirsButton() const;

public slots:
  void setCheckState(git::Index::StagedState state);

protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override;

signals:
  void stageStateChanged(int stageState);
  void discard();

private:
  QCheckBox *mCheck;
  DisclosureButton *mButton;
  QToolButton *mSave = nullptr;
  QToolButton *mUndo = nullptr;
  QToolButton *mOurs = nullptr;
  QToolButton *mTheirs = nullptr;
};
} // namespace _HunkWidget

/*!
 * Represents one hunk of a patch of a file.
 * \brief The HunkWidget class
 */
class HunkWidget : public QFrame {
  Q_OBJECT

public:
  HunkWidget(DiffView *view, const git::Diff &diff, const git::Patch &patch,
             const git::Patch &staged, int index, bool lfs, bool submodule,
             QWidget *parent = nullptr);
  _HunkWidget::Header *header() const;
  TextEditor *editor(bool ensureLoaded = true);
  void invalidate();
  /*!
   * Return hunk retrieved from the editor with removed discard lines
   * Idea is to store the changes only in the texteditor
   * and provide the data to the patch if needed
   * \brief hunk
   * \return
   */
  QByteArray hunk() const;
  /*!
   * Return hunk retrieved from the editor to apply patch
   * Idea is to store the changes only in the texteditor
   * and provide the data to the patch if needed
   * \brief hunk
   * \return
   */
  QByteArray apply();
  /*!
   * \brief stageState
   * Calculate stage state of the hunk. Git does not provide
   * such functionality, so the staged and unstaged lines
   * must be counted.
   * \return
   */
  git::Index::StagedState stageState();
  /*!
   * Stage/Unstage all
   * \brief setStaged
   * \param staged
   */
  void setStaged(bool staged);
  void setStageState(git::Index::StagedState state);
  void stageSelected(int startLine, int end, bool emitSignal = true);
  void unstageSelected(int startLine, int end, bool emitSignal = true);
  void discardSelected(int startLine, int end);
  /*!
   * Called by the hunk header
   * \brief discard
   */
  void discard();
  void load();
  /*!
   * update hunk content
   * \brief load
   * \param force Set to true to force reloading
   */
  void load(git::Patch &staged, bool force = false);

signals:
  /*!
   * It is not possible to stage single hunks.
   * So the complete file must be staged. Inform the FileWidget
   * about changes and it will perform a stage
   * \brief stageStateChanged
   * \param stageState
   */
  void stageStateChanged(git::Index::StagedState state);
  void discardSignal();

protected:
  void paintEvent(QPaintEvent *event);

private slots:
  /*!
   * Shows dialog if the changes should be discarded
   * \brief discardDialog
   * \param startLine
   * \param end
   */
  void discardDialog(int startLine, int end);
  void headerCheckStateChanged(int state);
  /*!
   * Stage/Unstage line with index lidx
   * \brief setStaged
   * \param lidx Line index
   * \param staged Staged if true, else unstaged
   */
  void setStaged(int lidx, bool staged, bool emitSignal = true);
  void marginClicked(int pos, int modifier, int margin);

private:
  void createMarkersAndLineNumbers(const Line &line, int lidx,
                                   Account::FileComments &comments,
                                   int width) const;

  /*!
   * \brief setEditorLineInfos
   * Setting marker, line numbers and staged icon to the lines
   */
  void setEditorLineInfos(QList<Line> &lines, Account::FileComments &comments,
                          int width);

  struct Token {
    int pos;
    QByteArray text;
  };

  int tokenEndPosition(int pos) const;

  QList<Token> tokens(int line) const;
  QByteArray tokenBuffer(const QList<Token> &tokens);
  void chooseLines(TextEditor::Marker kind);

  DiffView *mView;
  git::Patch mPatch;
  git::Patch mStaged;
  int mIndex;
  bool mLfs;

  _HunkWidget::Header *mHeader;
  TextEditor *mEditor;
  bool mLoaded{false};
  bool mLoading{false}; // during execution of the load() method
  bool mStagedStateLoaded{false};
  git::Index::StagedState mStagedStage;
};

#endif // HUNKWIDGET_H
