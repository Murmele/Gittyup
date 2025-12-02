//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "PlatQt.h"
#include "ScintillaQt.h"
#include "CaseFolder.h"
#ifdef SCI_LEXER
#include <LexerModule.h>
#include <ExternalLexer.h>
#endif

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMenu>
#include <QScrollBar>
#include <QTimer>
#include <QTextFormat>

#define MAXLENINPUTIME 200
#define SC_INDICATOR_INPUT INDIC_IME
#define SC_INDICATOR_TARGET INDIC_IME + 1
#define SC_INDICATOR_CONVERTED INDIC_IME + 2
#define SC_INDICATOR_UNKNOWN INDIC_IME_MAX

namespace Scintilla {

namespace {

#if defined(Q_OS_WIN)
const QString sMSDEVColumnSelect = "MSDEVColumnSelect";
const QString sWrappedMSDEVColumnSelect =
    "application/x-qt-windows-mime;value=\"MSDEVColumnSelect\"";
#elif defined(Q_OS_MAC)
const QString sScintillaRecMimeType =
    "text/x-scintilla.utf16-plain-text.rectangular";
#else // Linux
const QString sMimeRectangularMarker = "text/x-rectangular-marker";
#endif

void AddRectangularToMime(QMimeData *mimeData, QString su) {
#if defined(Q_OS_WIN)
  // Add an empty marker
  mimeData->setData(sMSDEVColumnSelect, QByteArray());
#elif defined(Q_OS_MAC)
  // OS X gets marker + data to work with other implementations.
  // Don't understand how this works but it does - the
  // clipboard format is supposed to be UTF-16, not UTF-8.
  mimeData->setData(sScintillaRecMimeType, su.toUtf8());
#else
  Q_UNUSED(su);
  // Linux
  // Add an empty marker
  mimeData->setData(sMimeRectangularMarker, QByteArray());
#endif
}

bool IsRectangularInMime(const QMimeData *mimeData) {
  QStringList formats = mimeData->formats();
  for (int i = 0; i < formats.size(); ++i) {
#if defined(Q_OS_WIN)
    // Windows rectangular markers
    // If rectangular copies made by this application, see base name.
    if (formats[i] == sMSDEVColumnSelect)
      return true;
    // Otherwise see wrapped name.
    if (formats[i] == sWrappedMSDEVColumnSelect)
      return true;
#elif defined(Q_OS_MAC)
    if (formats[i] == sScintillaRecMimeType)
      return true;
#else
    // Linux
    if (formats[i] == sMimeRectangularMarker)
      return true;
#endif
  }

  return false;
}

bool IsHangul(const QChar qchar) {
  int unicode = (int)qchar.unicode();
  // Korean character ranges used for preedit chars.
  // http://www.programminginkorean.com/programming/hangul-in-unicode/
  const bool HangulJamo = (0x1100 <= unicode && unicode <= 0x11FF);
  const bool HangulCompatibleJamo = (0x3130 <= unicode && unicode <= 0x318F);
  const bool HangulJamoExtendedA = (0xA960 <= unicode && unicode <= 0xA97F);
  const bool HangulJamoExtendedB = (0xD7B0 <= unicode && unicode <= 0xD7FF);
  const bool HangulSyllable = (0xAC00 <= unicode && unicode <= 0xD7A3);
  return HangulJamo || HangulCompatibleJamo || HangulSyllable ||
         HangulJamoExtendedA || HangulJamoExtendedB;
}

} // namespace

ScintillaQt::ScintillaQt(QWidget *parent) : QAbstractScrollArea(parent) {
  timer.start();

  // Set Qt defaults.
  setAcceptDrops(true);
  setMouseTracking(true);
  setFrameStyle(QFrame::NoFrame);
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_StaticContents);
  setAttribute(Qt::WA_KeyCompression);
  setAttribute(Qt::WA_InputMethodEnabled);
  viewport()->setAutoFillBackground(false);
  viewport()->setAttribute(Qt::WA_OpaquePaintEvent);

  vs.indicators[SC_INDICATOR_UNKNOWN] =
      Indicator(INDIC_HIDDEN, ColourDesired(0, 0, 0xff));
  vs.indicators[SC_INDICATOR_INPUT] =
      Indicator(INDIC_DOTS, ColourDesired(0, 0, 0xff));
  vs.indicators[SC_INDICATOR_CONVERTED] =
      Indicator(INDIC_COMPOSITIONTHICK, ColourDesired(0, 0, 0xff));
  vs.indicators[SC_INDICATOR_TARGET] =
      Indicator(INDIC_STRAIGHTBOX, ColourDesired(0, 0, 0xff));

  wMain = viewport();

  imeInteraction = imeInline;

  send(SCI_SETBUFFEREDDRAW, false);
  send(SCI_SETCODEPAGE, SC_CP_UTF8);

