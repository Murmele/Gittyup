//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TextEditor.h"
#include <LexerModule.h>
#include <Scintillua.h>
#include "app/Application.h"
#include "conf/Settings.h"
#include <QFocusEvent>
#include <QMainWindow>
#include <QScrollBar>
#include <QStyle>
#include <QWindow>
#include <QMenu>
#include <QCheckBox>

#include "PlatQt.h"
#include "ui/HotkeyManager.h"
#include <QRegularExpression>
#include <QVariantMap>

using namespace Scintilla;

namespace {

QPixmap stagedUnstagedIcon(const bool &checked, const QColor &background,
                           const int &fontHeight) {
  // Set background color and checkbox size.
  QString checkBoxStyle = "QCheckBox {"
                          "  background: %1;"
                          "}"
                          "QCheckBox::indicator {"
                          "  height: %2;"
                          "  width: %2;"
                          "}";

  QCheckBox checkBox;
  checkBox.setChecked(checked);
  checkBox.setStyleSheet(
      checkBoxStyle.arg(background.name()).arg(fontHeight - 2));
  return checkBox.grab(
      QRect(QPoint(0, 0), QSize(fontHeight - 2, fontHeight - 2)));
}

Scintilla::Colour ToScintillaColour(const QColor &color) {
  return color.red() | (color.green() << 8) | (color.blue() << 16) |
         (color.alpha() << 24);
}

// Expand '$(key)' references against the theme's flat property map. Entries
// may reference other entries (e.g. style.constant = '$(style.keyword)'), so
// repeat until nothing changes, bounded to avoid a cycle looping forever.
// NOTE: This is AI generated code
QString expandThemeMacro(QString value, const QVariantMap &props) {
  static const QRegularExpression macro(R"(\$\(([^)]+)\))");
  for (int i = 0; i < 16; ++i) {
    QRegularExpressionMatch match = macro.match(value);
    if (!match.hasMatch())
      break;
    value.replace(match.capturedStart(), match.capturedLength(),
                  props.value(match.captured(1)).toString());
  }
  return value;
}

// Apply a SciTE/Scintillua-style spec string, e.g. "fore:#0000FF,bold", to a
// numbered style.
// NOTE: This is AI generated code
void applyThemeStyleSpec(ScintillaEdit *editor, int style,
                         const QString &spec) {
  foreach (QString token, spec.split(',', Qt::SkipEmptyParts)) {
    token = token.trimmed();
    int colon = token.indexOf(':');
    if (colon < 0) {
      QString keyword = token.toLower();
      if (keyword == "bold") {
        editor->styleSetBold(style, true);
      } else if (keyword == "italics" || keyword == "italic") {
        editor->styleSetItalic(style, true);
      } else if (keyword == "underline" || keyword == "underlined") {
        editor->styleSetUnderline(style, true);
      } else if (keyword == "notvisible") {
        editor->styleSetVisible(style, false);
      } else if (keyword == "eolfilled") {
        editor->styleSetEOLFilled(style, true);
      }
      continue;
    }

    QString key = token.left(colon).trimmed().toLower();
    QColor color(token.mid(colon + 1).trimmed());
    if (!color.isValid())
      continue;

    if (key == "fore") {
      editor->styleSetFore(style, ToScintillaColour(color));
    } else if (key == "back") {
      editor->styleSetBack(style, ToScintillaColour(color));
    }
  }
}

static Hotkey stage = HotkeyManager::registerHotkey(
    "s", "stage selected changes", "DiffView/Stage Selected Lines");

static Hotkey unstage = HotkeyManager::registerHotkey(
    "u", "unstage selected changes", "DiffView/Unstage Selected Lines");

static Hotkey discard = HotkeyManager::registerHotkey(
    "r", "discard selected changes", "DiffView/Discard Selected Changes");

} // namespace

static bool LuaAdded = false;

