//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SCINTILLAIFACE_H
#define SCINTILLAIFACE_H

#include "ScintillaQt.h"

namespace Scintilla {

class ScintillaIFace : public ScintillaQt {
public:
  ScintillaIFace(QWidget *parent = nullptr);

  // The functions below are generated from Scintilla.iface.

  //++FuncDef
  /**
   * Add text to the document at current position.
   */
  void addText(const QString &text);
  /**
   * Add array of cells to document.
   */
  void addStyledText(int length, const QString &c);
  /**
   * Insert string at a position.
   */
  void insertText(int pos, const QString &text);
  /**
   * Change the text that is being inserted in response to SC_MOD_INSERTCHECK
   */
  void changeInsertion(const QString &text);
  /**
   * Delete all text in the document.
   */
  void clearAll();
  /**
   * Delete a range of text in the document.
   */
  void deleteRange(int pos, int deleteLength);
  /**
   * Set all style bytes to 0, remove all folding information.
   */
  void clearDocumentStyle();
  /**
   * Returns the number of bytes in the document.
   */
  int length() const;
  /**
   * Returns the character byte at the position.
   */
  int charAt(int pos) const;
  /**
   * Returns the position of the caret.
   */
  int currentPos() const;
  /**
   * Returns the position of the opposite end of the selection to the caret.
   */
  int anchor() const;
  /**
   * Returns the style byte at the position.
   */
  int styleAt(int pos) const;
  /**
   * Redoes the next action on the undo history.
   */
  void redo();
  /**
   * Choose between collecting actions into the undo
   * history and discarding them.
   */
  void setUndoCollection(bool collectUndo);
  /**
   * Select all the text in the document.
   */
  void selectAll();
  /**
   * Remember the current position in the undo history as the position
   * at which the document was saved.
   */
  void setSavePoint();
  /**
   * Retrieve a buffer of cells.
   * Returns the number of bytes in the buffer not including terminating NULs.
   */
  int styledText(Sci_TextRange *tr);
  /**
   * Are there any redoable actions in the undo history?
   */
  bool canRedo();
  /**
   * Retrieve the line number at which a particular marker is located.
   */
  int markerLineFromHandle(int handle);
  /**
   * Delete a marker.
   */
  void markerDeleteHandle(int handle);
  /**
   * Is undo history being collected?
   */
  bool undoCollection() const;
  /**
   * Are white space characters currently visible?
   * Returns one of SCWS_* constants.
   */
  int viewWS() const;
  /**
   * Make white space characters invisible, always visible or visible outside
   * indentation.
   */
  void setViewWS(int viewWS);
  /**
   * Find the position from a point within the window.
   */
  int positionFromPoint(int x, int y);
  /**
   * Find the position from a point within the window but return
   * INVALID_POSITION if not close to text.
   */
  int positionFromPointClose(int x, int y);
  /**
   * Set caret to start of a line and ensure it is visible.
   */
  void gotoLine(int line);
  /**
   * Set caret to a position and ensure it is visible.
   */
  void gotoPos(int pos);
  /**
   * Set the selection anchor to a position. The anchor is the opposite
   * end of the selection from the caret.
   */
  void setAnchor(int posAnchor);
  /**
   * Retrieve the text of the line containing the caret.
   * Returns the index of the caret on the line.
   * Result is NUL-terminated.
   */
  int curLine(int length, char *text);
  /**
   * Retrieve the position of the last correctly styled character.
   */
  int endStyled() const;
  /**
   * Convert all line endings in the document to one mode.
   */
  void convertEOLs(int eolMode);
  /**
   * Retrieve the current end of line mode - one of CRLF, CR, or LF.
   */
  int eOLMode() const;
  /**
   * Set the current end of line mode.
   */
  void setEOLMode(int eolMode);
  /**
   * Set the current styling position to pos.
   */
  void startStyling(int pos);
  /**
   * Change style from current styling position for length characters to a style
   * and move the current styling position to after this newly styled segment.
   */
  void setStyling(int length, int style);
  /**
   * Is drawing done first into a buffer or direct to the screen?
   */
  bool bufferedDraw() const;
  /**
   * If drawing is buffered then each line of text is drawn into a bitmap buffer
   * before drawing it to the screen to avoid flicker.
   */
  void setBufferedDraw(bool buffered);
  /**
   * Change the visible size of a tab to be a multiple of the width of a space
   * character.
   */
  void setTabWidth(int tabWidth);
  /**
   * Retrieve the visible size of a tab.
   */
  int tabWidth() const;
  /**
   * Clear explicit tabstops on a line.
   */
  void clearTabStops(int line);
  /**
   * Add an explicit tab stop for a line.
   */
  void addTabStop(int line, int x);
  /**
   * Find the next explicit tab stop position on a line after a position.
   */
  int nextTabStop(int line, int x);
  /**
   * Set the code page used to interpret the bytes of the document as
   * characters. The SC_CP_UTF8 value can be used to enter Unicode mode.
   */
  void setCodePage(int codePage);
  /**
   * Is the IME displayed in a winow or inline?
   */
  int iMEInteraction() const;
  /**
   * Choose to display the the IME in a winow or inline.
   */
  void setIMEInteraction(int imeInteraction);
  /**
   * Set the symbol used for a particular marker number.
   */
  void markerDefine(int markerNumber, int markerSymbol);
  /**
   * Set the foreground colour used for a particular marker number.
   */
  void markerSetFore(int markerNumber, QColor fore);
  /**
   * Set the background colour used for a particular marker number.
   */
  void markerSetBack(int markerNumber, QColor back);
  /**
   * Set the background colour used for a particular marker number when its
   * folding block is selected.
   */
  void markerSetBackSelected(int markerNumber, QColor back);
  /**
   * Enable/disable highlight for current folding bloc (smallest one that
   * contains the caret)
   */
  void markerEnableHighlight(bool enabled);
  /**
   * Add a marker to a line, returning an ID which can be used to find or delete
   * the marker.
   */
  int markerAdd(int line, int markerNumber);
  /**
   * Delete a marker from a line.
   */
  void markerDelete(int line, int markerNumber);
  /**
   * Delete all markers with a particular number from all lines.
   */
  void markerDeleteAll(int markerNumber);
  /**
   * Get a bit mask of all the markers set on a line.
   */
  int markers(int line);
  /**
   * Find the next line at or after lineStart that includes a marker in mask.
   * Return -1 when no more lines.
   */
  int markerNext(int lineStart, int markerMask);
  /**
   * Find the previous line before lineStart that includes a marker in mask.
   */
  int markerPrevious(int lineStart, int markerMask);
  /**
   * Define a marker from a pixmap.
   */
  void markerDefinePixmap(int markerNumber, const char *pixmap);
  /**
   * Add a set of markers to a line.
   */
  void markerAddSet(int line, int set);
  /**
   * Set the alpha used for a marker that is drawn in the text area, not the
   * margin.
   */
  void markerSetAlpha(int markerNumber, int alpha);
  /**
   * Set a margin to be either numeric or symbolic.
   */
  void setMarginTypeN(int margin, int marginType);
  /**
   * Retrieve the type of a margin.
   */
  int marginTypeN(int margin) const;
  /**
   * Set the width of a margin to a width expressed in pixels.
   */
  void setMarginWidthN(int margin, int pixelWidth);
  /**
   * Retrieve the width of a margin in pixels.
   */
  int marginWidthN(int margin) const;
  /**
   * Set a mask that determines which markers are displayed in a margin.
   */
  void setMarginMaskN(int margin, int mask);
  /**
   * Retrieve the marker mask of a margin.
   */
  int marginMaskN(int margin) const;
  /**
   * Make a margin sensitive or insensitive to mouse clicks.
   */
  void setMarginSensitiveN(int margin, bool sensitive);
  /**
   * Retrieve the mouse click sensitivity of a margin.
   */
  bool marginSensitiveN(int margin) const;
  /**
   * Set the cursor shown when the mouse is inside a margin.
   */
  void setMarginCursorN(int margin, int cursor);
  /**
   * Retrieve the cursor shown in a margin.
   */
  int marginCursorN(int margin) const;
  /**
   * Clear all the styles and make equivalent to the global default style.
   */
  void styleClearAll();
  /**
   * Set the foreground colour of a style.
   */
  void styleSetFore(int style, QColor fore);
  /**
   * Set the background colour of a style.
   */
  void styleSetBack(int style, QColor back);
  /**
   * Set the font of a style.
   */
  void styleSetFont(int style, const QFont &font);
  /**
   * Set a style to have its end of line filled or not.
   */
  void styleSetEOLFilled(int style, bool filled);
  /**
   * Reset the default style to its state at startup
   */
  void styleResetDefault();
  /**
   * Set a style to be underlined or not.
   */
  void styleSetUnderline(int style, bool underline);
  /**
   * Get the foreground colour of a style.
   */
  QColor styleFore(int style) const;
  /**
   * Get the background colour of a style.
   */
  QColor styleBack(int style) const;
  /**
   * Get the font of a style.
   */
  QFont styleFont(int style) const;
  /**
   * Get is a style to have its end of line filled or not.
   */
  bool styleEOLFilled(int style) const;
  /**
   * Get is a style underlined or not.
   */
  bool styleUnderline(int style) const;
  /**
   * Get is a style mixed case, or to force upper or lower case.
   */
  int styleCase(int style) const;
  /**
   * Get the character get of the font in a style.
   */
  int styleCharacterSet(int style) const;
  /**
   * Get is a style visible or not.
   */
  bool styleVisible(int style) const;
  /**
   * Get is a style changeable or not (read only).
   * Experimental feature, currently buggy.
   */
  bool styleChangeable(int style) const;
  /**
   * Get is a style a hotspot or not.
   */
  bool styleHotSpot(int style) const;
  /**
   * Set a style to be mixed case, or to force upper or lower case.
   */
  void styleSetCase(int style, int caseForce);
  /**
   * Set the size of characters of a style. Size is in points multiplied by 100.
   */
  void styleSetSizeFractional(int style, int caseForce);
  /**
   * Get the size of characters of a style in points multiplied by 100
   */
  int styleSizeFractional(int style) const;
  /**
   * Set the weight of characters of a style.
   */
  void styleSetWeight(int style, int weight);
  /**
   * Get the weight of characters of a style.
   */
  int styleWeight(int style) const;
  /**
   * Set the character set of the font in a style.
   */
  void styleSetCharacterSet(int style, int characterSet);
  /**
   * Set a style to be a hotspot or not.
   */
  void styleSetHotSpot(int style, bool hotspot);
  /**
   * Set the foreground colour of the main and additional selections and whether
   * to use this setting.
   */
  void setSelFore(bool useSetting, QColor fore);
  /**
   * Set the background colour of the main and additional selections and whether
   * to use this setting.
   */
  void setSelBack(bool useSetting, QColor back);
  /**
   * Get the alpha of the selection.
   */
  int selAlpha() const;
  /**
   * Set the alpha of the selection.
   */
  void setSelAlpha(int alpha);
  /**
   * Is the selection end of line filled?
   */
  bool selEOLFilled() const;
  /**
   * Set the selection to have its end of line filled or not.
   */
  void setSelEOLFilled(bool filled);
  /**
   * Set the foreground colour of the caret.
   */
  void setCaretFore(QColor fore);
  /**
   * When key+modifier combination km is pressed perform msg.
   */
  void assignCmdKey(int km, int msg);
  /**
   * When key+modifier combination km is pressed do nothing.
   */
  void clearCmdKey(int km);
  /**
   * Drop all key mappings.
   */
  void clearAllCmdKeys();
  /**
   * Set the styles for a segment of the document.
   */
  void setStylingEx(int length, const QString &styles);
  /**
   * Set a style to be visible or not.
   */
  void styleSetVisible(int style, bool visible);
  /**
   * Get the time in milliseconds that the caret is on and off.
   */
  int caretPeriod() const;
  /**
   * Get the time in milliseconds that the caret is on and off. 0 = steady on.
   */
  void setCaretPeriod(int periodMilliseconds);
  /**
   * Set the set of characters making up words for when moving or selecting by
   * word. First sets defaults like SetCharsDefault.
   */
  void setWordChars(const QByteArray &chars);
  /**
   * Get the set of characters making up words for when moving or selecting by
   * word. Returns the number of characters
   */
  QByteArray wordChars() const;
  /**
   * Start a sequence of actions that is undone and redone as a unit.
   * May be nested.
   */
  void beginUndoAction();
  /**
   * End a sequence of actions that is undone and redone as a unit.
   */
  void endUndoAction();
  /**
   * Set an indicator to plain, squiggle or TT.
   */
  void indicSetStyle(int indic, int style);
  /**
   * Retrieve the style of an indicator.
   */
  int indicStyle(int indic) const;
  /**
   * Set the foreground colour of an indicator.
   */
  void indicSetFore(int indic, QColor fore);
  /**
   * Retrieve the foreground colour of an indicator.
   */
  QColor indicFore(int indic) const;
  /**
   * Set an indicator to draw under text or over(default).
   */
  void indicSetUnder(int indic, bool under);
  /**
   * Retrieve whether indicator drawn under or over text.
   */
  bool indicUnder(int indic) const;
  /**
   * Set a hover indicator to plain, squiggle or TT.
   */
  void indicSetHoverStyle(int indic, int style);
  /**
   * Retrieve the hover style of an indicator.
   */
  int indicHoverStyle(int indic) const;
  /**
   * Set the foreground hover colour of an indicator.
   */
  void indicSetHoverFore(int indic, QColor fore);
  /**
   * Retrieve the foreground hover colour of an indicator.
   */
  QColor indicHoverFore(int indic) const;
  /**
   * Set the attributes of an indicator.
   */
  void indicSetFlags(int indic, int flags);
  /**
   * Retrieve the attributes of an indicator.
   */
  int indicFlags(int indic) const;
  /**
   * Set the foreground colour of all whitespace and whether to use this
   * setting.
   */
  void setWhitespaceFore(bool useSetting, QColor fore);
  /**
   * Set the background colour of all whitespace and whether to use this
   * setting.
   */
  void setWhitespaceBack(bool useSetting, QColor back);
  /**
   * Set the size of the dots used to mark space characters.
   */
  void setWhitespaceSize(int size);
  /**
   * Get the size of the dots used to mark space characters.
   */
  int whitespaceSize() const;
  /**
   * Used to hold extra styling information for each line.
   */
  void setLineState(int line, int state);
  /**
   * Retrieve the extra styling information for a line.
   */
  int lineState(int line) const;
  /**
   * Retrieve the last line number that has line state.
   */
  int maxLineState() const;
  /**
   * Is the background of the line containing the caret in a different colour?
   */
  bool caretLineVisible() const;
  /**
   * Display the background of the line containing the caret in a different
   * colour.
   */
  void setCaretLineVisible(bool show);
  /**
   * Get the colour of the background of the line containing the caret.
   */
  QColor caretLineBack() const;
  /**
   * Set the colour of the background of the line containing the caret.
   */
  void setCaretLineBack(QColor back);
  /**
   * Set a style to be changeable or not (read only).
   * Experimental feature, currently buggy.
   */
  void styleSetChangeable(int style, bool changeable);
  /**
   * Display a auto-completion list.
   * The lenEntered parameter indicates how many characters before
   * the caret should be used to provide context.
   */
  void autoCShow(int lenEntered, const QString &itemList);
  /**
   * Remove the auto-completion list from the screen.
   */
  void autoCCancel();
  /**
   * Is there an auto-completion list visible?
   */
  bool autoCActive();
  /**
   * Retrieve the position of the caret when the auto-completion list was
   * displayed.
   */
  int autoCPosStart();
  /**
   * User has selected an item so remove the list and insert the selection.
   */
  void autoCComplete();
  /**
   * Define a set of character that when typed cancel the auto-completion list.
   */
  void autoCStops(const QString &characterSet);
  /**
   * Change the separator character in the string setting up an auto-completion
   * list. Default is space but can be changed if items contain space.
   */
  void autoCSetSeparator(int separatorCharacter);
  /**
   * Retrieve the auto-completion list separator character.
   */
  int autoCSeparator() const;
  /**
   * Select the item in the auto-completion list that starts with a string.
   */
  void autoCSelect(const QString &text);
  /**
   * Should the auto-completion list be cancelled if the user backspaces to a
   * position before where the box was created.
   */
  void autoCSetCancelAtStart(bool cancel);
  /**
   * Retrieve whether auto-completion cancelled by backspacing before start.
   */
  bool autoCCancelAtStart() const;
  /**
   * Define a set of characters that when typed will cause the autocompletion to
   * choose the selected item.
   */
  void autoCSetFillUps(const QString &characterSet);
  /**
   * Should a single item auto-completion list automatically choose the item.
   */
  void autoCSetChooseSingle(bool chooseSingle);
  /**
   * Retrieve whether a single item auto-completion list automatically choose
   * the item.
   */
  bool autoCChooseSingle() const;
  /**
   * Set whether case is significant when performing auto-completion searches.
   */
  void autoCSetIgnoreCase(bool ignoreCase);
  /**
   * Retrieve state of ignore case flag.
   */
  bool autoCIgnoreCase() const;
  /**
   * Display a list of strings and send notification when user chooses one.
   */
  void userListShow(int listType, const QString &itemList);
  /**
   * Set whether or not autocompletion is hidden automatically when nothing
   * matches.
   */
  void autoCSetAutoHide(bool autoHide);
  /**
   * Retrieve whether or not autocompletion is hidden automatically when nothing
   * matches.
   */
  bool autoCAutoHide() const;
  /**
   * Set whether or not autocompletion deletes any word characters
   * after the inserted text upon completion.
   */
  void autoCSetDropRestOfWord(bool dropRestOfWord);
  /**
   * Retrieve whether or not autocompletion deletes any word characters
   * after the inserted text upon completion.
   */
  bool autoCDropRestOfWord() const;
  /**
   * Register an XPM image for use in autocompletion lists.
   */
  void registerImage(int type, const char *xpmData);
  /**
   * Clear all the registered XPM images.
   */
  void clearRegisteredImages();
  /**
   * Retrieve the auto-completion list type-separator character.
   */
  int autoCTypeSeparator() const;
  /**
   * Change the type-separator character in the string setting up an
   * auto-completion list. Default is '?' but can be changed if items contain
   * '?'.
   */
  void autoCSetTypeSeparator(int separatorCharacter);
  /**
   * Set the maximum width, in characters, of auto-completion and user lists.
   * Set to 0 to autosize to fit longest item, which is the default.
   */
  void autoCSetMaxWidth(int characterCount);
  /**
   * Get the maximum width, in characters, of auto-completion and user lists.
   */
  int autoCMaxWidth() const;
  /**
   * Set the maximum height, in rows, of auto-completion and user lists.
   * The default is 5 rows.
   */
  void autoCSetMaxHeight(int rowCount);
  /**
   * Set the maximum height, in rows, of auto-completion and user lists.
   */
  int autoCMaxHeight() const;
  /**
   * Set the number of spaces used for one level of indentation.
   */
  void setIndent(int indentSize);
  /**
   * Retrieve indentation size.
   */
  int indent() const;
  /**
   * Indentation will only use space characters if useTabs is false, otherwise
   * it will use a combination of tabs and spaces.
   */
  void setUseTabs(bool useTabs);
  /**
   * Retrieve whether tabs will be used in indentation.
   */
  bool useTabs() const;
  /**
   * Change the indentation of a line to a number of columns.
   */
  void setLineIndentation(int line, int indentSize);
  /**
   * Retrieve the number of columns that a line is indented.
   */
  int lineIndentation(int line) const;
  /**
   * Retrieve the position before the first non indentation character on a line.
   */
  int lineIndentPosition(int line) const;
  /**
   * Retrieve the column number of a position, taking tab width into account.
   */
  int column(int pos) const;
  /**
   * Count characters between two positions.
   */
  int countCharacters(int startPos, int endPos);
  /**
   * Show or hide the horizontal scroll bar.
   */
  void setHScrollBar(bool show);
  /**
   * Is the horizontal scroll bar visible?
   */
  bool hScrollBar() const;
  /**
   * Show or hide indentation guides.
   */
  void setIndentationGuides(int indentView);
  /**
   * Are the indentation guides visible?
   */
  int indentationGuides() const;
  /**
   * Set the highlighted indentation guide column.
   * 0 = no highlighted guide.
   */
  void setHighlightGuide(int column);
  /**
   * Get the highlighted indentation guide column.
   */
  int highlightGuide() const;
  /**
   * Get the position after the last visible characters on a line.
   */
  int lineEndPosition(int line) const;
  /**
   * Get the code page used to interpret the bytes of the document as
   * characters.
   */
  int codePage() const;
  /**
   * Get the foreground colour of the caret.
   */
  QColor caretFore() const;
  /**
   * In read-only mode?
   */
  bool readOnly() const;
  /**
   * Sets the position of the caret.
   */
  void setCurrentPos(int pos);
  /**
   * Sets the position that starts the selection - this becomes the anchor.
   */
  void setSelectionStart(int pos);
  /**
   * Returns the position at the start of the selection.
   */
  int selectionStart() const;
  /**
   * Sets the position that ends the selection - this becomes the
   * currentPosition.
   */
  void setSelectionEnd(int pos);
  /**
   * Returns the position at the end of the selection.
   */
  int selectionEnd() const;
  /**
   * Set caret to a position, while removing any existing selection.
   */
  void setEmptySelection(int pos);
  /**
   * Sets the print magnification added to the point size of each style for
   * printing.
   */
  void setPrintMagnification(int magnification);
  /**
   * Returns the print magnification.
   */
  int printMagnification() const;
  /**
   * Modify colours when printing for clearer printed text.
   */
  void setPrintColourMode(int mode);
  /**
   * Returns the print colour mode.
   */
  int printColourMode() const;
  /**
   * Find some text in the document.
   */
  QPair<int, int> findText(int flags, const char *text, int min, int max);
  /**
   * On Windows, will draw the document into a display context such as a
   * printer.
   */
  int formatRange(bool draw, Sci_RangeToFormat *fr);
  /**
   * Retrieve the display line at the top of the display.
   */
  int firstVisibleLine() const;
  /**
   * Returns the contents of a line.
   */
  QString line(int line);
  /**
   * Returns the number of lines in the document. There is always at least one.
   */
  int lineCount() const;
  /**
   * Sets the size in pixels of the left margin.
   */
  void setMarginLeft(int pixelWidth);
  /**
   * Returns the size in pixels of the left margin.
   */
  int marginLeft() const;
  /**
   * Sets the size in pixels of the right margin.
   */
  void setMarginRight(int pixelWidth);
  /**
   * Returns the size in pixels of the right margin.
   */
  int marginRight() const;
  /**
   * Is the document different from when it was last saved?
   */
  bool isModified() const;
  /**
   * Select a range of text.
   */
  void setSel(int start, int end);
  /**
   * Return the selected text.
   */
  QString selText();
  /**
   * Retrieve a range of text.
   * Return the length of the text.
   */
  QByteArray textRange(int min, int max) const;
  /**
   * Draw the selection in normal style or with selection highlighted.
   */
  void hideSelection(bool normal);
  /**
   * Retrieve the  point in the window where a position is displayed.
   */
  QPoint pointFromPosition(int pos);
  /**
   * Retrieve the line containing a position.
   */
  int lineFromPosition(int pos);
  /**
   * Retrieve the position at the start of a line.
   */
  int positionFromLine(int line);
  /**
   * Scroll horizontally and vertically.
   */
  void lineScroll(int columns, int lines);
  /**
   * Ensure the caret is visible.
   */
  void scrollCaret();
  /**
   * Scroll the argument positions and the range between them into view giving
   * priority to the primary position then the secondary position.
   * This may be used to make a search match visible.
   */
  void scrollRange(int secondary, int primary);
  /**
   * Replace the selected text with the argument text.
   */
  void replaceSelection(const QString &text);
  /**
   * Set to read only or read write.
   */
  void setReadOnly(bool readOnly);
  /**
   * Null operation.
   */
  void null();
  /**
   * Will a paste succeed?
   */
  bool canPaste();
  /**
   * Are there any undoable actions in the undo history?
   */
  bool canUndo();
  /**
   * Delete the undo history.
   */
  void emptyUndoBuffer();
  /**
   * Undo one action in the undo history.
   */
  void undo();
  /**
   * Cut the selection to the clipboard.
   */
  void cut();
  /**
   * Copy the selection to the clipboard.
   */
  void copy();
  /**
   * Paste the contents of the clipboard into the document replacing the
   * selection.
   */
  void paste();
  /**
   * Clear the selection.
   */
  void clear();
  /**
   * Replace the contents of the document with the argument text.
   */
  void setText(const QString &text);
  /**
   * Retrieve all the text in the document.
   */
  QString text() const;
  /**
   * Retrieve a pointer to a function that processes messages for this
   * Scintilla.
   */
  uintptr_t directFunction() const;
  /**
   * Retrieve a pointer value to use as the first argument when calling
   * the function returned by GetDirectFunction.
   */
  uintptr_t directPointer() const;
  /**
   * Set to overtype (true) or insert mode.
   */
  void setOvertype(bool overtype);
  /**
   * Returns true if overtype mode is active otherwise false is returned.
   */
  bool overtype() const;
  /**
   * Set the width of the insert mode caret.
   */
  void setCaretWidth(int pixelWidth);
  /**
   * Returns the width of the insert mode caret.
   */
  int caretWidth() const;
  /**
   * Sets the position that starts the target which is used for updating the
   * document without affecting the scroll position.
   */
  void setTargetStart(int pos);
  /**
   * Get the position that starts the target.
   */
  int targetStart() const;
  /**
   * Sets the position that ends the target which is used for updating the
   * document without affecting the scroll position.
   */
  void setTargetEnd(int pos);
  /**
   * Get the position that ends the target.
   */
  int targetEnd() const;
  /**
   * Sets both the start and end of the target in one call.
   */
  void setTargetRange(int start, int end);
  /**
   * Retrieve the text in the target.
   */
  int targetText(char *characters) const;
  /**
   * Make the target range start and end be the same as the selection range
   * start and end.
   */
  void targetFromSelection();
  /**
   * Sets the target to the whole document.
   */
  void targetWholeDocument();
  /**
   * Replace the target text with the argument text.
   * Text is counted so it can contain NULs.
   * Returns the length of the replacement text.
   */
  int replaceTarget(const QString &text);
  /**
   * Replace the target text with the argument text after \d processing.
   * Text is counted so it can contain NULs.
   * Looks for \d where d is between 1 and 9 and replaces these with the strings
   * matched in the last search operation which were surrounded by \( and \).
   * Returns the length of the replacement text including any change
   * caused by processing the \d patterns.
   */
  int replaceTargetRE(const QString &text);
  /**
   * Search for a counted string in the target and set the target to the found
   * range. Text is counted so it can contain NULs.
   * Returns length of range or -1 for failure in which case target is not
   * moved.
   */
  int searchInTarget(const QString &text);
  /**
   * Set the search flags used by SearchInTarget.
   */
  void setSearchFlags(int flags);
  /**
   * Get the search flags used by SearchInTarget.
   */
  int searchFlags() const;
  /**
   * Show a call tip containing a definition near position pos.
   */
  void callTipShow(int pos, const QString &definition);
  /**
   * Remove the call tip from the screen.
   */
  void callTipCancel();
  /**
   * Is there an active call tip?
   */
  bool callTipActive();
  /**
   * Retrieve the position where the caret was before displaying the call tip.
   */
  int callTipPosStart();
  /**
   * Set the start position in order to change when backspacing removes the
   * calltip.
   */
  void callTipSetPosStart(int posStart);
  /**
   * Highlight a segment of the definition.
   */
  void callTipSetHlt(int start, int end);
  /**
   * Set the background colour for the call tip.
   */
  void callTipSetBack(QColor back);
  /**
   * Set the foreground colour for the call tip.
   */
  void callTipSetFore(QColor fore);
  /**
   * Set the foreground colour for the highlighted part of the call tip.
   */
  void callTipSetForeHlt(QColor fore);
  /**
   * Enable use of STYLE_CALLTIP and set call tip tab size in pixels.
   */
  void callTipUseStyle(int tabSize);
  /**
   * Set position of calltip, above or below text.
   */
  void callTipSetPosition(bool above);
  /**
   * Find the display line of a document line taking hidden lines into account.
   */
  int visibleFromDocLine(int line);
  /**
   * Find the document line of a display line taking hidden lines into account.
   */
  int docLineFromVisible(int lineDisplay);
  /**
   * The number of display lines needed to wrap a document line
   */
  int wrapCount(int line);
  /**
   * Set the fold level of a line.
   * This encodes an integer level along with flags indicating whether the
   * line is a header and whether it is effectively white space.
   */
  void setFoldLevel(int line, int level);
  /**
   * Retrieve the fold level of a line.
   */
  int foldLevel(int line) const;
  /**
   * Find the last child line of a header line.
   */
  int lastChild(int line, int level) const;
  /**
   * Find the parent line of a child line.
   */
  int foldParent(int line) const;
  /**
   * Make a range of lines visible.
   */
  void showLines(int lineStart, int lineEnd);
  /**
   * Make a range of lines invisible.
   */
  void hideLines(int lineStart, int lineEnd);
  /**
   * Is a line visible?
   */
  bool lineVisible(int line) const;
  /**
   * Are all lines visible?
   */
  bool allLinesVisible() const;
  /**
   * Show the children of a header line.
   */
  void setFoldExpanded(int line, bool expanded);
  /**
   * Is a header line expanded?
   */
  bool foldExpanded(int line) const;
  /**
   * Switch a header line between expanded and contracted.
   */
  void toggleFold(int line);
  /**
   * Expand or contract a fold header.
   */
  void foldLine(int line, int action);
  /**
   * Expand or contract a fold header and its children.
   */
  void foldChildren(int line, int action);
  /**
   * Expand a fold header and all children. Use the level argument instead of
   * the line's current level.
   */
  void expandChildren(int line, int level);
  /**
   * Expand or contract all fold headers.
   */
  void foldAll(int action);
  /**
   * Ensure a particular line is visible by expanding any header line hiding it.
   */
  void ensureVisible(int line);
  /**
   * Set automatic folding behaviours.
   */
  void setAutomaticFold(int automaticFold);
  /**
   * Get automatic folding behaviours.
   */
  int automaticFold() const;
  /**
   * Set some style options for folding.
   */
  void setFoldFlags(int flags);
  /**
   * Ensure a particular line is visible by expanding any header line hiding it.
   * Use the currently set visibility policy to determine which range to
   * display.
   */
  void ensureVisibleEnforcePolicy(int line);
  /**
   * Sets whether a tab pressed when caret is within indentation indents.
   */
  void setTabIndents(bool tabIndents);
  /**
   * Does a tab pressed when caret is within indentation indent?
   */
  bool tabIndents() const;
  /**
   * Sets whether a backspace pressed when caret is within indentation
   * unindents.
   */
  void setBackSpaceUnIndents(bool bsUnIndents);
  /**
   * Does a backspace pressed when caret is within indentation unindent?
   */
  bool backSpaceUnIndents() const;
  /**
   * Sets the time the mouse must sit still to generate a mouse dwell event.
   */
  void setMouseDwellTime(int periodMilliseconds);
  /**
   * Retrieve the time the mouse must sit still to generate a mouse dwell event.
   */
  int mouseDwellTime() const;
  /**
   * Get position of start of word.
   */
  int wordStartPosition(int pos, bool onlyWordCharacters) const;
  /**
   * Get position of end of word.
   */
  int wordEndPosition(int pos, bool onlyWordCharacters) const;
  /**
   * Is the range start..end considered a word?
   */
  bool isRangeWord(int start, int end);
  /**
   * Sets whether text is word wrapped.
   */
  void setWrapMode(int mode);
  /**
   * Retrieve whether text is word wrapped.
   */
  int wrapMode() const;
  /**
   * Set the display mode of visual flags for wrapped lines.
   */
  void setWrapVisualFlags(int wrapVisualFlags);
  /**
   * Retrieve the display mode of visual flags for wrapped lines.
   */
  int wrapVisualFlags() const;
  /**
   * Set the location of visual flags for wrapped lines.
   */
  void setWrapVisualFlagsLocation(int wrapVisualFlagsLocation);
  /**
   * Retrieve the location of visual flags for wrapped lines.
   */
  int wrapVisualFlagsLocation() const;
  /**
   * Set the start indent for wrapped lines.
   */
  void setWrapStartIndent(int indent);
  /**
   * Retrieve the start indent for wrapped lines.
   */
  int wrapStartIndent() const;
  /**
   * Sets how wrapped sublines are placed. Default is fixed.
   */
  void setWrapIndentMode(int mode);
  /**
   * Retrieve how wrapped sublines are placed. Default is fixed.
   */
  int wrapIndentMode() const;
  /**
   * Sets the degree of caching of layout information.
   */
  void setLayoutCache(int mode);
  /**
   * Retrieve the degree of caching of layout information.
   */
  int layoutCache() const;
  /**
   * Sets the document width assumed for scrolling.
   */
  void setScrollWidth(int pixelWidth);
  /**
   * Retrieve the document width assumed for scrolling.
   */
  int scrollWidth() const;
  /**
   * Sets whether the maximum width line displayed is used to set scroll width.
   */
  void setScrollWidthTracking(bool tracking);
  /**
   * Retrieve whether the scroll width tracks wide lines.
   */
  bool scrollWidthTracking() const;
  /**
   * Measure the pixel width of some text in a particular style.
   * NUL terminated text argument.
   * Does not handle tab or control characters.
   */
  int textWidth(int style, const QString &text);
  /**
   * Sets the scroll range so that maximum scroll position has
   * the last line at the bottom of the view (default).
   * Setting this to false allows scrolling one page below the last line.
   */
  void setEndAtLastLine(bool endAtLastLine);
  /**
   * Retrieve whether the maximum scroll position has the last
   * line at the bottom of the view.
   */
  bool endAtLastLine() const;
  /**
   * Retrieve the height of a particular line of text in pixels.
   */
  int textHeight(int line);
  /**
   * Retrieve the point size of a particular line of text in pixels.
   */
  int fontPointSize(int line);
  /**
   * Show or hide the vertical scroll bar.
   */
  void setVScrollBar(bool show);
  /**
   * Is the vertical scroll bar visible?
   */
  bool vScrollBar() const;
  /**
   * Append a string to the end of the document without changing the selection.
   */
  void appendText(const QString &text);
  /**
   * Is drawing done in two phases with backgrounds drawn before foregrounds?
   */
  bool twoPhaseDraw() const;
  /**
   * In twoPhaseDraw mode, drawing is performed in two phases, first the
   * background and then the foreground. This avoids chopping off characters
   * that overlap the next run.
   */
  void setTwoPhaseDraw(bool twoPhase);
  /**
   * How many phases is drawing done in?
   */
  int phasesDraw() const;
  /**
   * In one phase draw, text is drawn in a series of rectangular blocks with no
   * overlap. In two phase draw, text is drawn in a series of lines allowing
   * runs to overlap horizontally. In multiple phase draw, each element is drawn
   * over the whole drawing area, allowing text to overlap from one line to the
   * next.
   */
  void setPhasesDraw(int phases);
  /**
   * Choose the quality level for text from the FontQuality enumeration.
   */
  void setFontQuality(int fontQuality);
  /**
   * Retrieve the quality level for text.
   */
  int fontQuality() const;
  /**
   * Scroll so that a display line is at the top of the display.
   */
  void setFirstVisibleLine(int lineDisplay);
  /**
   * Change the effect of pasting when there are multiple selections.
   */
  void setMultiPaste(int multiPaste);
  /**
   * Retrieve the effect of pasting when there are multiple selections..
   */
  int multiPaste() const;
  /**
   * Retrieve the value of a tag from a regular expression search.
   * Result is NUL-terminated.
   */
  int tag(int tagNumber, char *tagValue) const;
  /**
   * Join the lines in the target.
   */
  void linesJoin();
  /**
   * Split the lines in the target into lines that are less wide than pixelWidth
   * where possible.
   */
  void linesSplit(int pixelWidth);
  /**
   * Set the colours used as a chequerboard pattern in the fold margin
   */
  void setFoldMarginColour(bool useSetting, QColor back);
  /**
   * Set the colours used as a chequerboard pattern in the fold margin
   */
  void setFoldMarginHiColour(bool useSetting, QColor fore);
  /**
   * Move caret down one line.
   */
  void lineDown();
  /**
   * Move caret down one line extending selection to new caret position.
   */
  void lineDownExtend();
  /**
   * Move caret up one line.
   */
  void lineUp();
  /**
   * Move caret up one line extending selection to new caret position.
   */
  void lineUpExtend();
  /**
   * Move caret left one character.
   */
  void charLeft();
  /**
   * Move caret left one character extending selection to new caret position.
   */
  void charLeftExtend();
  /**
   * Move caret right one character.
   */
  void charRight();
  /**
   * Move caret right one character extending selection to new caret position.
   */
  void charRightExtend();
  /**
   * Move caret left one word.
   */
  void wordLeft();
  /**
   * Move caret left one word extending selection to new caret position.
   */
  void wordLeftExtend();
  /**
   * Move caret right one word.
   */
  void wordRight();
  /**
   * Move caret right one word extending selection to new caret position.
   */
  void wordRightExtend();
  /**
   * Move caret to first position on line.
   */
  void home();
  /**
   * Move caret to first position on line extending selection to new caret
   * position.
   */
  void homeExtend();
  /**
   * Move caret to last position on line.
   */
  void lineEnd();
  /**
   * Move caret to last position on line extending selection to new caret
   * position.
   */
  void lineEndExtend();
  /**
   * Move caret to first position in document.
   */
  void documentStart();
  /**
   * Move caret to first position in document extending selection to new caret
   * position.
   */
  void documentStartExtend();
  /**
   * Move caret to last position in document.
   */
  void documentEnd();
  /**
   * Move caret to last position in document extending selection to new caret
   * position.
   */
  void documentEndExtend();
  /**
   * Move caret one page up.
   */
  void pageUp();
  /**
   * Move caret one page up extending selection to new caret position.
   */
  void pageUpExtend();
  /**
   * Move caret one page down.
   */
  void pageDown();
  /**
   * Move caret one page down extending selection to new caret position.
   */
  void pageDownExtend();
  /**
   * Switch from insert to overtype mode or the reverse.
   */
  void editToggleOvertype();
  /**
   * Cancel any modes such as call tip or auto-completion list display.
   */
  void cancel();
  /**
   * Delete the selection or if no selection, the character before the caret.
   */
  void deleteBack();
  /**
   * If selection is empty or all on one line replace the selection with a tab
   * character. If more than one line selected, indent the lines.
   */
  void tab();
  /**
   * Dedent the selected lines.
   */
  void backTab();
  /**
   * Insert a new line, may use a CRLF, CR or LF depending on EOL mode.
   */
  void newLine();
  /**
   * Insert a Form Feed character.
   */
  void formFeed();
  /**
   * Move caret to before first visible character on line.
   * If already there move to first character on line.
   */
  void vCHome();
  /**
   * Like VCHome but extending selection to new caret position.
   */
  void vCHomeExtend();
  /**
   * Magnify the displayed text by increasing the sizes by 1 point.
   */
  void zoomIn();
  /**
   * Make the displayed text smaller by decreasing the sizes by 1 point.
   */
  void zoomOut();
  /**
   * Delete the word to the left of the caret.
   */
  void delWordLeft();
  /**
   * Delete the word to the right of the caret.
   */
  void delWordRight();
  /**
   * Delete the word to the right of the caret, but not the trailing non-word
   * characters.
   */
  void delWordRightEnd();
  /**
   * Cut the line containing the caret.
   */
  void lineCut();
  /**
   * Delete the line containing the caret.
   */
  void lineDelete();
  /**
   * Switch the current line with the previous.
   */
  void lineTranspose();
  /**
   * Duplicate the current line.
   */
  void lineDuplicate();
  /**
   * Transform the selection to lower case.
   */
  void lowerCase();
  /**
   * Transform the selection to upper case.
   */
  void upperCase();
  /**
   * Scroll the document down, keeping the caret visible.
   */
  void lineScrollDown();
  /**
   * Scroll the document up, keeping the caret visible.
   */
  void lineScrollUp();
  /**
   * Delete the selection or if no selection, the character before the caret.
   * Will not delete the character before at the start of a line.
   */
  void deleteBackNotLine();
  /**
   * Move caret to first position on display line.
   */
  void homeDisplay();
  /**
   * Move caret to first position on display line extending selection to
   * new caret position.
   */
  void homeDisplayExtend();
  /**
   * Move caret to last position on display line.
   */
  void lineEndDisplay();
  /**
   * Move caret to last position on display line extending selection to new
   * caret position.
   */
  void lineEndDisplayExtend();
  /**
   * These are like their namesakes Home(Extend)?, LineEnd(Extend)?,
   * VCHome(Extend)? except they behave differently when word-wrap is enabled:
   * They go first to the start / end of the display line, like
   * (Home|LineEnd)Display The difference is that, the cursor is already at the
   * point, it goes on to the start or end of the document line, as appropriate
   * for (Home|LineEnd|VCHome)(Extend)?.
   */
  void homeWrap();
  /**
   * These are like their namesakes Home(Extend)?, LineEnd(Extend)?,
   * VCHome(Extend)? except they behave differently when word-wrap is enabled:
   * They go first to the start / end of the display line, like
   * (Home|LineEnd)Display The difference is that, the cursor is already at the
   * point, it goes on to the start or end of the document line, as appropriate
   * for (Home|LineEnd|VCHome)(Extend)?.
   */
  void homeWrapExtend();
  /**
   * These are like their namesakes Home(Extend)?, LineEnd(Extend)?,
   * VCHome(Extend)? except they behave differently when word-wrap is enabled:
   * They go first to the start / end of the display line, like
   * (Home|LineEnd)Display The difference is that, the cursor is already at the
   * point, it goes on to the start or end of the document line, as appropriate
   * for (Home|LineEnd|VCHome)(Extend)?.
   */
  void lineEndWrap();
  /**
   * These are like their namesakes Home(Extend)?, LineEnd(Extend)?,
   * VCHome(Extend)? except they behave differently when word-wrap is enabled:
   * They go first to the start / end of the display line, like
   * (Home|LineEnd)Display The difference is that, the cursor is already at the
   * point, it goes on to the start or end of the document line, as appropriate
   * for (Home|LineEnd|VCHome)(Extend)?.
   */
  void lineEndWrapExtend();
  /**
   * These are like their namesakes Home(Extend)?, LineEnd(Extend)?,
   * VCHome(Extend)? except they behave differently when word-wrap is enabled:
   * They go first to the start / end of the display line, like
   * (Home|LineEnd)Display The difference is that, the cursor is already at the
   * point, it goes on to the start or end of the document line, as appropriate
   * for (Home|LineEnd|VCHome)(Extend)?.
   */
  void vCHomeWrap();
  /**
   * These are like their namesakes Home(Extend)?, LineEnd(Extend)?,
   * VCHome(Extend)? except they behave differently when word-wrap is enabled:
   * They go first to the start / end of the display line, like
   * (Home|LineEnd)Display The difference is that, the cursor is already at the
   * point, it goes on to the start or end of the document line, as appropriate
   * for (Home|LineEnd|VCHome)(Extend)?.
   */
  void vCHomeWrapExtend();
  /**
   * Copy the line containing the caret.
   */
  void lineCopy();
  /**
   * Move the caret inside current view if it's not there already.
   */
  void moveCaretInsideView();
  /**
   * How many characters are on a line, including end of line characters?
   */
  int lineLength(int line);
  /**
   * Highlight the characters at two positions.
   */
  void braceHighlight(int pos1, int pos2);
  /**
   * Use specified indicator to highlight matching braces instead of changing
   * their style.
   */
  void braceHighlightIndicator(bool useBraceHighlightIndicator, int indicator);
  /**
   * Highlight the character at a position indicating there is no matching
   * brace.
   */
  void braceBadLight(int pos);
  /**
   * Use specified indicator to highlight non matching brace instead of changing
   * its style.
   */
  void braceBadLightIndicator(bool useBraceBadLightIndicator, int indicator);
  /**
   * Find the position of a matching brace or INVALID_POSITION if no match.
   */
  int braceMatch(int pos);
  /**
   * Are the end of line characters visible?
   */
  bool viewEOL() const;
  /**
   * Make the end of line characters visible or invisible.
   */
  void setViewEOL(bool visible);
  /**
   * Retrieve a pointer to the document object.
   */
  sptr_t docPointer() const;
  /**
   * Change the document object used.
   */
  void setDocPointer(sptr_t pointer);
  /**
   * Set which document modification events are sent to the container.
   */
  void setModEventMask(int mask);
  /**
   * Retrieve the column number which text should be kept within.
   */
  int edgeColumn() const;
  /**
   * Set the column number of the edge.
   * If text goes past the edge then it is highlighted.
   */
  void setEdgeColumn(int column);
  /**
   * Retrieve the edge highlight mode.
   */
  int edgeMode() const;
  /**
   * The edge may be displayed by a line (EDGE_LINE) or by highlighting text
   * that goes beyond it (EDGE_BACKGROUND) or not displayed at all (EDGE_NONE).
   */
  void setEdgeMode(int mode);
  /**
   * Retrieve the colour used in edge indication.
   */
  QColor edgeColour() const;
  /**
   * Change the colour used in edge indication.
   */
  void setEdgeColour(QColor edgeColour);
  /**
   * Sets the current caret position to be the search anchor.
   */
  void searchAnchor();
  /**
   * Find some text starting at the search anchor.
   * Does not ensure the selection is visible.
   */
  int searchNext(int flags, const QString &text);
  /**
   * Find some text starting at the search anchor and moving backwards.
   * Does not ensure the selection is visible.
   */
  int searchPrev(int flags, const QString &text);
  /**
   * Retrieves the number of lines completely visible.
   */
  int linesOnScreen() const;
  /**
   * Set whether a pop up menu is displayed automatically when the user presses
   * the wrong mouse button.
   */
  void usePopUp(bool allowPopUp);
  /**
   * Is the selection rectangular? The alternative is the more common stream
   * selection.
   */
  bool selectionIsRectangle() const;
  /**
   * Set the zoom level. This number of points is added to the size of all
   * fonts. It may be positive to magnify or negative to reduce.
   */
  void setZoom(int zoom);
  /**
   * Retrieve the zoom level.
   */
  int zoom() const;
  /**
   * Create a new document object.
   * Starts with reference count of 1 and not selected into editor.
   */
  int createDocument();
  /**
   * Extend life of document.
   */
  void addRefDocument(int doc);
  /**
   * Release a reference to the document, deleting document if it fades to
   * black.
   */
  void releaseDocument(int doc);
  /**
   * Get which document modification events are sent to the container.
   */
  int modEventMask() const;
  /**
   * Change internal focus flag.
   */
  void setFocus(bool focus);
  /**
   * Get internal focus flag.
   */
  bool focus() const;
  /**
   * Change error status - 0 = OK.
   */
  void setStatus(int statusCode);
  /**
   * Get error status.
   */
  int status() const;
  /**
   * Set whether the mouse is captured when its button is pressed.
   */
  void setMouseDownCaptures(bool captures);
  /**
   * Get whether mouse gets captured.
   */
  bool mouseDownCaptures() const;
  /**
   * Sets the cursor to one of the SC_CURSOR* values.
   */
  void setCursor(int cursorType);
  /**
   * Get cursor type.
   */
  int cursor() const;
  /**
   * Change the way control characters are displayed:
   * If symbol is < 32, keep the drawn way, else, use the given character.
   */
  void setControlCharSymbol(int symbol);
  /**
   * Get the way control characters are displayed.
   */
  int controlCharSymbol() const;
  /**
   * Move to the previous change in capitalisation.
   */
  void wordPartLeft();
  /**
   * Move to the previous change in capitalisation extending selection
   * to new caret position.
   */
  void wordPartLeftExtend();
  /**
   * Move to the change next in capitalisation.
   */
  void wordPartRight();
  /**
   * Move to the next change in capitalisation extending selection
   * to new caret position.
   */
  void wordPartRightExtend();
  /**
   * Set the way the display area is determined when a particular line
   * is to be moved to by Find, FindNext, GotoLine, etc.
   */
  void setVisiblePolicy(int visiblePolicy, int visibleSlop);
  /**
   * Delete back from the current position to the start of the line.
   */
  void delLineLeft();
  /**
   * Delete forwards from the current position to the end of the line.
   */
  void delLineRight();
  /**
   * Get and Set the xOffset (ie, horizontal scroll position).
   */
  void setXOffset(int newOffset);
  /**
   * Get and Set the xOffset (ie, horizontal scroll position).
   */
  int xOffset() const;
  /**
   * Set the last x chosen value to be the caret x position.
   */
  void chooseCaretX();
  /**
   * Set the focus to this Scintilla widget.
   */
  void grabFocus();
  /**
   * Set the way the caret is kept visible when going sideways.
   * The exclusion zone is given in pixels.
   */
  void setXCaretPolicy(int caretPolicy, int caretSlop);
  /**
   * Set the way the line the caret is on is kept visible.
   * The exclusion zone is given in lines.
   */
  void setYCaretPolicy(int caretPolicy, int caretSlop);
  /**
   * Set printing to line wrapped (SC_WRAP_WORD) or not line wrapped
   * (SC_WRAP_NONE).
   */
  void setPrintWrapMode(int mode);
  /**
   * Is printing line wrapped?
   */
  int printWrapMode() const;
  /**
   * Set a fore colour for active hotspots.
   */
  void setHotspotActiveFore(bool useSetting, QColor fore);
  /**
   * Get the fore colour for active hotspots.
   */
  QColor hotspotActiveFore() const;
  /**
   * Set a back colour for active hotspots.
   */
  void setHotspotActiveBack(bool useSetting, QColor back);
  /**
   * Get the back colour for active hotspots.
   */
  QColor hotspotActiveBack() const;
  /**
   * Enable / Disable underlining active hotspots.
   */
  void setHotspotActiveUnderline(bool underline);
  /**
   * Get whether underlining for active hotspots.
   */
  bool hotspotActiveUnderline() const;
  /**
   * Limit hotspots to single line so hotspots on two lines don't merge.
   */
  void setHotspotSingleLine(bool singleLine);
  /**
   * Get the HotspotSingleLine property
   */
  bool hotspotSingleLine() const;
  /**
   * Move caret between paragraphs (delimited by empty lines).
   */
  void paraDown();
  /**
   * Move caret between paragraphs (delimited by empty lines).
   */
  void paraDownExtend();
  /**
   * Move caret between paragraphs (delimited by empty lines).
   */
  void paraUp();
  /**
   * Move caret between paragraphs (delimited by empty lines).
   */
  void paraUpExtend();
  /**
   * Given a valid document position, return the previous position taking code
   * page into account. Returns 0 if passed 0.
   */
  int positionBefore(int pos);
  /**
   * Given a valid document position, return the next position taking code
   * page into account. Maximum value returned is the last position in the
   * document.
   */
  int positionAfter(int pos);
  /**
   * Given a valid document position, return a position that differs in a number
   * of characters. Returned value is always between 0 and last position in
   * document.
   */
  int positionRelative(int pos, int relative);
  /**
   * Copy a range of text to the clipboard. Positions are clipped into the
   * document.
   */
  void copyRange(int start, int end);
  /**
   * Copy argument text to the clipboard.
   */
  void copyText(const QString &text);
  /**
   * Set the selection mode to stream (SC_SEL_STREAM) or rectangular
   * (SC_SEL_RECTANGLE/SC_SEL_THIN) or by lines (SC_SEL_LINES).
   */
  void setSelectionMode(int mode);
  /**
   * Get the mode of the current selection.
   */
  int selectionMode() const;
  /**
   * Retrieve the position of the start of the selection at the given line
   * (INVALID_POSITION if no selection on this line).
   */
  int lineSelStartPosition(int line);
  /**
   * Retrieve the position of the end of the selection at the given line
   * (INVALID_POSITION if no selection on this line).
   */
  int lineSelEndPosition(int line);
  /**
   * Move caret down one line, extending rectangular selection to new caret
   * position.
   */
  void lineDownRectExtend();
  /**
   * Move caret up one line, extending rectangular selection to new caret
   * position.
   */
  void lineUpRectExtend();
  /**
   * Move caret left one character, extending rectangular selection to new caret
   * position.
   */
  void charLeftRectExtend();
  /**
   * Move caret right one character, extending rectangular selection to new
   * caret position.
   */
  void charRightRectExtend();
  /**
   * Move caret to first position on line, extending rectangular selection to
   * new caret position.
   */
  void homeRectExtend();
  /**
   * Move caret to before first visible character on line.
   * If already there move to first character on line.
   * In either case, extend rectangular selection to new caret position.
   */
  void vCHomeRectExtend();
  /**
   * Move caret to last position on line, extending rectangular selection to new
   * caret position.
   */
  void lineEndRectExtend();
  /**
   * Move caret one page up, extending rectangular selection to new caret
   * position.
   */
  void pageUpRectExtend();
  /**
   * Move caret one page down, extending rectangular selection to new caret
   * position.
   */
  void pageDownRectExtend();
  /**
   * Move caret to top of page, or one page up if already at top of page.
   */
  void stutteredPageUp();
  /**
   * Move caret to top of page, or one page up if already at top of page,
   * extending selection to new caret position.
   */
  void stutteredPageUpExtend();
  /**
   * Move caret to bottom of page, or one page down if already at bottom of
   * page.
   */
  void stutteredPageDown();
  /**
   * Move caret to bottom of page, or one page down if already at bottom of
   * page, extending selection to new caret position.
   */
  void stutteredPageDownExtend();
  /**
   * Move caret left one word, position cursor at end of word.
   */
  void wordLeftEnd();
  /**
   * Move caret left one word, position cursor at end of word, extending
   * selection to new caret position.
   */
  void wordLeftEndExtend();
  /**
   * Move caret right one word, position cursor at end of word.
   */
  void wordRightEnd();
  /**
   * Move caret right one word, position cursor at end of word, extending
   * selection to new caret position.
   */
  void wordRightEndExtend();
  /**
   * Set the set of characters making up whitespace for when moving or selecting
   * by word. Should be called after SetWordChars.
   */
  void setWhitespaceChars(const QByteArray &chars);
  /**
   * Get the set of characters making up whitespace for when moving or selecting
   * by word.
   */
  QByteArray whitespaceChars() const;
  /**
   * Set the set of characters making up punctuation characters
   * Should be called after SetWordChars.
   */
  void setPunctuationChars(const QByteArray &chars);
  /**
   * Get the set of characters making up punctuation characters
   */
  QByteArray punctuationChars() const;
  /**
   * Reset the set of characters for whitespace and word characters to the
   * defaults.
   */
  void setCharsDefault();
  /**
   * Get currently selected item position in the auto-completion list
   */
  int autoCCurrent() const;
  /**
   * Get currently selected item text in the auto-completion list
   * Returns the length of the item text
   * Result is NUL-terminated.
   */
  int autoCCurrentText(char *s) const;
  /**
   * Set auto-completion case insensitive behaviour to either prefer
   * case-sensitive matches or have no preference.
   */
  void autoCSetCaseInsensitiveBehaviour(int behaviour);
  /**
   * Get auto-completion case insensitive behaviour.
   */
  int autoCCaseInsensitiveBehaviour() const;
  /**
   * Change the effect of autocompleting when there are multiple selections.
   */
  void autoCSetMulti(int multi);
  /**
   * Retrieve the effect of autocompleting when there are multiple selections..
   */
  int autoCMulti() const;
  /**
   * Set the way autocompletion lists are ordered.
   */
  void autoCSetOrder(int order);
  /**
   * Get the way autocompletion lists are ordered.
   */
  int autoCOrder() const;
  /**
   * Enlarge the document to a particular size of text bytes.
   */
  void allocate(int bytes);
  /**
   * Returns the target converted to UTF8.
   * Return the length in bytes.
   */
  int targetAsUTF8(char *s);
  /**
   * Set the length of the utf8 argument for calling EncodedFromUTF8.
   * Set to -1 and the string will be measured to the first nul.
   */
  void setLengthForEncode(int bytes);
  /**
   * Translates a UTF8 string into the document encoding.
   * Return the length of the result in bytes.
   * On error return 0.
   */
  int encodedFromUTF8(const char *utf8, char *encoded);
  /**
   * Find the position of a column on a line taking into account tabs and
   * multi-byte characters. If beyond end of line, return line end position.
   */
  int findColumn(int line, int column);
  /**
   * Can the caret preferred x position only be changed by explicit movement
   * commands?
   */
  int caretSticky() const;
  /**
   * Stop the caret preferred x position changing when the user types.
   */
  void setCaretSticky(int useCaretStickyBehaviour);
  /**
   * Switch between sticky and non-sticky: meant to be bound to a key.
   */
  void toggleCaretSticky();
  /**
   * Enable/Disable convert-on-paste for line endings
   */
  void setPasteConvertEndings(bool convert);
  /**
   * Get convert-on-paste setting
   */
  bool pasteConvertEndings() const;
  /**
   * Duplicate the selection. If selection empty duplicate the line containing
   * the caret.
   */
  void selectionDuplicate();
  /**
   * Set background alpha of the caret line.
   */
  void setCaretLineBackAlpha(int alpha);
  /**
   * Get the background alpha of the caret line.
   */
  int caretLineBackAlpha() const;
  /**
   * Set the style of the caret to be drawn.
   */
  void setCaretStyle(int caretStyle);
  /**
   * Returns the current style of the caret.
   */
  int caretStyle() const;
  /**
   * Set the indicator used for IndicatorFillRange and IndicatorClearRange
   */
  void setIndicatorCurrent(int indicator);
  /**
   * Get the current indicator
   */
  int indicatorCurrent() const;
  /**
   * Set the value used for IndicatorFillRange
   */
  void setIndicatorValue(int value);
  /**
   * Get the current indicator value
   */
  int indicatorValue() const;
  /**
   * Turn a indicator on over a range.
   */
  void indicatorFillRange(int position, int fillLength);
  /**
   * Turn a indicator off over a range.
   */
  void indicatorClearRange(int position, int clearLength);
  /**
   * Are any indicators present at position?
   */
  int indicatorAllOnFor(int position);
  /**
   * What value does a particular indicator have at at a position?
   */
  int indicatorValueAt(int indicator, int position);
  /**
   * Where does a particular indicator start?
   */
  int indicatorStart(int indicator, int position);
  /**
   * Where does a particular indicator end?
   */
  int indicatorEnd(int indicator, int position);
  /**
   * Set number of entries in position cache
   */
  void setPositionCache(int size);
  /**
   * How many entries are allocated to the position cache?
   */
  int positionCache() const;
  /**
   * Copy the selection, if selection empty copy the line with the caret
   */
  void copyAllowLine();
  /**
   * Compact the document buffer and return a read-only pointer to the
   * characters in the document.
   */
  const char *characterPointer() const;
  /**
   * Return a read-only pointer to a range of characters in the document.
   * May move the gap so that the range is contiguous, but will only move up
   * to rangeLength bytes.
   */
  int rangePointer(int position, int rangeLength) const;
  /**
   * Return a position which, to avoid performance costs, should not be within
   * the range of a call to GetRangePointer.
   */
  int gapPosition() const;
  /**
   * Set the alpha fill colour of the given indicator.
   */
  void indicSetAlpha(int indicator, int alpha);
  /**
   * Get the alpha fill colour of the given indicator.
   */
  int indicAlpha(int indicator) const;
  /**
   * Set the alpha outline colour of the given indicator.
   */
  void indicSetOutlineAlpha(int indicator, int alpha);
  /**
   * Get the alpha outline colour of the given indicator.
   */
  int indicOutlineAlpha(int indicator) const;
  /**
   * Set extra ascent for each line
   */
  void setExtraAscent(int extraAscent);
  /**
   * Get extra ascent for each line
   */
  int extraAscent() const;
  /**
   * Set extra descent for each line
   */
  void setExtraDescent(int extraDescent);
  /**
   * Get extra descent for each line
   */
  int extraDescent() const;
  /**
   * Which symbol was defined for markerNumber with MarkerDefine
   */
  int markerSymbolDefined(int markerNumber);
  /**
   * Set the text in the text margin for a line
   */
  void marginSetText(int line, const QString &text);
  /**
   * Get the text in the text margin for a line
   */
  QString marginText(int line) const;
  /**
   * Set the style number for the text margin for a line
   */
  void marginSetStyle(int line, int style);
  /**
   * Get the style number for the text margin for a line
   */
  int marginStyle(int line) const;
  /**
   * Set the style in the text margin for a line
   */
  void marginSetStyles(int line, const QString &styles);
  /**
   * Get the styles in the text margin for a line
   */
  int marginStyles(int line, char *styles) const;
  /**
   * Clear the margin text on all lines
   */
  void marginTextClearAll();
  /**
   * Get the start of the range of style numbers used for margin text
   */
  void marginSetStyleOffset(int style);
  /**
   * Get the start of the range of style numbers used for margin text
   */
  int marginStyleOffset() const;
  /**
   * Set the margin options.
   */
  void setMarginOptions(int marginOptions);
  /**
   * Get the margin options.
   */
  int marginOptions() const;
  /**
   * Set the annotation text for a line
   */
  void annotationSetText(int line, const QString &text);
  /**
   * Get the annotation text for a line
   */
  QString annotationText(int line) const;
  /**
   * Set the style number for the annotations for a line
   */
  void annotationSetStyle(int line, int style);
  /**
   * Get the style number for the annotations for a line
   */
  int annotationStyle(int line) const;
  /**
   * Set the annotation styles for a line
   */
  void annotationSetStyles(int line, const QByteArray &styles);
  /**
   * Get the annotation styles for a line
   */
  int annotationStyles(int line, char *styles) const;
  /**
   * Get the number of annotation lines for a line
   */
  int annotationLines(int line) const;
  /**
   * Clear the annotations from all lines
   */
  void annotationClearAll();
  /**
   * Set the visibility for the annotations for a view
   */
  void annotationSetVisible(int visible);
  /**
   * Get the visibility for the annotations for a view
   */
  int annotationVisible() const;
  /**
   * Get the start of the range of style numbers used for annotations
   */
  void annotationSetStyleOffset(int style);
  /**
   * Get the start of the range of style numbers used for annotations
   */
  int annotationStyleOffset() const;
  /**
   * Release all extended (>255) style numbers
   */
  void releaseAllExtendedStyles();
  /**
   * Allocate some extended (>255) style numbers and return the start of the
   * range
   */
  int allocateExtendedStyles(int numberStyles);
  /**
   * Add a container action to the undo stack
   */
  void addUndoAction(int token, int flags);
  /**
   * Find the position of a character from a point within the window.
   */
  int charPositionFromPoint(int x, int y);
  /**
   * Find the position of a character from a point within the window.
   * Return INVALID_POSITION if not close to text.
   */
  int charPositionFromPointClose(int x, int y);
  /**
   * Set whether switching to rectangular mode while selecting with the mouse is
   * allowed.
   */
  void setMouseSelectionRectangularSwitch(bool mouseSelectionRectangularSwitch);
  /**
   * Whether switching to rectangular mode while selecting with the mouse is
   * allowed.
   */
  bool mouseSelectionRectangularSwitch() const;
  /**
   * Set whether multiple selections can be made
   */
  void setMultipleSelection(bool multipleSelection);
  /**
   * Whether multiple selections can be made
   */
  bool multipleSelection() const;
  /**
   * Set whether typing can be performed into multiple selections
   */
  void setAdditionalSelectionTyping(bool additionalSelectionTyping);
  /**
   * Whether typing can be performed into multiple selections
   */
  bool additionalSelectionTyping() const;
  /**
   * Set whether additional carets will blink
   */
  void setAdditionalCaretsBlink(bool additionalCaretsBlink);
  /**
   * Whether additional carets will blink
   */
  bool additionalCaretsBlink() const;
  /**
   * Set whether additional carets are visible
   */
  void setAdditionalCaretsVisible(bool additionalCaretsBlink);
  /**
   * Whether additional carets are visible
   */
  bool additionalCaretsVisible() const;
  /**
   * How many selections are there?
   */
  int selections() const;
  /**
   * Is every selected range empty?
   */
  bool selectionEmpty() const;
  /**
   * Clear selections to a single empty stream selection
   */
  void clearSelections();
  /**
   * Set a simple selection
   */
  int setSelection(int caret, int anchor);
  /**
   * Add a selection
   */
  int addSelection(int caret, int anchor);
  /**
   * Drop one selection
   */
  void dropSelectionN(int selection);
  /**
   * Set the main selection
   */
  void setMainSelection(int selection);
  /**
   * Which selection is the main selection
   */
  int mainSelection() const;
  /**
   * Which selection is the main selection
   */
  void setSelectionNCaret(int selection, int pos);
  /**
   * Which selection is the main selection
   */
  int selectionNCaret(int selection) const;
  /**
   * Which selection is the main selection
   */
  void setSelectionNAnchor(int selection, int posAnchor);
  /**
   * Which selection is the main selection
   */
  int selectionNAnchor(int selection) const;
  /**
   * Which selection is the main selection
   */
  void setSelectionNCaretVirtualSpace(int selection, int space);
  /**
   * Which selection is the main selection
   */
  int selectionNCaretVirtualSpace(int selection) const;
  /**
   * Which selection is the main selection
   */
  void setSelectionNAnchorVirtualSpace(int selection, int space);
  /**
   * Which selection is the main selection
   */
  int selectionNAnchorVirtualSpace(int selection) const;
  /**
   * Sets the position that starts the selection - this becomes the anchor.
   */
  void setSelectionNStart(int selection, int pos);
  /**
   * Returns the position at the start of the selection.
   */
  int selectionNStart(int selection) const;
  /**
   * Sets the position that ends the selection - this becomes the
   * currentPosition.
   */
  void setSelectionNEnd(int selection, int pos);
  /**
   * Returns the position at the end of the selection.
   */
  int selectionNEnd(int selection) const;
  /**
   * Returns the position at the end of the selection.
   */
  void setRectangularSelectionCaret(int pos);
  /**
   * Returns the position at the end of the selection.
   */
  int rectangularSelectionCaret() const;
  /**
   * Returns the position at the end of the selection.
   */
  void setRectangularSelectionAnchor(int posAnchor);
  /**
   * Returns the position at the end of the selection.
   */
  int rectangularSelectionAnchor() const;
  /**
   * Returns the position at the end of the selection.
   */
  void setRectangularSelectionCaretVirtualSpace(int space);
  /**
   * Returns the position at the end of the selection.
   */
  int rectangularSelectionCaretVirtualSpace() const;
  /**
   * Returns the position at the end of the selection.
   */
  void setRectangularSelectionAnchorVirtualSpace(int space);
  /**
   * Returns the position at the end of the selection.
   */
  int rectangularSelectionAnchorVirtualSpace() const;
  /**
   * Returns the position at the end of the selection.
   */
  void setVirtualSpaceOptions(int virtualSpaceOptions);
  /**
   * Returns the position at the end of the selection.
   */
  int virtualSpaceOptions() const;
  /**
   * On GTK+, allow selecting the modifier key to use for mouse-based
   * rectangular selection. Often the window manager requires Alt+Mouse Drag
   * for moving windows.
   * Valid values are SCMOD_CTRL(default), SCMOD_ALT, or SCMOD_SUPER.
   */
  void setRectangularSelectionModifier(int modifier);
  /**
   * Get the modifier key used for rectangular selection.
   */
  int rectangularSelectionModifier() const;
  /**
   * Set the foreground colour of additional selections.
   * Must have previously called SetSelFore with non-zero first argument for
   * this to have an effect.
   */
  void setAdditionalSelFore(QColor fore);
  /**
   * Set the background colour of additional selections.
   * Must have previously called SetSelBack with non-zero first argument for
   * this to have an effect.
   */
  void setAdditionalSelBack(QColor back);
  /**
   * Set the alpha of the selection.
   */
  void setAdditionalSelAlpha(int alpha);
  /**
   * Get the alpha of the selection.
   */
  int additionalSelAlpha() const;
  /**
   * Set the foreground colour of additional carets.
   */
  void setAdditionalCaretFore(QColor fore);
  /**
   * Get the foreground colour of additional carets.
   */
  QColor additionalCaretFore() const;
  /**
   * Set the main selection to the next selection.
   */
  void rotateSelection();
  /**
   * Swap that caret and anchor of the main selection.
   */
  void swapMainAnchorCaret();
  /**
   * Add the next occurrence of the main selection to the set of selections as
   * main. If the current selection is empty then select word around caret.
   */
  void multipleSelectAddNext();
  /**
   * Add each occurrence of the main selection in the target to the set of
   * selections. If the current selection is empty then select word around
   * caret.
   */
  void multipleSelectAddEach();
  /**
   * Indicate that the internal state of a lexer has changed over a range and
   * therefore there may be a need to redraw.
   */
  int changeLexerState(int start, int end);
  /**
   * Find the next line at or after lineStart that is a contracted fold header
   * line. Return -1 when no more lines.
   */
  int contractedFoldNext(int lineStart);
  /**
   * Centre current line in window.
   */
  void verticalCentreCaret();
  /**
   * Move the selected lines up one line, shifting the line above after the
   * selection
   */
  void moveSelectedLinesUp();
  /**
   * Move the selected lines down one line, shifting the line below before the
   * selection
   */
  void moveSelectedLinesDown();
  /**
   * Set the identifier reported as idFrom in notification messages.
   */
  void setIdentifier(int identifier);
  /**
   * Get the identifier.
   */
  int identifier() const;
  /**
   * Define a marker from image data.
   */
  void markerDefineImage(int markerNumber, const QImage &image);
  /**
   * Register an RGBA image for use in autocompletion lists.
   * It has the width and height from RGBAImageSetWidth/Height
   */
  void registerRGBAImage(int type, const QString &pixels);
  /**
   * Scroll to start of document.
   */
  void scrollToStart();
  /**
   * Scroll to end of document.
   */
  void scrollToEnd();
  /**
   * Set the technology used.
   */
  void setTechnology(int technology);
  /**
   * Get the tech.
   */
  int technology() const;
  /**
   * Create an ILoader*.
   */
  int createLoader(int bytes);
  /**
   * On OS X, show a find indicator.
   */
  void findIndicatorShow(int start, int end);
  /**
   * On OS X, flash a find indicator, then fade out.
   */
  void findIndicatorFlash(int start, int end);
  /**
   * On OS X, hide the find indicator.
   */
  void findIndicatorHide();
  /**
   * Move caret to before first visible character on display line.
   * If already there move to first character on display line.
   */
  void vCHomeDisplay();
  /**
   * Like VCHomeDisplay but extending selection to new caret position.
   */
  void vCHomeDisplayExtend();
  /**
   * Is the caret line always visible?
   */
  bool caretLineVisibleAlways() const;
  /**
   * Sets the caret line to always visible.
   */
  void setCaretLineVisibleAlways(bool alwaysVisible);
  /**
   * Set the line end types that the application wants to use. May not be used
   * if incompatible with lexer or encoding.
   */
  void setLineEndTypesAllowed(int lineEndBitSet);
  /**
   * Get the line end types currently allowed.
   */
  int lineEndTypesAllowed() const;
  /**
   * Get the line end types currently recognised. May be a subset of the allowed
   * types due to lexer limitation.
   */
  int lineEndTypesActive() const;
  /**
   * Set the way a character is drawn.
   */
  void setRepresentation(const QString &encodedCharacter,
                         const QString &representation);
  /**
   * Set the way a character is drawn.
   * Result is NUL-terminated.
   */
  int representation(const QString &encodedCharacter,
                     char *representation) const;
  /**
   * Remove a character representation.
   */
  void clearRepresentation(const QString &encodedCharacter);
  /**
   * Start notifying the container of all key presses and commands.
   */
  void startRecord();
  /**
   * Stop notifying the container of all key presses and commands.
   */
  void stopRecord();
  /**
   * Set the lexing language of the document.
   */
  void setLexer(int lexer);
  /**
   * Retrieve the lexing language of the document.
   */
  int lexer() const;
  /**
   * Colorize a segment of the document using the current lexing language.
   */
  void colorize(int start, int end);
  /**
   * Set up a value that may be used by a lexer for some optional feature.
   */
  void setProperty(const QString &key, const QString &value);
  /**
   * Set up the key words used by the lexer.
   */
  void setKeyWords(int keywordSet, const QString &keyWords);
  /**
   * Set the lexing language of the document based on string name.
   */
  void setLexerLanguage(const QString &language);
  /**
   * Load a lexer library (dll / so).
   */
  void loadLexerLibrary(const QString &path);
  /**
   * Retrieve a "property" value previously set with SetProperty.
   * Result is NUL-terminated.
   */
  int property(const QString &key, char *buf) const;
  /**
   * Retrieve a "property" value previously set with SetProperty,
   * with "$()" variable replacement on returned buffer.
   * Result is NUL-terminated.
   */
  int propertyExpanded(const QString &key, char *buf) const;
  /**
   * Retrieve a "property" value previously set with SetProperty,
   * interpreted as an int AFTER any "$()" variable replacement.
   */
  int propertyInt(const QString &key) const;
  /**
   * Retrieve the name of the lexer.
   * Return the length of the text.
   * Result is NUL-terminated.
   */
  int lexerLanguage(char *text) const;
  /**
   * For private communication between an application and a known lexer.
   */
  uintptr_t privateLexerCall(int operation, uintptr_t pointer);
  /**
   * Retrieve a '\n' separated list of properties understood by the current
   * lexer. Result is NUL-terminated.
   */
  int propertyNames(char *names);
  /**
   * Retrieve the type of a property.
   */
  int propertyType(const QString &name);
  /**
   * Describe a property.
   * Result is NUL-terminated.
   */
  int describeProperty(const QString &name, char *description);
  /**
   * Retrieve a '\n' separated list of descriptions of the keyword sets
   * understood by the current lexer. Result is NUL-terminated.
   */
  int describeKeyWordSets(char *descriptions);
  /**
   * Bit set of LineEndType enumertion for which line ends beyond the standard
   * LF, CR, and CRLF are supported by the lexer.
   */
  int lineEndTypesSupported() const;
  /**
   * Allocate a set of sub styles for a particular base style, returning start
   * of range
   */
  int allocateSubStyles(int styleBase, int numberStyles);
  /**
   * The starting style number for the sub styles associated with a base style
   */
  int subStylesStart(int styleBase) const;
  /**
   * The number of sub styles associated with a base style
   */
  int subStylesLength(int styleBase) const;
  /**
   * For a sub style, return the base style, else return the argument.
   */
  int styleFromSubStyle(int subStyle) const;
  /**
   * For a secondary style, return the primary style, else return the argument.
   */
  int primaryStyleFromStyle(int style) const;
  /**
   * Free allocated sub styles
   */
  void freeSubStyles();
  /**
   * Set the identifiers that are shown in a particular style
   */
  void setIdentifiers(int style, const QString &identifiers);
  /**
   * Where styles are duplicated by a feature such as active/inactive code
   * return the distance between the two types.
   */
  int distanceToSecondaryStyles() const;
  /**
   * Get the set of base styles that can be extended with sub styles
   * Result is NUL-terminated.
   */
  int subStyleBases(char *styles) const;
  //--
};

} // namespace Scintilla

#endif