  Initialise();
}

ScintillaQt::~ScintillaQt() { Finalise(); }

sptr_t ScintillaQt::send(unsigned int iMessage, uptr_t wParam,
                         sptr_t lParam) const {
  return const_cast<ScintillaQt *>(this)->WndProc(iMessage, wParam, lParam);
}

bool ScintillaQt::event(QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    // Circumvent the tab focus convention.
    keyPressEvent(static_cast<QKeyEvent *>(event));
    return event->isAccepted();
  }

  return QAbstractScrollArea::event(event);
}

void ScintillaQt::timerEvent(QTimerEvent *event) {
  for (int tr = TickReason::caret; tr <= TickReason::dwell; ++tr) {
    if (timers.at(tr) == event->timerId())
      TickFor(static_cast<TickReason>(tr));
  }
}

void ScintillaQt::paintEvent(QPaintEvent *event) {
  rcPaint = PRectFromQRect(event->rect());
  paintState = painting;
  PRectangle rcClient = GetClientRectangle();
  paintingAllText = rcPaint.Contains(rcClient);

  AutoSurface surface(this);
  Paint(surface, rcPaint);
  surface->Release();

  if (paintState == paintAbandoned) {
    // FIXME: Failure to paint the requested rectangle in each
    // paint event causes flicker on some platforms (macOS?)
    // Paint rect immediately.
    paintState = painting;
    paintingAllText = true;

    AutoSurface surface(this);
    Paint(surface, rcPaint);
    surface->Release();

    // Queue a full repaint.
    viewport()->update();
  }

  paintState = notPainting;
}

void ScintillaQt::wheelEvent(QWheelEvent *event) {
  if (event->angleDelta().x() != 0) {
    if (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff)
      event->ignore();
    else
      QAbstractScrollArea::wheelEvent(event);
  } else {
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
      // Zoom! We play with the font sizes in the styles.
      // Number of steps/line is ignored, we just care if sizing up or down
      if (event->angleDelta().y() > 0) {
        KeyCommand(SCI_ZOOMIN);
      } else {
        KeyCommand(SCI_ZOOMOUT);
      }
    } else {
      // Ignore wheel events when the scroll bars are disabled.
      if (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff) {
        event->ignore();
      } else {
        // Scroll
        QAbstractScrollArea::wheelEvent(event);
      }
    }
  }
}

void ScintillaQt::focusInEvent(QFocusEvent *event) {
  SetFocusState(true);
  emit updateUi();

  QAbstractScrollArea::focusInEvent(event);
}

void ScintillaQt::focusOutEvent(QFocusEvent *event) {
  SetFocusState(false);

  QAbstractScrollArea::focusOutEvent(event);
}

void ScintillaQt::resizeEvent(QResizeEvent *) { ChangeSize(); }

void ScintillaQt::keyPressEvent(QKeyEvent *event) {
  // All keystrokes containing the meta modifier are
  // assumed to be shortcuts not handled by scintilla.
  if (QApplication::keyboardModifiers() & Qt::MetaModifier) {
    QAbstractScrollArea::keyPressEvent(event);
    return;
  }

  int key = 0;
  switch (event->key()) {
    case Qt::Key_Down:
      key = SCK_DOWN;
      break;
    case Qt::Key_Up:
      key = SCK_UP;
      break;
    case Qt::Key_Left:
      key = SCK_LEFT;
      break;
    case Qt::Key_Right:
      key = SCK_RIGHT;
      break;
    case Qt::Key_Home:
      key = SCK_HOME;
      break;
    case Qt::Key_End:
      key = SCK_END;
      break;
    case Qt::Key_PageUp:
      key = SCK_PRIOR;
      break;
    case Qt::Key_PageDown:
      key = SCK_NEXT;
      break;
    case Qt::Key_Delete:
      key = SCK_DELETE;
      break;
    case Qt::Key_Insert:
      key = SCK_INSERT;
      break;
    case Qt::Key_Escape:
      key = SCK_ESCAPE;
      break;
    case Qt::Key_Backspace:
      key = SCK_BACK;
      break;
    case Qt::Key_Plus:
      key = SCK_ADD;
      break;
    case Qt::Key_Minus:
      key = SCK_SUBTRACT;
      break;
    case Qt::Key_Backtab: // fall through
    case Qt::Key_Tab:
      key = SCK_TAB;
      break;
    case Qt::Key_Enter: // fall through
    case Qt::Key_Return:
      key = SCK_RETURN;
      break;
    case Qt::Key_Control:
      key = 0;
      break;
    case Qt::Key_Alt:
      key = 0;
      break;
    case Qt::Key_Shift:
      key = 0;
      break;
    case Qt::Key_Meta:
      key = 0;
      break;
    default:
      key = event->key();
      break;
  }

  bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
  bool ctrl = QApplication::keyboardModifiers() & Qt::ControlModifier;
  bool alt = QApplication::keyboardModifiers() & Qt::AltModifier;

  bool consumed = false;
  bool added =
      KeyDownWithModifiers(key, ModifierFlags(shift, ctrl, alt), &consumed);
  if (!consumed)
    consumed = added;

  if (!consumed) {
    // Don't insert text if the control key was pressed unless
    // it was pressed in conjunction with alt for AltGr emulation.
    bool input = (!ctrl || alt);

    // Additionally, on non-macOS platforms, don't insert text
    // if the alt key was pressed unless control is also present.
    // On macOS alt can be used to insert special characters.
#ifndef Q_OS_MAC
    input &= (!alt || ctrl);
#endif

    QString text = event->text();
    if (input && !text.isEmpty() && text[0].isPrint()) {
      QByteArray utext = text.toUtf8();
      InsertCharacter(std::string_view(utext.data(), utext.size()),
                      CharacterSource::directInput);
    } else {
      event->ignore();
    }
  }
}