TextEditor::TextEditor(QWidget *parent) : ScintillaEdit(parent) {
  // Load colors.
  Theme *theme = Application::theme();
  mOursColor = theme->diff(Theme::Diff::Ours);
  mTheirsColor = theme->diff(Theme::Diff::Theirs);
  mAdditionColor = theme->diff(Theme::Diff::Addition);
  mDeletionColor = theme->diff(Theme::Diff::Deletion);

  // Load icons.
  QStyle *style = this->style();
  mNoteIcon = style->standardIcon(QStyle::SP_MessageBoxInformation);
  mWarningIcon = style->standardIcon(QStyle::SP_MessageBoxWarning);
  mErrorIcon = style->standardIcon(QStyle::SP_MessageBoxCritical);

  // Register the scintillua lexer library. This only needs to happen once
  // per process.
  if (!LuaAdded) {
    SetLibraryProperty("scintillua.lexers",
                       Settings::lexerDir().path().toStdString().c_str());
    LuaAdded = true;
  }

  // Every editor needs its own lexer instance: unlike a classic Scintilla
  // lexer, Scintillua's ILexer5 is stateful (it tracks the currently
  // detected language), so it can't be shared between editors.
  ILexer5 *lua = CreateLexer("lua");
  if (lua) {
    setILexer((sptr_t)lua);
  } else {
    qWarning() << "Error creating Lua lexer";
  }

  setScrollWidth(256);
  setScrollWidthTracking(true);
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

  setWrapMode(SC_WRAP_NONE);

  setMarginLeft(4);
  setMarginTypeN(Staged, SC_MARGIN_SYMBOL);
  setMarginTypeN(LineNumber, SC_MARGIN_NUMBER);
  setMarginTypeN(LineNumbers, SC_MARGIN_TEXT);
  setMarginTypeN(ErrorMargin, SC_MARGIN_SYMBOL);
  for (int i = 0; i <= SC_MAX_MARGIN; ++i) {
    setMarginWidthN(i, 0);
    setMarginMaskN(i, 0);
  }

  setMarginMaskN(Margin::Staged, (1 << StagedMarker) | (1 << UnstagedMarker));
  setStatusDiff(mStatusDiff); // to apply margin width

  int mask = 0;
  for (int i = NoteMarker; i <= ErrorMarker; ++i)
    mask |= (1 << i);
  setMarginMaskN(ErrorMargin, mask);
  setMarginSensitiveN(ErrorMargin, true);

  setSelEOLFilled(true);
  setSelBack(true, ToScintillaColour(palette().color(QPalette::Highlight)));
  setVirtualSpaceOptions(SCVS_RECTANGULARSELECTION);

  // Unset default zoom in/out shortcuts.
  clearCmdKey(SCK_ADD + (static_cast<int>(Scintilla::KeyMod::Ctrl) << 16));
  clearCmdKey(SCK_SUBTRACT + (static_cast<int>(Scintilla::KeyMod::Ctrl) << 16));

  // Set find indicators.
  indicSetStyle(FindAll, INDIC_STRAIGHTBOX);
  indicSetFore(FindAll, Qt::white);
  indicSetAlpha(FindAll, 255);
  indicSetUnder(FindAll, true);

  indicSetStyle(FindCurrent, INDIC_STRAIGHTBOX);
  indicSetFore(FindCurrent, Qt::yellow);
  indicSetAlpha(FindCurrent, 255);
  indicSetUnder(FindCurrent, true);

  // Set word diff indicators.
  indicSetFore(WordAddition,
               ToScintillaColour(theme->diff(Theme::Diff::WordAddition)));
  indicSetAlpha(WordAddition, 255);
  indicSetUnder(WordAddition, true);

  indicSetFore(WordDeletion,
               ToScintillaColour(theme->diff(Theme::Diff::WordDeletion)));
  indicSetAlpha(WordDeletion, 255);
  indicSetUnder(WordDeletion, true);

  indicSetFore(NoteIndicator,
               ToScintillaColour(theme->diff(Theme::Diff::Note)));
  indicSetAlpha(NoteIndicator, 255);
  indicSetUnder(NoteIndicator, true);

  indicSetFore(WarningIndicator,
               ToScintillaColour(theme->diff(Theme::Diff::Warning)));
  indicSetAlpha(WarningIndicator, 255);
  indicSetUnder(WarningIndicator, true);

  indicSetFore(ErrorIndicator,
               ToScintillaColour(theme->diff(Theme::Diff::Error)));
  indicSetAlpha(ErrorIndicator, 255);
  indicSetUnder(ErrorIndicator, true);

  // Initialize LPeg lexer.
  QColor text = palette().color(QPalette::Text);
  setCaretFore(ToScintillaColour(text));

  // Apply default settings.
  applySettings();
  connect(Settings::instance(), &Settings::settingsChanged, this,
          &TextEditor::applySettings);

  // Update geometry when the scroll bar becomes visible.
  connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this,
          &TextEditor::updateGeometry);
}

