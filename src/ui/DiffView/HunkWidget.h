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
    class Header : public QFrame
    {
        Q_OBJECT
    public:
      Header(
        const git::Diff &diff,
        const git::Patch &patch,
        int index,
        bool lfs,
        bool submodule,
        QWidget *parent = nullptr);
      QCheckBox *check() const;

      DisclosureButton* button() const;

      QToolButton *saveButton() const;
      QToolButton *undoButton() const;
      QToolButton *oursButton() const;
      QToolButton *theirsButton() const;

    public slots:
      void stageStateChanged(int stageState);

    protected:
      void mouseDoubleClickEvent(QMouseEvent *event) override;

    signals:
      void stageStageChanged(int stageState);
      void discard();

    private:
      QCheckBox *mCheck;
      DisclosureButton *mButton;
      QToolButton *mSave = nullptr;
      QToolButton *mUndo = nullptr;
      QToolButton *mOurs = nullptr;
      QToolButton *mTheirs = nullptr;
    };
}

/*!
 * Represents one hunk of a patch of a file.
 * \brief The HunkWidget class
 */
class HunkWidget : public QFrame
{
  Q_OBJECT

public:
  HunkWidget(
    DiffView *view,
    const git::Diff &diff,
    const git::Patch &patch,
    const git::Patch &staged,
    int index,
    bool lfs,
    bool submodule,
    QWidget *parent = nullptr);
  _HunkWidget::Header *header() const;
  TextEditor *editor(bool ensureLoaded = true);
  ~HunkWidget();
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
  QByteArray apply() const;
  git::Index::StagedState stageState();
  /*!
   * Stage/Unstage all
   * \brief setStaged
   * \param staged
   */
  void setStaged(bool staged);
  /*!
   * Called by the hunk header
   * \brief discard
   */
  void discard();
  /*!
   * update hunk content
   * \brief load
   * \param force Set to true to force reloading
   */
  void load(git::Patch &staged, bool force = false);
  /*!
   * Determines if the line is staged or not. The result is written into the \p staged variable
   * \brief findStagedLines
   * \param lines Lines of the diff patch (staged and unstaged)
   * \param additions Number of additions until the current line. This variable is increased in this function if it is an addition.
   * \param lidx Index of the current line
   * \param offset Line offset between the diff patch and the staged patch, because a patch contains only 3 lines before and after the changes.
   * \param linesStaged Lines of the staged patch
   * \param stagedAdditions Number of additions which are staged until current line. This variable is increased in this function if it is staged.
   * \param staged Determines if the line is staged or not
   */
  static void findStagedLines(QList<Line>& lines, int& additions, int lidx, int offset, QList<Line> &linesStaged, int &stagedAdditions, bool &staged);
  /*!
   * Determines the line offset between the lines in the patch and the staged Lines
   * \brief determineLineOffset
   * \param lines Lines of the diff patch (staged and unstaged)
   * \param stagedLines Lines of the staged patch
   * \return Offset between the lines in the patch and in the staged Lines. If no staged Lines
   * are available, -1 is returned
   */
  static int determineLineOffset(QList<Line> &lines, QList<Line> &stagedLines);

signals:
  /*!
   * It is not possible to stage single hunks.
   * So the complete file must be staged. Inform the FileWidget
   * about changes and it will perform a stage
   * \brief stageStageChanged
   * \param stageState
   */
  void stageStageChanged(int stageState);
  void discardSignal();

protected:
  void paintEvent(QPaintEvent *event);

private slots:
  void stageSelected(int startLine, int end);
  void unstageSelected(int startLine, int end);
  void discardSelected(int startLine, int end);
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
  void setStaged(int lidx, bool staged);
  void marginClicked(int pos, int modifier, int margin);

private:
  void createMarkersAndLineNumbers(const Line& line, int lidx, Account::FileComments& comments, int width) const;
  void findMatchingLines(QList<Line> &lines, int lidx, int count, int& marker, int &countDiffLines, int& additions, int& deletions, bool& staged) const;
  struct Token
  {
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

  _HunkWidget::Header *mHeader;
  TextEditor *mEditor;
  bool mLoaded{false};
  bool mLoading{false}; // during execution of the load() method
  bool mStagedStateLoaded{false};
  git::Index::StagedState mStagedStage;
};

#endif // HUNKWIDGET_H