#ifdef Q_WS_X11
static int modifierTranslated(int sciModifier) {
  switch (sciModifier) {
    case SCMOD_SHIFT:
      return Qt::ShiftModifier;
    case SCMOD_CTRL:
      return Qt::ControlModifier;
    case SCMOD_ALT:
      return Qt::AltModifier;
    case SCMOD_SUPER:
      return Qt::MetaModifier;
    default:
      return 0;
  }
}
#endif

void ScintillaQt::mousePressEvent(QMouseEvent *event) {
  Point pos = PointFromQPoint(event->pos());

  if (event->button() == Qt::MiddleButton &&
      QApplication::clipboard()->supportsSelection()) {
    SelectionPosition selPos =
        SPositionFromLocation(pos, false, false, UserVirtualSpace());
    sel.Clear();
    SetSelection(selPos, selPos);
    PasteFromMode(QClipboard::Selection);
    return;
  }

  if (event->button() == Qt::LeftButton) {
    Qt::KeyboardModifiers modifiers = event->modifiers();
    bool shift = modifiers & Qt::ShiftModifier;
    bool ctrl = modifiers & Qt::ControlModifier;
#ifdef Q_WS_X11
    // On X allow choice of rectangular modifier since most window
    // managers grab alt + click for moving windows.
    bool alt = modifiers & modifierTranslated(rectangularSelectionModifier);
#else
    bool alt = modifiers & Qt::AltModifier;
#endif

    ButtonDownWithModifiers(pos, timer.elapsed(),
                            ModifierFlags(shift, ctrl, alt));
  }
}

void ScintillaQt::mouseReleaseEvent(QMouseEvent *event) {
  Point point = PointFromQPoint(event->pos());
  bool ctrl = QApplication::keyboardModifiers() & Qt::ControlModifier;
  if (event->button() == Qt::LeftButton)
    ButtonUpWithModifiers(point, timer.elapsed(),
                          ModifierFlags(false, ctrl, false));

  int pos = send(SCI_POSITIONFROMPOINT, point.x, point.y);
  int line = send(SCI_LINEFROMPOSITION, pos);
  int modifiers = QApplication::keyboardModifiers();

  emit textAreaClicked(line, modifiers);
}

void ScintillaQt::mouseDoubleClickEvent(QMouseEvent *event) {
  // Scintilla does its own double-click detection.
  mousePressEvent(event);
}

void ScintillaQt::mouseMoveEvent(QMouseEvent *event) {
  Point pos = PointFromQPoint(event->pos());

  bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
  bool ctrl = QApplication::keyboardModifiers() & Qt::ControlModifier;
#ifdef Q_WS_X11
  // On X allow choice of rectangular modifier since most window
  // managers grab alt + click for moving windows.
  bool alt = QApplication::keyboardModifiers() &
             modifierTranslated(rectangularSelectionModifier);
#else
  bool alt = QApplication::keyboardModifiers() & Qt::AltModifier;
#endif

  ButtonMoveWithModifiers(pos, timer.elapsed(),
                          ModifierFlags(shift, ctrl, alt));
}

void ScintillaQt::contextMenuEvent(QContextMenuEvent *event) {
  Point pos = PointFromQPoint(event->globalPos());
  Point pt = PointFromQPoint(event->pos());
  if (!PointInSelection(pt))
    SetEmptySelection(PositionFromLocation(pt));
  ContextMenu(pos);
}

void ScintillaQt::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasText()) {
    event->acceptProposedAction();

    Point point = PointFromQPoint(event->position().toPoint());
    SetDragPosition(
        SPositionFromLocation(point, false, false, UserVirtualSpace()));
  } else {
    event->ignore();
  }
}

void ScintillaQt::dragLeaveEvent(QDragLeaveEvent *event) {
  Q_UNUSED(event);
  SetDragPosition(SelectionPosition(Sci::invalidPosition));
}