void TextEditor::applySettings() {
  // Set default font and size.
  Settings *settings = Settings::instance();
  QString family = settings->value(Setting::Id::FontFamily).toString();
  int pointSize = settings->value(Setting::Id::FontSize).toInt();
  StyleSetQFont(STYLE_DEFAULT, QFont(family, pointSize));

  setUseTabs(settings->value(Setting::Id::UseTabsForIndent).toBool());
  setIndent(settings->value(Setting::Id::IndentWidth).toInt());
  setTabWidth(settings->value(Setting::Id::TabWidth).toInt());
  setViewWS(settings->value(Setting::Id::ShowWhitespaceInEditor).toBool());

  if (Settings::instance()->isTextEditorWrapLines()) {
    setWrapMode(SC_WRAP_WORD);
  } else {
    setWrapMode(SC_WRAP_NONE);
  }

  // Initialize markers.
  QColor background = palette().color(QPalette::Base);
  int fontHeight = textHeight(0);
  setStatusDiff(mStatusDiff); // to apply margin width
  mStagedIcon = stagedUnstagedIcon(true, background, fontHeight);
  mUnStagedIcon = stagedUnstagedIcon(false, background, fontHeight);
  if (mStatusDiff)
    setMarginWidthN(Staged, fontHeight);

  // used to colorize the background of the text
  markerDefine(Context, SC_MARK_EMPTY);
  markerDefine(Ours, SC_MARK_BACKGROUND);
  markerDefine(Theirs, SC_MARK_BACKGROUND);
  markerDefine(Addition, SC_MARK_BACKGROUND);
  markerDefine(Deletion, SC_MARK_BACKGROUND);
  markerDefine(StagedMarker, SC_MARK_RGBAIMAGE);
  markerDefine(UnstagedMarker, SC_MARK_RGBAIMAGE);
  markerDefine(DiscardMarker, SC_MARK_EMPTY);

  markerSetBack(Ours, ToScintillaColour(mOursColor));
  markerSetBack(Theirs, ToScintillaColour(mTheirsColor));
  markerSetBack(Addition, ToScintillaColour(mAdditionColor));
  markerSetBack(Deletion, ToScintillaColour(mDeletionColor));

  // Initialize error markers.
  loadMarkerIcon(NoteMarker, mNoteIcon);
  loadMarkerIcon(WarningMarker, mWarningIcon);
  loadMarkerIcon(ErrorMarker, mErrorIcon);

  loadMarkerPixmap(StagedMarker, mStagedIcon);
  loadMarkerPixmap(UnstagedMarker, mUnStagedIcon);

  // Set LPeg lexer language.
  QByteArray lexer = this->lexer().toUtf8();
  uintptr_t ptr = reinterpret_cast<uintptr_t>(lexer.constData());
  privateLexerCall(SCLUA_DETECT, ptr);

  // Re-apply theme colors: the set of named styles is lexer-dependent and
  // may have just changed.
  applyLexerStyles();

  // Set annotation styles.
  QFont regular = font();
  QFont bold = regular;
  bold.setBold(true);
  QFont italic = regular;
  italic.setItalic(true);

  // Missing newline style. Fore/back were never set, so it fell back to
  // Scintilla's raw internal default (white background regardless of theme).
  // Match STYLE_DEFAULT, which applyLexerStyles() already kept in sync with
  // the current theme, so it blends into the editor background.
  StyleSetQFont(EofNewline, italic);
  styleSetFore(EofNewline, styleFore(STYLE_DEFAULT));
  styleSetBack(EofNewline, styleBack(STYLE_DEFAULT));

  // Remote comment styles
  Theme *theme = Application::theme();
  StyleSetQFont(CommentBody, regular);
  styleSetFore(CommentBody,
               ToScintillaColour(theme->remoteComment(Theme::Comment::Body)));
  styleSetBack(CommentBody, ToScintillaColour(theme->remoteComment(
                                Theme::Comment::Background)));

  StyleSetQFont(CommentAuthor, bold);
  styleSetFore(CommentAuthor,
               ToScintillaColour(theme->remoteComment(Theme::Comment::Author)));
  styleSetBack(CommentAuthor, ToScintillaColour(theme->remoteComment(
                                  Theme::Comment::Background)));

  StyleSetQFont(CommentTimestamp, regular);
  styleSetFore(CommentTimestamp, ToScintillaColour(theme->remoteComment(
                                     Theme::Comment::Timestamp)));
  styleSetBack(CommentTimestamp, ToScintillaColour(theme->remoteComment(
                                     Theme::Comment::Background)));

  // Emit own signal.
  emit settingsChanged();

  // Size hint may have changed.
  updateGeometry();
}

