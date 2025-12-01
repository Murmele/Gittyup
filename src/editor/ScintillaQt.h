//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SCINTILLAQT_H
#define SCINTILLAQT_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include <Scintilla.h>
#include <ScintillaBase.h>
#include <Platform.h>
#include <Geometry.h>
#include <ILexer.h>
#include <Position.h>
#include <SplitVector.h>
#include <Partitioning.h>
#include <RunStyles.h>
#include <ContractionState.h>
#include <CellBuffer.h>
#include <CallTip.h>
#include <KeyMap.h>
#include <Indicator.h>
#include <XPM.h>
#include <LineMarker.h>
#include <Style.h>
#include <AutoComplete.h>
#include <UniqueString.h>
#include <ViewStyle.h>
#include <CharClassify.h>
#include <Decoration.h>
#include <CaseFolder.h>
#include <ILoader.h>
#include <CharacterCategory.h>
#include <Document.h>
#include <Selection.h>
#include <PositionCache.h>
#include <EditModel.h>
#include <MarginView.h>
#include <EditView.h>
#include <Editor.h>
#include <ScintillaBase.h>
#include <CaseConvert.h>

#ifdef SCI_LEXER
#include "SciLexer.h"
#include "PropSetSimple.h"
#endif

#include <QAbstractScrollArea>
#include <QAction>
#include <QClipboard>
#include <QElapsedTimer>
#include <QPaintEvent>

namespace Scintilla::Internal {
class SelectionText;
class CaseFolder;
}

namespace Scintilla {

class ScintillaQt : public QAbstractScrollArea, public Internal::ScintillaBase {
  Q_OBJECT

public:
  ScintillaQt(QWidget *parent = nullptr);
  virtual ~ScintillaQt();

  sptr_t send(unsigned int iMessage, uptr_t wParam = 0,
              sptr_t lParam = 0) const;

signals:
  // Clients can use this hook to add additional
  // formats (e.g. rich text) to the MIME data.
  void aboutToCopy(QMimeData *data);

  // Scintilla Notifications
  void linesAdded(int linesAdded);
  void styleNeeded(int position);
  void charAdded(int ch);
  void savePointChanged(bool dirty);
  void modifyAttemptReadOnly();
  void key(int key);
  void doubleClick(int position, int line);
  void updateUi();
  void modified(int type, int position, int length, int linesAdded,
                const QByteArray &text, int line, int foldNow, int foldPrev);
  void macroRecord(int message, uptr_t wParam, sptr_t lParam);
  void marginClicked(int position, int modifiers, int margin);
  void textAreaClicked(int line, int modifiers);
  void needShown(int position, int length);
  void painted();
  void userListSelection(); // Wants some args.
  void uriDropped();        // Wants some args.
  void dwellStart(int x, int y);
  void dwellEnd(int x, int y);
  void zoom(int zoom);
  void hotSpotClick(int position, int modifiers);
  void hotSpotDoubleClick(int position, int modifiers);
  void callTipClick();
  void autoCompleteSelection(int position, const QString &text);
  void autoCompleteCancelled();

protected:
  bool event(QEvent *event) override;
  void timerEvent(QTimerEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void inputMethodEvent(QInputMethodEvent *event) override;
  QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
  void scrollContentsBy(int dx, int dy) override {}
  static sptr_t DirectFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam,
                               sptr_t lParam);
  sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) override;

private:
  void PasteFromMode(QClipboard::Mode);
  void CopyToModeClipboard(const Internal::SelectionText &, QClipboard::Mode);

  void MoveImeCarets(int offset);
  void DrawImeIndicator(int indicator, int len);

  void Initialise() override;
  void Finalise() override;
  bool DragThreshold(Point ptStart, Point ptNow) override;
  bool ValidCodePage(int codePage) const override;
  void ScrollText(Sci::Line linesToMove) override;
  void SetVerticalScrollPos() override;
  void SetHorizontalScrollPos() override;
  bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) override;
  void ReconfigureScrollBars() override;
  void Copy() override;
  void CopyToClipboard(const Internal::SelectionText &selectedText) override;
  void Paste() override;
  void ClaimSelection() override;
  void NotifyChange() override;
  void NotifyParent(SCNotification scn) override;
  bool FineTickerRunning(TickReason reason) override;
  void FineTickerStart(TickReason reason, int millis, int tolerance) override;
  void FineTickerCancel(TickReason reason) override;
  bool SetIdle(bool on) override;
  void SetMouseCapture(bool on) override;
  bool HaveMouseCapture() override;
  void StartDrag() override;
  Internal::CaseFolder *CaseFolderForEncoding() override;
  std::string CaseMapString(const std::string &s, int caseMapping) override;
  void CreateCallTipWindow(Internal::PRectangle rc) override;
  void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) override;
  sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam,
                    sptr_t lParam) override;

private:
  QElapsedTimer timer;

  int preeditPos = -1;
  QString preeditString;

  int timers[tickDwell + 1];

  int vMax = 0, hMax = 0;   // Scroll bar maximums.
  int vPage = 0, hPage = 0; // Scroll bar page sizes.

  bool haveMouseCapture = false;
};

} // namespace Scintilla

#endif