void ScintillaQt::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasText()) {
    event->acceptProposedAction();

    Point point = PointFromQPoint(event->position().toPoint());
    SetDragPosition(
        SPositionFromLocation(point, false, false, UserVirtualSpace()));
  } else {
    event->ignore();
  }
}

void ScintillaQt::dropEvent(QDropEvent *event) {
  if (event->mimeData()->hasText()) {
    event->acceptProposedAction();

    const QMimeData *data = event->mimeData();
    Point point = PointFromQPoint(event->position().toPoint());
    bool move =
        (event->source() == this && event->proposedAction() == Qt::MoveAction);

    SelectionPosition pos =
        SPositionFromLocation(point, false, false, UserVirtualSpace());
    QByteArray bytes = data->text().toUtf8();
    DropAt(pos, bytes, bytes.length(), move, IsRectangularInMime(data));
  } else {
    event->ignore();
  }
}

void ScintillaQt::MoveImeCarets(int offset) {
  // Move carets relatively by bytes
  for (size_t r = 0; r < sel.Count(); r++) {
    int positionInsert = sel.Range(r).Start().Position();
    sel.Range(r).caret.SetPosition(positionInsert + offset);
    sel.Range(r).anchor.SetPosition(positionInsert + offset);
  }
}

void ScintillaQt::DrawImeIndicator(int indicator, int len) {
  // Emulate the visual style of IME characters with indicators.
  // Draw an indicator on the character before caret by the character bytes of
  // len so it should be called after AddCharUTF(). It does not affect caret
  // positions.
  if (indicator < 8 || indicator > INDIC_MAX) {
    return;
  }
  pdoc->decorations->SetCurrentIndicator(indicator);
  for (size_t r = 0; r < sel.Count(); ++r) {
    int positionInsert = sel.Range(r).Start().Position();
    pdoc->DecorationFillRange(positionInsert - len, 1, len);
  }
}

void ScintillaQt::inputMethodEvent(QInputMethodEvent *event) {
  // Copy & paste by johnsonj with a lot of helps of Neil
  // Great thanks for my forerunners, jiniya and BLUEnLIVE

  bool initialCompose = false;
  if (pdoc->TentativeActive()) {
    pdoc->TentativeUndo();
  } else {
    // No tentative undo means start of this composition so
    // Fill in any virtual spaces.
    initialCompose = true;
  }

  view.imeCaretBlockOverride = false;

  if (!event->commitString().isEmpty()) {
    const QString commitStr = event->commitString();
    const unsigned int commitStrLen = commitStr.length();

    for (unsigned int i = 0; i < commitStrLen;) {
      const unsigned int ucWidth = commitStr.at(i).isHighSurrogate() ? 2 : 1;
      const QString oneCharUTF16 = commitStr.mid(i, ucWidth);
      const QByteArray oneChar = oneCharUTF16.toUtf8();
      const int oneCharLen = oneChar.length();
      InsertCharacter(std::string_view(oneChar.data(), oneCharLen),
                      CharacterSource::directInput);
      i += ucWidth;
    }

  } else if (!event->preeditString().isEmpty()) {
    const QString preeditStr = event->preeditString();
    const unsigned int preeditStrLen = preeditStr.length();
    if ((preeditStrLen == 0) || (preeditStrLen > MAXLENINPUTIME)) {
      ShowCaretAtCurrentPosition();
      return;
    }

    if (initialCompose)
      ClearBeforeTentativeStart();
    pdoc->TentativeStart(); // TentativeActive() from now on.

    // Mark segments and get ime caret position.
    unsigned int imeCaretPos = 0;
    unsigned int imeIndicator[MAXLENINPUTIME] = {0};
#ifdef Q_OS_LINUX
    // ibus-qt has a bug to return only one underline style.
    // Q_OS_LINUX blocks are temporary work around to cope with it.
    unsigned int attrSegment = 0;
#endif

    foreach (QInputMethodEvent::Attribute attr, event->attributes()) {
      if (attr.type == QInputMethodEvent::TextFormat) {
        QTextFormat format = attr.value.value<QTextFormat>();
        QTextCharFormat charFormat = format.toCharFormat();

        unsigned int indicator = SC_INDICATOR_UNKNOWN;
        switch (charFormat.underlineStyle()) {
          case QTextCharFormat::NoUnderline:
            indicator = SC_INDICATOR_TARGET; // target input
            break;
          case QTextCharFormat::SingleUnderline:
          case QTextCharFormat::DashUnderline:
            indicator = SC_INDICATOR_INPUT; // normal input
            break;
          case QTextCharFormat::DotLine:
          case QTextCharFormat::DashDotLine:
          case QTextCharFormat::WaveUnderline:
          case QTextCharFormat::SpellCheckUnderline:
            indicator = SC_INDICATOR_CONVERTED;
            break;

          case QTextCharFormat::DashDotDotLine:
            indicator = SC_INDICATOR_UNKNOWN;
        }

#ifdef Q_OS_LINUX
        attrSegment++;
        indicator = attr.start;
#endif
        for (int i = attr.start; i < attr.start + attr.length; i++) {
          imeIndicator[i] = indicator;
        }
      } else if (attr.type == QInputMethodEvent::Cursor) {
        imeCaretPos = attr.start;
      }
    }
#ifdef Q_OS_LINUX
    const bool targetInput =
        (attrSegment > 1) ||
        ((imeCaretPos == 0) && (preeditStr != preeditString));
    preeditString = preeditStr;
#endif
    // Display preedit characters one by one.
    int imeCharPos[MAXLENINPUTIME] = {0};
    int numBytes = 0;

    const bool recording = recordingMacro;
    recordingMacro = false;
    for (unsigned int i = 0; i < preeditStrLen;) {
      const unsigned int ucWidth = preeditStr.at(i).isHighSurrogate() ? 2 : 1;
      const QString oneCharUTF16 = preeditStr.mid(i, ucWidth);
      const QByteArray oneChar = oneCharUTF16.toUtf8();
      const int oneCharLen = oneChar.length();

      // Record character positions for moving ime caret.
      numBytes += oneCharLen;
      imeCharPos[i + 1] = numBytes;

      InsertCharacter(std::string_view(oneChar.data(), oneCharLen),
                      CharacterSource::directInput);

#ifdef Q_OS_LINUX
      // Segment marked with imeCaretPos is for target input.
      if ((imeIndicator[i] == imeCaretPos) && (targetInput)) {
        DrawImeIndicator(SC_INDICATOR_TARGET, oneCharLen);
      } else {
        DrawImeIndicator(SC_INDICATOR_INPUT, oneCharLen);
      }
#else
      DrawImeIndicator(imeIndicator[i], oneCharLen);
#endif
      i += ucWidth;
    }
    recordingMacro = recording;

    // Move IME carets.
    if (IsHangul(preeditStr.at(0))) {
      view.imeCaretBlockOverride = true;
      MoveImeCarets(-imeCharPos[preeditStrLen]);
    } else {
      MoveImeCarets(-imeCharPos[preeditStrLen] + imeCharPos[imeCaretPos]);
    }

    // Set candidate box position for Qt::ImCursorRectangle.
    preeditPos = CurrentPosition();
    EnsureCaretVisible();
    updateMicroFocus();
  }
  ShowCaretAtCurrentPosition();
}