// NOTE: This is AI generated code
void TextEditor::applyLexerStyles() {
  // Scintillua no longer has static style numbers or a built-in theme
  // loader: the lexer assigns style numbers dynamically (including the
  // predefined ones like STYLE_DEFAULT/STYLE_LINENUMBER), and it's up to the
  // application to map each one's name to a color via NamedStyles()/
  // NameOfStyle() and apply it itself.
  QVariantMap props = Application::theme()->editorStyleProperties();
  QString defaultSpec =
      expandThemeMacro(props.value("style.default").toString(), props);

  // STYLE_DEFAULT's font was already configured above from app settings.
  // Nothing sets it on the other named styles otherwise, so without this
  // they'd keep Scintilla's raw internal default font/size, producing a
  // visible font mismatch against the rest of the editor.
  QFont baseFont = styleGetQFont(STYLE_DEFAULT);

  int count = namedStyles();
  for (int style = 0; style < count; ++style) {
    // Establish font/fore/back as a baseline so styles the theme doesn't
    // mention explicitly (e.g. newer tag names) still look consistent
    // instead of falling back to Scintilla's raw internal default.
    StyleSetQFont(style, baseFont);
    if (!defaultSpec.isEmpty())
      applyThemeStyleSpec(this, style, defaultSpec);

    // Look up the most specific theme entry for this style's tag name,
    // falling back to progressively broader categories, e.g.
    // "constant.builtin" -> style.constantbuiltin, then style.constant.
    QString tag = QString::fromUtf8(nameOfStyle(style));
    while (!tag.isEmpty()) {
      QString key = "style." + QString(tag).remove('.');
      if (props.contains(key)) {
        QString spec = expandThemeMacro(props.value(key).toString(), props);
        if (!spec.isEmpty())
          applyThemeStyleSpec(this, style, spec);
        break;
      }

      int lastDot = tag.lastIndexOf('.');
      if (lastDot < 0)
        break;
      tag = tag.left(lastDot);
    }
  }
}

QString TextEditor::lexer() const { return Settings::instance()->lexer(mPath); }

void TextEditor::setLineCount(int lines) { mLineCount = lines; }

void TextEditor::setLexer(const QString &path) {
  mPath = path;
  applySettings();
}

void TextEditor::load(const QString &path, const QString &text) {
  setScrollWidth(256);
  setLexer(path);
  setText(text.toStdString().c_str());

  // Clear undo.
  setSavePoint();
  emptyUndoBuffer();

  // Notify layout of size change.
  updateGeometry();
}

void TextEditor::setStatusDiff(bool statusDiff) {
  mStatusDiff = statusDiff;
  if (mStatusDiff) {
    setMarginWidthN(Staged, textHeight(0));
    setMarginSensitiveN(Staged,
                        true); // to change by mouseclick staged/unstaged
  } else {
    setMarginWidthN(Staged, 0);
    setMarginSensitiveN(Staged, false);
  }
}