QVariant ScintillaQt::inputMethodQuery(Qt::InputMethodQuery query) const {
  int pos = send(SCI_GETCURRENTPOS);
  int line = send(SCI_LINEFROMPOSITION, pos);

  switch (query) {
    case Qt::ImCursorRectangle: {
      int startPos = (preeditPos >= 0) ? preeditPos : pos;
      Point pt =
          const_cast<ScintillaQt *>(this)->LocationFromPosition(startPos);
      int width = send(SCI_GETCARETWIDTH);
      int height = send(SCI_TEXTHEIGHT, line);
      return QRect(pt.x, pt.y, width, height);
    }

    case Qt::ImFont: {
      char fontName[64];
      int style = send(SCI_GETSTYLEAT, pos);
      int len = send(SCI_STYLEGETFONT, style, (sptr_t)fontName);
      int size = send(SCI_STYLEGETSIZE, style);
      bool italic = send(SCI_STYLEGETITALIC, style);
      int weight = send(SCI_STYLEGETBOLD, style) ? QFont::Bold : -1;
      return QFont(QString::fromUtf8(fontName, len), size, weight, italic);
    }

    case Qt::ImCursorPosition: {
      int paraStart = pdoc->ParaUp(pos);
      return pos - paraStart;
    }

    case Qt::ImSurroundingText: {
      int paraStart = pdoc->ParaUp(pos);
      int paraEnd = pdoc->ParaDown(pos);
      QVarLengthArray<char, 1024> buffer(paraEnd - paraStart + 1);

      Sci_CharacterRange charRange;
      charRange.cpMin = paraStart;
      charRange.cpMax = paraEnd;

      Sci_TextRange textRange;
      textRange.chrg = charRange;
      textRange.lpstrText = buffer.data();

      send(SCI_GETTEXTRANGE, 0, (sptr_t)&textRange);

      return buffer.constData();
    }

    case Qt::ImCurrentSelection: {
      QVarLengthArray<char, 1024> buffer(send(SCI_GETSELTEXT));
      send(SCI_GETSELTEXT, 0, (sptr_t)buffer.data());

      return buffer.constData();
    }

    case Qt::ImEnabled:                // fall through
    case Qt::ImAnchorPosition:         // fall through
    case Qt::ImHints:                  // fall through
    case Qt::ImMaximumTextLength:      // fall through
    case Qt::ImPreferredLanguage:      // fall through
    case Qt::ImAbsolutePosition:       // fall through
    case Qt::ImTextBeforeCursor:       // fall through
    case Qt::ImTextAfterCursor:        // fall through
    case Qt::ImEnterKeyType:           // fall through
    case Qt::ImAnchorRectangle:        // fall through
    case Qt::ImInputItemClipRectangle: // fall through
    case Qt::ImPlatformData:           // fall through
    case Qt::ImQueryInput:             // fall through
    case Qt::ImQueryAll:               // fall through
    case Qt::ImReadOnly:               // fall through
      break;
  }
  return QVariant();
}

void ScintillaQt::PasteFromMode(QClipboard::Mode clipboardMode) {
  QClipboard *clipboard = QApplication::clipboard();
  const QMimeData *mimeData = clipboard->mimeData(clipboardMode);
  bool isRectangular = IsRectangularInMime(mimeData);
  QString text = clipboard->text(clipboardMode);
  QByteArray utext = text.toUtf8();
  std::string dest(utext.constData(), utext.length());
  SelectionText selText;
  int characterSet = vs.styles[STYLE_DEFAULT].characterSet;
  selText.Copy(dest, pdoc->dbcsCodePage, characterSet, isRectangular, false);

  UndoGroup ug(pdoc);
  Q_UNUSED(ug);

  ClearSelection(multiPasteMode == SC_MULTIPASTE_EACH);
  InsertPasteShape(selText.Data(), static_cast<int>(selText.Length()),
                   selText.rectangular ? pasteRectangular : pasteStream);
  EnsureCaretVisible();
}

void ScintillaQt::CopyToModeClipboard(const SelectionText &selectedText,
                                      QClipboard::Mode clipboardMode) {
  QClipboard *clipboard = QApplication::clipboard();
  clipboard->clear(clipboardMode);

  QMimeData *mimeData = new QMimeData;
  QString text = QString::fromUtf8(selectedText.Data(), selectedText.Length());
  mimeData->setText(text);
  if (selectedText.rectangular)
    AddRectangularToMime(mimeData, text);

  // Allow client code to add additional data (e.g rich text).
  emit aboutToCopy(mimeData);

  clipboard->setMimeData(mimeData, clipboardMode);
}

void ScintillaQt::Initialise() {
  // Connect scroll bars.
  QScrollBar *vsb = verticalScrollBar();
  connect(vsb, &QScrollBar::valueChanged, this,
          [this](int pos) { ScrollTo(pos); });

  QScrollBar *hsb = horizontalScrollBar();
  connect(hsb, &QScrollBar::valueChanged, this,
          [this](int pos) { HorizontalScrollTo(pos); });

  // Connect clipboard.
  QClipboard *clipboard = QApplication::clipboard();
  connect(clipboard, &QClipboard::selectionChanged, this, [this] {
    bool nowPrimary = QApplication::clipboard()->ownsSelection();
    if (nowPrimary != primarySelection) {
      primarySelection = nowPrimary;
      Redraw();
    }
  });

  for (int tr = tickCaret; tr <= tickDwell; ++tr)
    timers.at(tr) = 0;
}

void ScintillaQt::Finalise() {
  for (int tr = tickCaret; tr <= tickDwell; ++tr)
    FineTickerCancel(static_cast<TickReason>(tr));
  ScintillaBase::Finalise();
}

bool ScintillaQt::DragThreshold(Internal::Point ptStart, Internal::Point ptNow) {
  return (std::abs(ptStart.x - ptNow.x) > QApplication::startDragDistance()) ||
         (std::abs(ptStart.y - ptNow.y) > QApplication::startDragDistance());
}

bool ScintillaQt::ValidCodePage(int codePage) const {
  return (codePage == SC_CP_UTF8);
}

void ScintillaQt::ScrollText(Sci::Line linesToMove) {
  viewport()->scroll(0, vs.lineHeight * linesToMove);
}

void ScintillaQt::SetVerticalScrollPos() {
  verticalScrollBar()->setValue(topLine);
}

void ScintillaQt::SetHorizontalScrollPos() {
  horizontalScrollBar()->setValue(xOffset);
}

bool ScintillaQt::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) {
  bool modified = false;

  int vNewPage = nPage;
  int vNewMax = nMax - vNewPage + 1;
  if (vMax != vNewMax || vPage != vNewPage) {
    vMax = vNewMax;
    vPage = vNewPage;
    modified = true;

    verticalScrollBar()->setMaximum(vMax);
    verticalScrollBar()->setPageStep(vPage);
  }

  int hNewPage = GetTextRectangle().Width();
  int hNewMax = (scrollWidth > hNewPage) ? scrollWidth - hNewPage : 0;
  int charWidth = vs.styles[STYLE_DEFAULT].aveCharWidth;
  if (hMax != hNewMax || hPage != hNewPage ||
      horizontalScrollBar()->singleStep() != charWidth) {
    hMax = hNewMax;
    hPage = hNewPage;
    modified = true;

    horizontalScrollBar()->setMaximum(hMax);
    horizontalScrollBar()->setPageStep(hPage);
    horizontalScrollBar()->setSingleStep(charWidth);
  }

  return modified;
}