void TextEditor::clearHighlights() {
  setIndicatorCurrent(FindAll);
  indicatorClearRange(0, length());

  setIndicatorCurrent(FindCurrent);
  indicatorClearRange(0, length());

  // Restore styles.
  applySettings();

  emit highlightActivated(false);
}

int TextEditor::highlightAll(const QString &text) {

  clearHighlights();
  if (text.isEmpty())
    return 0;

  // Darken styles.
  markerSetBack(Addition, ToScintillaColour(mAdditionColor.darker(120)));
  markerSetBack(Deletion, ToScintillaColour(mDeletionColor.darker(120)));
  markerSetBack(Ours, ToScintillaColour(mOursColor.darker(120)));
  markerSetBack(Theirs, ToScintillaColour(mTheirsColor.darker(120)));
  for (int i = 0; i <= STYLE_DEFAULT; i++) {
    Scintilla::Colour c = styleBack(i);
    styleSetBack(i, ToScintillaColour(QColor(c).darker(120)));
  }

  emit highlightActivated(true);

  setIndicatorCurrent(FindAll);

  QByteArray utf8 = text.toUtf8();
  const char *data = utf8.constData();

  int matches = 0;
  int max = length();
  QPair<int, int> match = findText(0, data, 0, max);
  while (match.first >= 0) {
    // Search again from the end of the last range.
    indicatorFillRange(match.first, match.second - match.first);
    match = findText(0, data, match.second, max);
    ++matches;
  }

  return matches;
}

int TextEditor::find(const QString &text, bool forward, bool indicator) {
  QByteArray utf8 = text.toUtf8();
  const char *data = utf8.constData();

  searchAnchor();
  int pos = forward ? searchNext(0, data) : searchPrev(0, data);

  if (pos >= 0)
    scrollCaret();

  if (indicator) {
    // Clear previous indicator.
    setIndicatorCurrent(FindCurrent);
    indicatorClearRange(0, length());

    if (pos >= 0) {
      int start = selectionStart();
      indicatorFillRange(start, selectionEnd() - start);
    }
  }

  return pos;
}

QList<TextEditor::Diagnostic> TextEditor::diagnostics(int line) {
  return mDiagnostics.value(line);
}

void TextEditor::addDiagnostic(int line, const Diagnostic &diag) {
  int marker;
  int indicator;
  switch (diag.kind) {
    case Note:
      marker = NoteMarker;
      indicator = NoteIndicator;
      break;

    case Warning:
      marker = WarningMarker;
      indicator = WarningIndicator;
      break;

    case Error:
      marker = ErrorMarker;
      indicator = ErrorIndicator;
      break;
  }

  // Add indictator.
  setIndicatorCurrent(indicator);
  indicatorFillRange(positionFromLine(line) + diag.range.pos, diag.range.len);

  // Add marker.
  int height = textHeight(line);
  setMarginWidthN(ErrorMargin, height + 4);

  int current = diagnosticMarker(line);
  if (current >= 0)
    markerDelete(line, current);
  markerAdd(line, qMax(marker, current));

  // Remember diagnostic.
  mDiagnostics[line].append(diag);

  // Signal addition.
  emit diagnosticAdded(line, diag);
}

/// @brief Custom context menu bypassing Scintilla
/// @param event
void TextEditor::contextMenuEvent(QContextMenuEvent *event) {
  // The following logic was present before porting to Scintilla 5.x. However
  // it's yet to be determined how this is triggered or used
  //  Point pt = PointFromQPoint(event->pos());
  //  if (!PointInSelection(pt))
  //    SetEmptySelection(PositionFromLocation(pt));

  int startLine = lineFromPosition(selectionStart());
  int end = lineFromPosition(selectionEnd()) + 1;
  int staged = 0;
  int diffLines = 0;
  for (int i = startLine; i < end; i++) {
    int mask = markerGet(i);
    if (mask & (1 << TextEditor::Marker::Addition |
                1 << TextEditor::Marker::Deletion)) {
      diffLines++;
      if (mask & 1 << TextEditor::Marker::StagedMarker)
        staged++;
    }
  }
  const bool writable = !readOnly();
  ScintillaDocument *pdoc = get_doc();
  mPopup.clear();
  AddToPopUp("Undo", Undo, writable && pdoc->can_undo());
  AddToPopUp("Redo", Redo, writable && pdoc->can_redo());
  AddToPopUp("");
  AddToPopUp("Cut", Cut, writable && !selectionEmpty());
  AddToPopUp("Copy", Copy, !selectionEmpty());
  AddToPopUp("Paste", Paste, writable && canPaste());
  AddToPopUp("Delete", Delete, writable && !selectionEmpty());
  if (mStatusDiff) {
    AddToPopUp("");
    AddToPopUp((QString("Stage selected\t") + stage.currentKeys().toString())
                   .toStdString()
                   .data(),
               StageSelected, diffLines - staged > 0);
    AddToPopUp(
        (QString("Unstage selected\t") + unstage.currentKeys().toString())
            .toStdString()
            .data(),
        UnstageSelected, staged > 0);
    AddToPopUp(
        (QString("Discard selected\t") + discard.currentKeys().toString())
            .toStdString()
            .data(),
        DiscardSelected, diffLines > 0);
  }
  AddToPopUp("");
  AddToPopUp("Select All", SelectAll);
  mPopup.exec(event->globalPos());
}

void TextEditor::StyleSetQFont(int style, const QFont &font) {
  styleSetFont(style, font.family().toStdString().c_str());
  styleSetSize(style, font.pointSize());
  styleSetBold(style, font.bold());
  styleSetItalic(style, font.italic());
}

void TextEditor::AddToPopUp(const QString &label, MenuAction cmd,
                            bool enabled) {
  if (label.length() == 0) {
    mPopup.addSeparator();
  } else {
    QAction *action = mPopup.addAction(label);
    action->setData(cmd);
    action->setEnabled(enabled);
  }

  // Make sure the menu's signal is connected only once.
  mPopup.disconnect();
  connect(&mPopup, &QMenu::triggered, this, [this](QAction *action) {
    Command(action->data().value<MenuAction>());
  });
}

void TextEditor::Command(MenuAction action) {

  switch (action) {
    case StageSelected: {
      int startLine = lineFromPosition(selectionStart());
      int end = lineFromPosition(selectionEnd()) + 1;
      emit stageSelectedSignal(startLine, end);
      break;
    }
    case UnstageSelected: {
      int startLine = lineFromPosition(selectionStart());
      int end = lineFromPosition(selectionEnd()) + 1;
      emit unstageSelectedSignal(startLine, end);
      break;
    }
    case DiscardSelected: {
      int startLine = lineFromPosition(selectionStart());
      int end = lineFromPosition(selectionEnd()) + 1;
      emit discardSelectedSignal(startLine, end);
      break;
    }
    case Undo:
      undo();
      break;
    case Redo:
      redo();
      break;
    case Cut:
      cut();
      break;
    case Copy:
      copy();
      break;
    case Paste:
      paste();
      break;
    case Delete:
      clear();
      break;
    case SelectAll:
      selectAll();
      break;
    default:
      break;
  }
}

QSize TextEditor::viewportSizeHint() const {
  // Return placeholder size if the content isn't loaded.
  QSize size = ScintillaEdit::viewportSizeHint();
  if (length() == 0 && mLineCount >= 0) {
    int height = const_cast<TextEditor *>(this)->textHeight(0);
    return QSize(size.width(), mLineCount * height);
  }

  // Total height is the y position of the start of the last line
  // plus the height of the line itself and any annotation lines.
  // Documents always have at least one line by definition.
  int line = lineCount() - 1;
  int lines = annotationLines(line) + 1;
  int height = const_cast<TextEditor *>(this)->textHeight(line);
  int y = const_cast<TextEditor *>(this)->pointFromPosition(length()).y();

  int scrollBarHeight = 0;
  if (scrollWidth() > width())
    scrollBarHeight = horizontalScrollBar()->height();

  return QSize(size.width(), y + (lines * height) + scrollBarHeight);
}