void ScintillaQt::ReconfigureScrollBars() {
  setVerticalScrollBarPolicy(verticalScrollBarVisible ? Qt::ScrollBarAsNeeded
                                                      : Qt::ScrollBarAlwaysOff);

  setHorizontalScrollBarPolicy(horizontalScrollBarVisible && !Wrapping()
                                   ? Qt::ScrollBarAsNeeded
                                   : Qt::ScrollBarAlwaysOff);
}

void ScintillaQt::Copy() {
  if (sel.Empty())
    return;

  SelectionText st;
  CopySelectionRange(&st);
  CopyToClipboard(st);
}

void ScintillaQt::CopyToClipboard(const SelectionText &selectedText) {
  CopyToModeClipboard(selectedText, QClipboard::Clipboard);
}

void ScintillaQt::Paste() { PasteFromMode(QClipboard::Clipboard); }

void ScintillaQt::ClaimSelection() {
  if (!QApplication::clipboard()->supportsSelection())
    return;

  // X Windows has a 'primary selection' as well as the clipboard.
  // Whenever the user selects some text, we become the primary selection
  if (!sel.Empty()) {
    primarySelection = true;
    SelectionText st;
    CopySelectionRange(&st);
    CopyToModeClipboard(st, QClipboard::Selection);
  } else {
    primarySelection = false;
  }
}

void ScintillaQt::NotifyChange() {}

void ScintillaQt::NotifyParent(NotificationData scn) {
  switch (scn.nmhdr.code) {
    case SCN_STYLENEEDED:
      emit styleNeeded(scn.position);
      break;

    case SCN_CHARADDED:
      emit charAdded(scn.ch);
      break;

    case SCN_SAVEPOINTREACHED:
      emit savePointChanged(false);
      break;

    case SCN_SAVEPOINTLEFT:
      emit savePointChanged(true);
      break;

    case SCN_MODIFYATTEMPTRO:
      emit modifyAttemptReadOnly();
      break;

    case SCN_KEY:
      emit key(scn.ch);
      break;

    case SCN_DOUBLECLICK:
      emit doubleClick(scn.position, scn.line);
      break;

    case SCN_UPDATEUI:
      emit updateUi();
      break;

    case SCN_MODIFIED: {
      bool added = scn.modificationType & SC_MOD_INSERTTEXT;
      bool deleted = scn.modificationType & SC_MOD_DELETETEXT;

      int length = send(SCI_GETTEXTLENGTH);
      bool firstLineAdded = (added && length == 1) || (deleted && length == 0);

      if (scn.linesAdded != 0) {
        emit linesAdded(scn.linesAdded);
      } else if (firstLineAdded) {
        emit linesAdded(added ? 1 : -1);
      }

      const QByteArray bytes = QByteArray::fromRawData(
          scn.text, (scn.text != NULL ? scn.length : 0));
      emit modified(scn.modificationType, scn.position, scn.length,
                    scn.linesAdded, bytes, scn.line, scn.foldLevelNow,
                    scn.foldLevelPrev);
      break;
    }

    case SCN_MACRORECORD:
      emit macroRecord(scn.message, scn.wParam, scn.lParam);
      break;

    case SCN_MARGINCLICK:
      emit marginClicked(scn.position, scn.modifiers, scn.margin);
      break;

    case SCN_NEEDSHOWN:
      emit needShown(scn.position, scn.length);
      break;

    case SCN_PAINTED:
      emit painted();
      break;

    case SCN_USERLISTSELECTION:
      emit userListSelection();
      break;

    case SCN_URIDROPPED:
      emit uriDropped();
      break;

    case SCN_DWELLSTART:
      emit dwellStart(scn.x, scn.y);
      break;

    case SCN_DWELLEND:
      emit dwellEnd(scn.x, scn.y);
      break;

    case SCN_ZOOM:
      emit zoom(send(SCI_GETZOOM));
      break;

    case SCN_HOTSPOTCLICK:
      emit hotSpotClick(scn.position, scn.modifiers);
      break;

    case SCN_HOTSPOTDOUBLECLICK:
      emit hotSpotDoubleClick(scn.position, scn.modifiers);
      break;

    case SCN_CALLTIPCLICK:
      emit callTipClick();
      break;

    case SCN_AUTOCSELECTION:
      emit autoCompleteSelection(scn.lParam, QString::fromUtf8(scn.text));
      break;

    case SCN_AUTOCCANCELLED:
      emit autoCompleteCancelled();
      break;

    default:
      return;
  }
}