int TextEditor::diagnosticMarker(int line) {
  int marks = markerGet(line);
  if (marks & (1 << NoteMarker))
    return NoteMarker;

  if (marks & (1 << WarningMarker))
    return WarningMarker;

  if (marks & (1 << ErrorMarker))
    return ErrorMarker;

  return -1;
}

void TextEditor::loadMarkerPixmap(Marker marker, const QPixmap &pixmap) {
  qreal dpr = 1.0;
  if (QWidget *window = this->window()) {
    if (QWindow *handle = window->windowHandle())
      dpr = handle->devicePixelRatio();
  }

  qreal height = textHeight(0);
  qreal scaled = height * dpr;
  QPixmap scaledPixmap(pixmap);
  if (pixmap.height() > height) {
    // scale
    scaledPixmap = pixmap.scaled(scaled, scaled, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
  }

  markerDefineImage(marker, scaledPixmap.toImage());
}

void TextEditor::loadMarkerIcon(Marker marker, const QIcon &icon) {
  qreal dpr = 1.0;
  if (QWidget *window = this->window()) {
    if (QWindow *handle = window->windowHandle())
      dpr = handle->devicePixelRatio();
  }

  qreal height = textHeight(0);
  qreal scaled = height * dpr;

  QPixmap pixmap = icon.pixmap(height, height);
  pixmap.setDevicePixelRatio(dpr);
  QPixmap scaledPixmap = pixmap.scaled(scaled, scaled, Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
  markerDefineImage(marker, scaledPixmap.toImage());
}

void TextEditor::keyPressEvent(QKeyEvent *ke) {
  if (mStatusDiff && (ke->key() == Qt::Key_S || ke->key() == Qt::Key_U ||
                      ke->key() == Qt::Key_R)) {
    int startLine = lineFromPosition(selectionStart());
    int end = lineFromPosition(selectionEnd()) + 1;
    int staged = 0;
    int diffLines = 0;
    for (int i = startLine; i < end; i++) {
      int mask = markerGet(i);
      if (mask & (1 << TextEditor::Marker::Addition |
                  1 << TextEditor::Marker::Deletion)) {
        diffLines++;
        if (mask & 1 << TextEditor::Marker::StagedMarker)
          staged++;
      }
    }

    if (ke->key() == Qt::Key_S && diffLines - staged > 0) {
      int startLine = lineFromPosition(selectionStart());
      int end = lineFromPosition(selectionEnd()) + 1;
      emit stageSelectedSignal(startLine, end);
      return;
    } else if (ke->key() == Qt::Key_U && staged > 0) {
      int startLine = lineFromPosition(selectionStart());
      int end = lineFromPosition(selectionEnd()) + 1;
      emit unstageSelectedSignal(startLine, end);
      return;
    } else if (ke->key() == Qt::Key_R && diffLines > 0) {
      int startLine = lineFromPosition(selectionStart());
      int end = lineFromPosition(selectionEnd()) + 1;
      emit discardSelectedSignal(startLine, end);
      return;
    }
  }

  ScintillaEdit::keyPressEvent(ke);
}

QFont TextEditor::styleGetQFont(int style) {
  QByteArray fontName = styleFont(style);
  QFont font(QString::fromUtf8(fontName.constData(), fontName.length()));
  font.setPointSize(send(SCI_STYLEGETSIZE, style));
  font.setBold(send(SCI_STYLEGETBOLD, style));
  font.setItalic(send(SCI_STYLEGETITALIC, style));
  return font;
}

QPoint TextEditor::pointFromPosition(int pos) {
  return QPoint(pointXFromPosition(pos), pointYFromPosition(pos));
}

void TextEditor::markerDefineImage(int markerNumber, const QImage &image) {
  QImage argb = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  rGBAImageSetWidth(argb.width());
  rGBAImageSetHeight(argb.height());
  rGBAImageSetScale(argb.devicePixelRatio() * 100);
  markerDefineRGBAImage(markerNumber, (const char *)argb.bits());
}