bool ScintillaQt::FineTickerRunning(TickReason reason) {
  return timers[reason] != 0;
}

void ScintillaQt::FineTickerStart(TickReason reason, int millis,
                                  int tolerance) {
  FineTickerCancel(reason);
  timers[reason] = startTimer(millis);
}

void ScintillaQt::FineTickerCancel(TickReason reason) {
  if (timers[reason]) {
    killTimer(timers[reason]);
    timers[reason] = 0;
  }
}

bool ScintillaQt::SetIdle(bool on) {
  if (on) {
    // Start idler, if it's not running.
    if (!idler.state) {
      idler.state = true;
      QTimer *timer = new QTimer;
      connect(timer, &QTimer::timeout, this, [this] {
        if (!Idle())
          SetIdle(false);
      });
      timer->start(0);
      idler.idlerID = timer;
    }
  } else {
    // Stop idler, if it's running
    if (idler.state) {
      idler.state = false;
      QTimer *timer = static_cast<QTimer *>(idler.idlerID);
      timer->stop();
      disconnect(timer, &QTimer::timeout, nullptr, nullptr);
      delete timer;
      idler.idlerID = nullptr;
    }
  }

  return true;
}

std::unique_ptr<CaseFolder> ScintillaQt::CaseFolderForEncoding() {
  return new CaseFolderUnicode();
}

std::string ScintillaQt::CaseMapString(const std::string &s, CaseMapping caseMapping) {
  if (s.size() == 0 || caseMapping == CaseMapping::same)
    return s;

  std::string mapped(s.length() * maxExpansionCaseConversion, 0);
  mapped.resize(CaseConvertString(
      &mapped[0], mapped.length(), s.c_str(), s.length(),
      (caseMapping == CaseMapping::Upper) ? CaseConversionUpper : CaseConversionLower));

  return mapped;
}

void ScintillaQt::SetMouseCapture(bool on) {
  // This is handled automatically by Qt.
  if (mouseDownCaptures)
    haveMouseCapture = on;
}

bool ScintillaQt::HaveMouseCapture() { return haveMouseCapture; }

void ScintillaQt::StartDrag() {
  inDragDrop = ddDragging;
  dropWentOutside = true;
  if (drag.Length()) {
    QMimeData *mimeData = new QMimeData;
    QString text = QString::fromUtf8(drag.Data(), drag.Length());
    mimeData->setText(text);
    if (drag.rectangular)
      AddRectangularToMime(mimeData, text);

    // This QDrag is not freed as that causes a crash on Linux.
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
    if ((dropAction == Qt::MoveAction) && dropWentOutside)
      ClearSelection(); // Remove dragged out text.
  }

  inDragDrop = ddNone;
  SetDragPosition(SelectionPosition(Sci::invalidPosition));
}

void ScintillaQt::CreateCallTipWindow(PRectangle rc) {
  if (ct.wCallTip.Created())
    return;

  QWidget *callTip = new QWidget(0, Qt::ToolTip);
  ct.wCallTip = callTip;
  callTip->move(rc.left, rc.top);
  callTip->resize(rc.Width(), rc.Height());
}

void ScintillaQt::AddToPopUp(const char *label, int cmd, bool enabled) {
  QMenu *menu = static_cast<QMenu *>(popup.GetID());

  if (!qstrlen(label)) {
    menu->addSeparator();
  } else {
    QAction *action = menu->addAction(label);
    action->setData(cmd);
    action->setEnabled(enabled);
  }

  // Make sure the menu's signal is connected only once.
  menu->disconnect();
  connect(menu, &QMenu::triggered, this,
          [this](QAction *action) { Command(action->data().toInt()); });
}

sptr_t ScintillaQt::WndProc(Message message, uptr_t wParam,
                            sptr_t lParam) {
  switch (message) {
    case SCI_SETIMEINTERACTION:
      // Only inline IME supported on Qt.
      break;

    case SCI_GRABFOCUS:
      setFocus(Qt::OtherFocusReason);
      break;

    case SCI_GETDIRECTFUNCTION:
      return reinterpret_cast<sptr_t>(DirectFunction);

    case SCI_GETDIRECTPOINTER:
      return reinterpret_cast<sptr_t>(this);

    default:
      return ScintillaBase::WndProc(message, wParam, lParam);
  }

  return 0;
}

sptr_t ScintillaQt::DefWndProc(Message, uptr_t, sptr_t) { return 0; }

sptr_t ScintillaQt::DirectFunction(sptr_t ptr, unsigned int iMessage,
                                   uptr_t wParam, sptr_t lParam) {
  return reinterpret_cast<ScintillaQt *>(ptr)->WndProc(iMessage, wParam,
                                                       lParam);
}

} // namespace Scintilla
