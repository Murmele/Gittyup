//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ScintillaIFace.h"
#include <Platform.h>
#include <Scintilla.h>
#include <QVarLengthArray>

namespace Scintilla {

namespace {

long encodeColor(const QColor &color) {
  long result = 0;

  result |= color.red();
  result |= color.green() << 8;
  result |= color.blue() << 16;

  return result;
}

QColor decodeColor(long color) {
  return QColor(color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff);
}

} // namespace

ScintillaIFace::ScintillaIFace(QWidget *parent) : ScintillaQt(parent) {}

// The functions below are generated from Scintilla.iface.

//++FuncImp
void ScintillaIFace::addText(const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  send(SCI_ADDTEXT, (uptr_t)textUtf8.length(), (sptr_t)textUtf8.constData());
}

void ScintillaIFace::addStyledText(int length, const QString &c) {
  QByteArray cUtf8 = c.toUtf8();
  send(SCI_ADDSTYLEDTEXT, (uptr_t)length, (sptr_t)cUtf8.constData());
}

void ScintillaIFace::insertText(int pos, const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  send(SCI_INSERTTEXT, (uptr_t)pos, (sptr_t)textUtf8.constData());
}

void ScintillaIFace::changeInsertion(const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  send(SCI_CHANGEINSERTION, (uptr_t)textUtf8.length(),
       (sptr_t)textUtf8.constData());
}

void ScintillaIFace::clearAll() { send(SCI_CLEARALL, 0, 0); }

void ScintillaIFace::deleteRange(int pos, int deleteLength) {
  send(SCI_DELETERANGE, (uptr_t)pos, (sptr_t)deleteLength);
}

void ScintillaIFace::clearDocumentStyle() {
  send(SCI_CLEARDOCUMENTSTYLE, 0, 0);
}

int ScintillaIFace::length() const { return (int)send(SCI_GETLENGTH, 0, 0); }

int ScintillaIFace::charAt(int pos) const {
  return (int)send(SCI_GETCHARAT, (uptr_t)pos, 0);
}

int ScintillaIFace::currentPos() const {
  return (int)send(SCI_GETCURRENTPOS, 0, 0);
}

int ScintillaIFace::anchor() const { return (int)send(SCI_GETANCHOR, 0, 0); }

int ScintillaIFace::styleAt(int pos) const {
  return (int)send(SCI_GETSTYLEAT, (uptr_t)pos, 0);
}

void ScintillaIFace::redo() { send(SCI_REDO, 0, 0); }

void ScintillaIFace::setUndoCollection(bool collectUndo) {
  send(SCI_SETUNDOCOLLECTION, (uptr_t)collectUndo, 0);
}

void ScintillaIFace::selectAll() { send(SCI_SELECTALL, 0, 0); }

void ScintillaIFace::setSavePoint() { send(SCI_SETSAVEPOINT, 0, 0); }

int ScintillaIFace::styledText(Sci_TextRange *tr) {
  return (int)send(SCI_GETSTYLEDTEXT, 0, (sptr_t)tr);
}

bool ScintillaIFace::canRedo() { return send(SCI_CANREDO, 0, 0) != 0; }

int ScintillaIFace::markerLineFromHandle(int handle) {
  return (int)send(SCI_MARKERLINEFROMHANDLE, (uptr_t)handle, 0);
}

void ScintillaIFace::markerDeleteHandle(int handle) {
  send(SCI_MARKERDELETEHANDLE, (uptr_t)handle, 0);
}

bool ScintillaIFace::undoCollection() const {
  return send(SCI_GETUNDOCOLLECTION, 0, 0) != 0;
}

int ScintillaIFace::viewWS() const { return (int)send(SCI_GETVIEWWS, 0, 0); }

void ScintillaIFace::setViewWS(int viewWS) {
  send(SCI_SETVIEWWS, (uptr_t)viewWS, 0);
}

int ScintillaIFace::positionFromPoint(int x, int y) {
  return (int)send(SCI_POSITIONFROMPOINT, (uptr_t)x, (sptr_t)y);
}

int ScintillaIFace::positionFromPointClose(int x, int y) {
  return (int)send(SCI_POSITIONFROMPOINTCLOSE, (uptr_t)x, (sptr_t)y);
}

void ScintillaIFace::gotoLine(int line) { send(SCI_GOTOLINE, (uptr_t)line, 0); }

void ScintillaIFace::gotoPos(int pos) { send(SCI_GOTOPOS, (uptr_t)pos, 0); }

void ScintillaIFace::setAnchor(int posAnchor) {
  send(SCI_SETANCHOR, (uptr_t)posAnchor, 0);
}

int ScintillaIFace::curLine(int length, char *text) {
  return (int)send(SCI_GETCURLINE, (uptr_t)length, (sptr_t)text);
}

int ScintillaIFace::endStyled() const {
  return (int)send(SCI_GETENDSTYLED, 0, 0);
}

void ScintillaIFace::convertEOLs(int eolMode) {
  send(SCI_CONVERTEOLS, (uptr_t)eolMode, 0);
}

int ScintillaIFace::eOLMode() const { return (int)send(SCI_GETEOLMODE, 0, 0); }

void ScintillaIFace::setEOLMode(int eolMode) {
  send(SCI_SETEOLMODE, (uptr_t)eolMode, 0);
}

void ScintillaIFace::startStyling(int pos) {
  send(SCI_STARTSTYLING, (uptr_t)pos, 0);
}

void ScintillaIFace::setStyling(int length, int style) {
  send(SCI_SETSTYLING, (uptr_t)length, (sptr_t)style);
}

bool ScintillaIFace::bufferedDraw() const {
  return send(SCI_GETBUFFEREDDRAW, 0, 0) != 0;
}

void ScintillaIFace::setBufferedDraw(bool buffered) {
  send(SCI_SETBUFFEREDDRAW, (uptr_t)buffered, 0);
}

void ScintillaIFace::setTabWidth(int tabWidth) {
  send(SCI_SETTABWIDTH, (uptr_t)tabWidth, 0);
}

int ScintillaIFace::tabWidth() const {
  return (int)send(SCI_GETTABWIDTH, 0, 0);
}

void ScintillaIFace::clearTabStops(int line) {
  send(SCI_CLEARTABSTOPS, (uptr_t)line, 0);
}

void ScintillaIFace::addTabStop(int line, int x) {
  send(SCI_ADDTABSTOP, (uptr_t)line, (sptr_t)x);
}

int ScintillaIFace::nextTabStop(int line, int x) {
  return (int)send(SCI_GETNEXTTABSTOP, (uptr_t)line, (sptr_t)x);
}

void ScintillaIFace::setCodePage(int codePage) {
  send(SCI_SETCODEPAGE, (uptr_t)codePage, 0);
}

int ScintillaIFace::iMEInteraction() const {
  return (int)send(SCI_GETIMEINTERACTION, 0, 0);
}

void ScintillaIFace::setIMEInteraction(int imeInteraction) {
  send(SCI_SETIMEINTERACTION, (uptr_t)imeInteraction, 0);
}

void ScintillaIFace::markerDefine(int markerNumber, int markerSymbol) {
  send(SCI_MARKERDEFINE, (uptr_t)markerNumber, (sptr_t)markerSymbol);
}

void ScintillaIFace::markerSetFore(int markerNumber, QColor fore) {
  send(SCI_MARKERSETFORE, (uptr_t)markerNumber, encodeColor(fore));
}

void ScintillaIFace::markerSetBack(int markerNumber, QColor back) {
  send(SCI_MARKERSETBACK, (uptr_t)markerNumber, encodeColor(back));
}

void ScintillaIFace::markerSetBackSelected(int markerNumber, QColor back) {
  send(SCI_MARKERSETBACKSELECTED, (uptr_t)markerNumber, encodeColor(back));
}

void ScintillaIFace::markerEnableHighlight(bool enabled) {
  send(SCI_MARKERENABLEHIGHLIGHT, (uptr_t)enabled, 0);
}

int ScintillaIFace::markerAdd(int line, int markerNumber) {
  return send(SCI_MARKERADD, line, markerNumber);
}

void ScintillaIFace::markerDelete(int line, int markerNumber) {
  send(SCI_MARKERDELETE, line, markerNumber);
}

void ScintillaIFace::markerDeleteAll(int markerNumber) {
  send(SCI_MARKERDELETEALL, markerNumber);
}

int ScintillaIFace::markers(int line) { return send(SCI_MARKERGET, line); }

int ScintillaIFace::markerNext(int lineStart, int markerMask) {
  return (int)send(SCI_MARKERNEXT, (uptr_t)lineStart, (sptr_t)markerMask);
}

int ScintillaIFace::markerPrevious(int lineStart, int markerMask) {
  return (int)send(SCI_MARKERPREVIOUS, (uptr_t)lineStart, (sptr_t)markerMask);
}

void ScintillaIFace::markerDefinePixmap(int markerNumber, const char *pixmap) {
  send(SCI_MARKERDEFINEPIXMAP, (uptr_t)markerNumber, (sptr_t)pixmap);
}

void ScintillaIFace::markerAddSet(int line, int set) {
  send(SCI_MARKERADDSET, (uptr_t)line, (sptr_t)set);
}

void ScintillaIFace::markerSetAlpha(int markerNumber, int alpha) {
  send(SCI_MARKERSETALPHA, (uptr_t)markerNumber, (sptr_t)alpha);
}

void ScintillaIFace::setMarginTypeN(int margin, int marginType) {
  send(SCI_SETMARGINTYPEN, (uptr_t)margin, (sptr_t)marginType);
}

int ScintillaIFace::marginTypeN(int margin) const {
  return (int)send(SCI_GETMARGINTYPEN, (uptr_t)margin, 0);
}

void ScintillaIFace::setMarginWidthN(int margin, int pixelWidth) {
  send(SCI_SETMARGINWIDTHN, (uptr_t)margin, (sptr_t)pixelWidth);
}

int ScintillaIFace::marginWidthN(int margin) const {
  return (int)send(SCI_GETMARGINWIDTHN, (uptr_t)margin, 0);
}

void ScintillaIFace::setMarginMaskN(int margin, int mask) {
  send(SCI_SETMARGINMASKN, (uptr_t)margin, (sptr_t)mask);
}

int ScintillaIFace::marginMaskN(int margin) const {
  return (int)send(SCI_GETMARGINMASKN, (uptr_t)margin, 0);
}

void ScintillaIFace::setMarginSensitiveN(int margin, bool sensitive) {
  send(SCI_SETMARGINSENSITIVEN, (uptr_t)margin, (sptr_t)sensitive);
}

bool ScintillaIFace::marginSensitiveN(int margin) const {
  return send(SCI_GETMARGINSENSITIVEN, (uptr_t)margin, 0) != 0;
}

void ScintillaIFace::setMarginCursorN(int margin, int cursor) {
  send(SCI_SETMARGINCURSORN, (uptr_t)margin, (sptr_t)cursor);
}

int ScintillaIFace::marginCursorN(int margin) const {
  return (int)send(SCI_GETMARGINCURSORN, (uptr_t)margin, 0);
}

void ScintillaIFace::styleClearAll() { send(SCI_STYLECLEARALL, 0, 0); }

void ScintillaIFace::styleSetFore(int style, QColor fore) {
  send(SCI_STYLESETFORE, (uptr_t)style, encodeColor(fore));
}

void ScintillaIFace::styleSetBack(int style, QColor back) {
  send(SCI_STYLESETBACK, (uptr_t)style, encodeColor(back));
}

void ScintillaIFace::styleSetFont(int style, const QFont &font) {
  send(SCI_STYLESETFONT, style, (sptr_t)font.family().toUtf8().constData());
  send(SCI_STYLESETSIZE, style, font.pointSize());
  send(SCI_STYLESETBOLD, style, font.bold());
  send(SCI_STYLESETITALIC, style, font.italic());
}

void ScintillaIFace::styleSetEOLFilled(int style, bool filled) {
  send(SCI_STYLESETEOLFILLED, (uptr_t)style, (sptr_t)filled);
}

void ScintillaIFace::styleResetDefault() { send(SCI_STYLERESETDEFAULT, 0, 0); }

void ScintillaIFace::styleSetUnderline(int style, bool underline) {
  send(SCI_STYLESETUNDERLINE, (uptr_t)style, (sptr_t)underline);
}

QColor ScintillaIFace::styleFore(int style) const {
  return decodeColor(send(SCI_STYLEGETFORE, (uptr_t)style, 0));
}

QColor ScintillaIFace::styleBack(int style) const {
  return decodeColor(send(SCI_STYLEGETBACK, (uptr_t)style, 0));
}

QFont ScintillaIFace::styleFont(int style) const {
  int len = send(SCI_STYLEGETFONT, style);
  QVarLengthArray<char, 32> buffer(len);
  send(SCI_STYLEGETFONT, style, (sptr_t)buffer.data());
  QFont font(QString::fromUtf8(buffer.constData(), len));
  font.setPointSize(send(SCI_STYLEGETSIZE, style));
  font.setBold(send(SCI_STYLEGETBOLD, style));
  font.setItalic(send(SCI_STYLEGETITALIC, style));
  return font;
}

bool ScintillaIFace::styleEOLFilled(int style) const {
  return send(SCI_STYLEGETEOLFILLED, (uptr_t)style, 0) != 0;
}

bool ScintillaIFace::styleUnderline(int style) const {
  return send(SCI_STYLEGETUNDERLINE, (uptr_t)style, 0) != 0;
}

int ScintillaIFace::styleCase(int style) const {
  return (int)send(SCI_STYLEGETCASE, (uptr_t)style, 0);
}

int ScintillaIFace::styleCharacterSet(int style) const {
  return (int)send(SCI_STYLEGETCHARACTERSET, (uptr_t)style, 0);
}

bool ScintillaIFace::styleVisible(int style) const {
  return send(SCI_STYLEGETVISIBLE, (uptr_t)style, 0) != 0;
}

bool ScintillaIFace::styleChangeable(int style) const {
  return send(SCI_STYLEGETCHANGEABLE, (uptr_t)style, 0) != 0;
}

bool ScintillaIFace::styleHotSpot(int style) const {
  return send(SCI_STYLEGETHOTSPOT, (uptr_t)style, 0) != 0;
}

void ScintillaIFace::styleSetCase(int style, int caseForce) {
  send(SCI_STYLESETCASE, (uptr_t)style, (sptr_t)caseForce);
}

void ScintillaIFace::styleSetSizeFractional(int style, int caseForce) {
  send(SCI_STYLESETSIZEFRACTIONAL, (uptr_t)style, (sptr_t)caseForce);
}

int ScintillaIFace::styleSizeFractional(int style) const {
  return (int)send(SCI_STYLEGETSIZEFRACTIONAL, (uptr_t)style, 0);
}

void ScintillaIFace::styleSetWeight(int style, int weight) {
  send(SCI_STYLESETWEIGHT, (uptr_t)style, (sptr_t)weight);
}

int ScintillaIFace::styleWeight(int style) const {
  return (int)send(SCI_STYLEGETWEIGHT, (uptr_t)style, 0);
}

void ScintillaIFace::styleSetCharacterSet(int style, int characterSet) {
  send(SCI_STYLESETCHARACTERSET, (uptr_t)style, (sptr_t)characterSet);
}

void ScintillaIFace::styleSetHotSpot(int style, bool hotspot) {
  send(SCI_STYLESETHOTSPOT, (uptr_t)style, (sptr_t)hotspot);
}

void ScintillaIFace::setSelFore(bool useSetting, QColor fore) {
  send(SCI_SETSELFORE, (uptr_t)useSetting, encodeColor(fore));
}

void ScintillaIFace::setSelBack(bool useSetting, QColor back) {
  send(SCI_SETSELBACK, (uptr_t)useSetting, encodeColor(back));
}

int ScintillaIFace::selAlpha() const {
  return (int)send(SCI_GETSELALPHA, 0, 0);
}

void ScintillaIFace::setSelAlpha(int alpha) {
  send(SCI_SETSELALPHA, (uptr_t)alpha, 0);
}

bool ScintillaIFace::selEOLFilled() const {
  return send(SCI_GETSELEOLFILLED, 0, 0) != 0;
}

void ScintillaIFace::setSelEOLFilled(bool filled) {
  send(SCI_SETSELEOLFILLED, (uptr_t)filled, 0);
}

void ScintillaIFace::setCaretFore(QColor fore) {
  send(SCI_SETCARETFORE, encodeColor(fore), 0);
}

void ScintillaIFace::assignCmdKey(int km, int msg) {
  send(SCI_ASSIGNCMDKEY, (uptr_t)km, (sptr_t)msg);
}

void ScintillaIFace::clearCmdKey(int km) {
  send(SCI_CLEARCMDKEY, (uptr_t)km, 0);
}

void ScintillaIFace::clearAllCmdKeys() { send(SCI_CLEARALLCMDKEYS, 0, 0); }

void ScintillaIFace::setStylingEx(int length, const QString &styles) {
  QByteArray stylesUtf8 = styles.toUtf8();
  send(SCI_SETSTYLINGEX, (uptr_t)length, (sptr_t)stylesUtf8.constData());
}

void ScintillaIFace::styleSetVisible(int style, bool visible) {
  send(SCI_STYLESETVISIBLE, (uptr_t)style, (sptr_t)visible);
}

int ScintillaIFace::caretPeriod() const {
  return (int)send(SCI_GETCARETPERIOD, 0, 0);
}

void ScintillaIFace::setCaretPeriod(int periodMilliseconds) {
  send(SCI_SETCARETPERIOD, (uptr_t)periodMilliseconds, 0);
}

void ScintillaIFace::setWordChars(const QByteArray &chars) {
  send(SCI_SETWORDCHARS, 0, (sptr_t)chars.constData());
}

QByteArray ScintillaIFace::wordChars() const {
  QByteArray chars(send(SCI_GETWORDCHARS), '\0');
  send(SCI_GETWORDCHARS, 0, (sptr_t)chars.data());
  return chars;
}

void ScintillaIFace::beginUndoAction() { send(SCI_BEGINUNDOACTION, 0, 0); }

void ScintillaIFace::endUndoAction() { send(SCI_ENDUNDOACTION, 0, 0); }

void ScintillaIFace::indicSetStyle(int indic, int style) {
  send(SCI_INDICSETSTYLE, (uptr_t)indic, (sptr_t)style);
}

int ScintillaIFace::indicStyle(int indic) const {
  return (int)send(SCI_INDICGETSTYLE, (uptr_t)indic, 0);
}

void ScintillaIFace::indicSetFore(int indic, QColor fore) {
  send(SCI_INDICSETFORE, (uptr_t)indic, encodeColor(fore));
}

QColor ScintillaIFace::indicFore(int indic) const {
  return decodeColor(send(SCI_INDICGETFORE, (uptr_t)indic, 0));
}

void ScintillaIFace::indicSetUnder(int indic, bool under) {
  send(SCI_INDICSETUNDER, (uptr_t)indic, (sptr_t)under);
}

bool ScintillaIFace::indicUnder(int indic) const {
  return send(SCI_INDICGETUNDER, (uptr_t)indic, 0) != 0;
}

void ScintillaIFace::indicSetHoverStyle(int indic, int style) {
  send(SCI_INDICSETHOVERSTYLE, (uptr_t)indic, (sptr_t)style);
}

int ScintillaIFace::indicHoverStyle(int indic) const {
  return (int)send(SCI_INDICGETHOVERSTYLE, (uptr_t)indic, 0);
}

void ScintillaIFace::indicSetHoverFore(int indic, QColor fore) {
  send(SCI_INDICSETHOVERFORE, (uptr_t)indic, encodeColor(fore));
}

QColor ScintillaIFace::indicHoverFore(int indic) const {
  return decodeColor(send(SCI_INDICGETHOVERFORE, (uptr_t)indic, 0));
}

void ScintillaIFace::indicSetFlags(int indic, int flags) {
  send(SCI_INDICSETFLAGS, (uptr_t)indic, (sptr_t)flags);
}

int ScintillaIFace::indicFlags(int indic) const {
  return (int)send(SCI_INDICGETFLAGS, (uptr_t)indic, 0);
}

void ScintillaIFace::setWhitespaceFore(bool useSetting, QColor fore) {
  send(SCI_SETWHITESPACEFORE, (uptr_t)useSetting, encodeColor(fore));
}

void ScintillaIFace::setWhitespaceBack(bool useSetting, QColor back) {
  send(SCI_SETWHITESPACEBACK, (uptr_t)useSetting, encodeColor(back));
}

void ScintillaIFace::setWhitespaceSize(int size) {
  send(SCI_SETWHITESPACESIZE, (uptr_t)size, 0);
}

int ScintillaIFace::whitespaceSize() const {
  return (int)send(SCI_GETWHITESPACESIZE, 0, 0);
}

void ScintillaIFace::setLineState(int line, int state) {
  send(SCI_SETLINESTATE, (uptr_t)line, (sptr_t)state);
}

int ScintillaIFace::lineState(int line) const {
  return (int)send(SCI_GETLINESTATE, (uptr_t)line, 0);
}

int ScintillaIFace::maxLineState() const {
  return (int)send(SCI_GETMAXLINESTATE, 0, 0);
}

bool ScintillaIFace::caretLineVisible() const {
  return send(SCI_GETCARETLINEVISIBLE, 0, 0) != 0;
}

void ScintillaIFace::setCaretLineVisible(bool show) {
  send(SCI_SETCARETLINEVISIBLE, (uptr_t)show, 0);
}

QColor ScintillaIFace::caretLineBack() const {
  return decodeColor(send(SCI_GETCARETLINEBACK, 0, 0));
}

void ScintillaIFace::setCaretLineBack(QColor back) {
  send(SCI_SETCARETLINEBACK, encodeColor(back), 0);
}

void ScintillaIFace::styleSetChangeable(int style, bool changeable) {
  send(SCI_STYLESETCHANGEABLE, (uptr_t)style, (sptr_t)changeable);
}

void ScintillaIFace::autoCShow(int lenEntered, const QString &itemList) {
  QByteArray itemListUtf8 = itemList.toUtf8();
  send(SCI_AUTOCSHOW, (uptr_t)lenEntered, (sptr_t)itemListUtf8.constData());
}

void ScintillaIFace::autoCCancel() { send(SCI_AUTOCCANCEL, 0, 0); }

bool ScintillaIFace::autoCActive() { return send(SCI_AUTOCACTIVE, 0, 0) != 0; }

int ScintillaIFace::autoCPosStart() {
  return (int)send(SCI_AUTOCPOSSTART, 0, 0);
}

void ScintillaIFace::autoCComplete() { send(SCI_AUTOCCOMPLETE, 0, 0); }

void ScintillaIFace::autoCStops(const QString &characterSet) {
  QByteArray characterSetUtf8 = characterSet.toUtf8();
  send(SCI_AUTOCSTOPS, 0, (sptr_t)characterSetUtf8.constData());
}

void ScintillaIFace::autoCSetSeparator(int separatorCharacter) {
  send(SCI_AUTOCSETSEPARATOR, (uptr_t)separatorCharacter, 0);
}

int ScintillaIFace::autoCSeparator() const {
  return (int)send(SCI_AUTOCGETSEPARATOR, 0, 0);
}

void ScintillaIFace::autoCSelect(const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  send(SCI_AUTOCSELECT, 0, (sptr_t)textUtf8.constData());
}

void ScintillaIFace::autoCSetCancelAtStart(bool cancel) {
  send(SCI_AUTOCSETCANCELATSTART, (uptr_t)cancel, 0);
}

bool ScintillaIFace::autoCCancelAtStart() const {
  return send(SCI_AUTOCGETCANCELATSTART, 0, 0) != 0;
}

void ScintillaIFace::autoCSetFillUps(const QString &characterSet) {
  QByteArray characterSetUtf8 = characterSet.toUtf8();
  send(SCI_AUTOCSETFILLUPS, 0, (sptr_t)characterSetUtf8.constData());
}

void ScintillaIFace::autoCSetChooseSingle(bool chooseSingle) {
  send(SCI_AUTOCSETCHOOSESINGLE, (uptr_t)chooseSingle, 0);
}

bool ScintillaIFace::autoCChooseSingle() const {
  return send(SCI_AUTOCGETCHOOSESINGLE, 0, 0) != 0;
}

void ScintillaIFace::autoCSetIgnoreCase(bool ignoreCase) {
  send(SCI_AUTOCSETIGNORECASE, (uptr_t)ignoreCase, 0);
}

bool ScintillaIFace::autoCIgnoreCase() const {
  return send(SCI_AUTOCGETIGNORECASE, 0, 0) != 0;
}

void ScintillaIFace::userListShow(int listType, const QString &itemList) {
  QByteArray itemListUtf8 = itemList.toUtf8();
  send(SCI_USERLISTSHOW, (uptr_t)listType, (sptr_t)itemListUtf8.constData());
}

void ScintillaIFace::autoCSetAutoHide(bool autoHide) {
  send(SCI_AUTOCSETAUTOHIDE, (uptr_t)autoHide, 0);
}

bool ScintillaIFace::autoCAutoHide() const {
  return send(SCI_AUTOCGETAUTOHIDE, 0, 0) != 0;
}

void ScintillaIFace::autoCSetDropRestOfWord(bool dropRestOfWord) {
  send(SCI_AUTOCSETDROPRESTOFWORD, (uptr_t)dropRestOfWord, 0);
}

bool ScintillaIFace::autoCDropRestOfWord() const {
  return send(SCI_AUTOCGETDROPRESTOFWORD, 0, 0) != 0;
}

void ScintillaIFace::registerImage(int type, const char *xpmData) {
  send(SCI_REGISTERIMAGE, (uptr_t)type, (sptr_t)xpmData);
}

void ScintillaIFace::clearRegisteredImages() {
  send(SCI_CLEARREGISTEREDIMAGES, 0, 0);
}

int ScintillaIFace::autoCTypeSeparator() const {
  return (int)send(SCI_AUTOCGETTYPESEPARATOR, 0, 0);
}

void ScintillaIFace::autoCSetTypeSeparator(int separatorCharacter) {
  send(SCI_AUTOCSETTYPESEPARATOR, (uptr_t)separatorCharacter, 0);
}

void ScintillaIFace::autoCSetMaxWidth(int characterCount) {
  send(SCI_AUTOCSETMAXWIDTH, (uptr_t)characterCount, 0);
}

int ScintillaIFace::autoCMaxWidth() const {
  return (int)send(SCI_AUTOCGETMAXWIDTH, 0, 0);
}

void ScintillaIFace::autoCSetMaxHeight(int rowCount) {
  send(SCI_AUTOCSETMAXHEIGHT, (uptr_t)rowCount, 0);
}

int ScintillaIFace::autoCMaxHeight() const {
  return (int)send(SCI_AUTOCGETMAXHEIGHT, 0, 0);
}

void ScintillaIFace::setIndent(int indentSize) {
  send(SCI_SETINDENT, (uptr_t)indentSize, 0);
}

int ScintillaIFace::indent() const { return (int)send(SCI_GETINDENT, 0, 0); }

void ScintillaIFace::setUseTabs(bool useTabs) {
  send(SCI_SETUSETABS, (uptr_t)useTabs, 0);
}

bool ScintillaIFace::useTabs() const { return send(SCI_GETUSETABS, 0, 0) != 0; }

void ScintillaIFace::setLineIndentation(int line, int indentSize) {
  send(SCI_SETLINEINDENTATION, (uptr_t)line, (sptr_t)indentSize);
}

int ScintillaIFace::lineIndentation(int line) const {
  return (int)send(SCI_GETLINEINDENTATION, (uptr_t)line, 0);
}

int ScintillaIFace::lineIndentPosition(int line) const {
  return (int)send(SCI_GETLINEINDENTPOSITION, (uptr_t)line, 0);
}

int ScintillaIFace::column(int pos) const { return send(SCI_GETCOLUMN, pos); }

int ScintillaIFace::countCharacters(int startPos, int endPos) {
  return (int)send(SCI_COUNTCHARACTERS, (uptr_t)startPos, (sptr_t)endPos);
}

void ScintillaIFace::setHScrollBar(bool show) {
  send(SCI_SETHSCROLLBAR, (uptr_t)show, 0);
}

bool ScintillaIFace::hScrollBar() const {
  return send(SCI_GETHSCROLLBAR, 0, 0) != 0;
}

void ScintillaIFace::setIndentationGuides(int indentView) {
  send(SCI_SETINDENTATIONGUIDES, (uptr_t)indentView, 0);
}

int ScintillaIFace::indentationGuides() const {
  return (int)send(SCI_GETINDENTATIONGUIDES, 0, 0);
}

void ScintillaIFace::setHighlightGuide(int column) {
  send(SCI_SETHIGHLIGHTGUIDE, (uptr_t)column, 0);
}

int ScintillaIFace::highlightGuide() const {
  return (int)send(SCI_GETHIGHLIGHTGUIDE, 0, 0);
}

int ScintillaIFace::lineEndPosition(int line) const {
  return (int)send(SCI_GETLINEENDPOSITION, (uptr_t)line, 0);
}

int ScintillaIFace::codePage() const {
  return (int)send(SCI_GETCODEPAGE, 0, 0);
}

QColor ScintillaIFace::caretFore() const {
  return decodeColor(send(SCI_GETCARETFORE, 0, 0));
}

bool ScintillaIFace::readOnly() const {
  return send(SCI_GETREADONLY, 0, 0) != 0;
}

void ScintillaIFace::setCurrentPos(int pos) {
  send(SCI_SETCURRENTPOS, (uptr_t)pos, 0);
}

void ScintillaIFace::setSelectionStart(int pos) {
  send(SCI_SETSELECTIONSTART, (uptr_t)pos, 0);
}

int ScintillaIFace::selectionStart() const {
  return (int)send(SCI_GETSELECTIONSTART, 0, 0);
}

void ScintillaIFace::setSelectionEnd(int pos) {
  send(SCI_SETSELECTIONEND, (uptr_t)pos, 0);
}

int ScintillaIFace::selectionEnd() const {
  return (int)send(SCI_GETSELECTIONEND, 0, 0);
}

void ScintillaIFace::setEmptySelection(int pos) {
  send(SCI_SETEMPTYSELECTION, (uptr_t)pos, 0);
}

void ScintillaIFace::setPrintMagnification(int magnification) {
  send(SCI_SETPRINTMAGNIFICATION, (uptr_t)magnification, 0);
}

int ScintillaIFace::printMagnification() const {
  return (int)send(SCI_GETPRINTMAGNIFICATION, 0, 0);
}

void ScintillaIFace::setPrintColourMode(int mode) {
  send(SCI_SETPRINTCOLOURMODE, (uptr_t)mode, 0);
}

int ScintillaIFace::printColourMode() const {
  return (int)send(SCI_GETPRINTCOLOURMODE, 0, 0);
}

QPair<int, int> ScintillaIFace::findText(int flags, const char *text, int min,
                                         int max) {
  Sci_TextToFind ft = {{min, max}, text, {min, max}};
  int pos = send(SCI_FINDTEXT, flags, (sptr_t)&ft);
  return QPair<int, int>(pos, ft.chrgText.cpMax);
}

int ScintillaIFace::formatRange(bool draw, Sci_RangeToFormat *fr) {
  return (int)send(SCI_FORMATRANGE, (uptr_t)draw, (sptr_t)fr);
}

int ScintillaIFace::firstVisibleLine() const {
  return (int)send(SCI_GETFIRSTVISIBLELINE, 0, 0);
}

QString ScintillaIFace::line(int line) {
  int len = send(SCI_GETLINE, line);
  QVarLengthArray<char, 128> buffer(len);
  send(SCI_GETLINE, line, (sptr_t)buffer.data());
  return QString::fromUtf8(buffer.constData(), len);
}

int ScintillaIFace::lineCount() const {
  return (int)send(SCI_GETLINECOUNT, 0, 0);
}

void ScintillaIFace::setMarginLeft(int pixelWidth) {
  send(SCI_SETMARGINLEFT, 0, (sptr_t)pixelWidth);
}

int ScintillaIFace::marginLeft() const {
  return (int)send(SCI_GETMARGINLEFT, 0, 0);
}

void ScintillaIFace::setMarginRight(int pixelWidth) {
  send(SCI_SETMARGINRIGHT, 0, (sptr_t)pixelWidth);
}

int ScintillaIFace::marginRight() const {
  return (int)send(SCI_GETMARGINRIGHT, 0, 0);
}

bool ScintillaIFace::isModified() const {
  return send(SCI_GETMODIFY, 0, 0) != 0;
}

void ScintillaIFace::setSel(int start, int end) {
  send(SCI_SETSEL, (uptr_t)start, (sptr_t)end);
}

QString ScintillaIFace::selText() {
  int len = send(SCI_GETSELTEXT, 0, 0);
  QVarLengthArray<char, 1024> buffer(len);
  send(SCI_GETSELTEXT, 0, (sptr_t)buffer.data());
  return QString::fromUtf8(buffer.constData(), len);
}

QByteArray ScintillaIFace::textRange(int min, int max) const {
  int len = max - min + 1;
  QVarLengthArray<char, 1024> buffer(len);

  Sci_TextRange range;
  range.chrg.cpMin = min;
  range.chrg.cpMax = max;
  range.lpstrText = buffer.data();

  send(SCI_GETTEXTRANGE, 0, (sptr_t)&range);
  return QByteArray(buffer.constData(), len - 1);
}

void ScintillaIFace::hideSelection(bool normal) {
  send(SCI_HIDESELECTION, (uptr_t)normal, 0);
}

QPoint ScintillaIFace::pointFromPosition(int pos) {
  int x = send(SCI_POINTXFROMPOSITION, 0, (sptr_t)pos);
  int y = send(SCI_POINTYFROMPOSITION, 0, (sptr_t)pos);
  return QPoint(x, y);
}

int ScintillaIFace::lineFromPosition(int pos) {
  return send(SCI_LINEFROMPOSITION, pos, 0);
}

int ScintillaIFace::positionFromLine(int line) {
  return send(SCI_POSITIONFROMLINE, line, 0);
}

void ScintillaIFace::lineScroll(int columns, int lines) {
  send(SCI_LINESCROLL, (uptr_t)columns, (sptr_t)lines);
}

void ScintillaIFace::scrollCaret() { send(SCI_SCROLLCARET, 0, 0); }

void ScintillaIFace::scrollRange(int secondary, int primary) {
  send(SCI_SCROLLRANGE, (uptr_t)secondary, (sptr_t)primary);
}

void ScintillaIFace::replaceSelection(const QString &text) {
  send(SCI_REPLACESEL, 0, (sptr_t)text.toUtf8().constData());
}

void ScintillaIFace::setReadOnly(bool readOnly) {
  send(SCI_SETREADONLY, readOnly);
}

void ScintillaIFace::null() { send(SCI_NULL, 0, 0); }

bool ScintillaIFace::canPaste() { return send(SCI_CANPASTE, 0, 0) != 0; }

bool ScintillaIFace::canUndo() { return send(SCI_CANUNDO, 0, 0) != 0; }

void ScintillaIFace::emptyUndoBuffer() { send(SCI_EMPTYUNDOBUFFER, 0, 0); }

void ScintillaIFace::undo() { send(SCI_UNDO, 0, 0); }

void ScintillaIFace::cut() { send(SCI_CUT, 0, 0); }

void ScintillaIFace::copy() { send(SCI_COPY, 0, 0); }

void ScintillaIFace::paste() { send(SCI_PASTE, 0, 0); }

void ScintillaIFace::clear() { send(SCI_CLEAR, 0, 0); }

void ScintillaIFace::setText(const QString &text) {
  send(SCI_SETTEXT, 0, (sptr_t)text.toUtf8().constData());
}

QString ScintillaIFace::text() const {
  return QString::fromUtf8(characterPointer(), length());
}

uintptr_t ScintillaIFace::directFunction() const {
  return (uintptr_t)send(SCI_GETDIRECTFUNCTION);
}

uintptr_t ScintillaIFace::directPointer() const {
  return (uintptr_t)send(SCI_GETDIRECTPOINTER);
}

void ScintillaIFace::setOvertype(bool overtype) {
  send(SCI_SETOVERTYPE, (uptr_t)overtype, 0);
}

bool ScintillaIFace::overtype() const {
  return send(SCI_GETOVERTYPE, 0, 0) != 0;
}

void ScintillaIFace::setCaretWidth(int pixelWidth) {
  send(SCI_SETCARETWIDTH, (uptr_t)pixelWidth, 0);
}

int ScintillaIFace::caretWidth() const {
  return (int)send(SCI_GETCARETWIDTH, 0, 0);
}

void ScintillaIFace::setTargetStart(int pos) {
  send(SCI_SETTARGETSTART, (uptr_t)pos, 0);
}

int ScintillaIFace::targetStart() const {
  return (int)send(SCI_GETTARGETSTART, 0, 0);
}

void ScintillaIFace::setTargetEnd(int pos) {
  send(SCI_SETTARGETEND, (uptr_t)pos, 0);
}

int ScintillaIFace::targetEnd() const {
  return (int)send(SCI_GETTARGETEND, 0, 0);
}

void ScintillaIFace::setTargetRange(int start, int end) {
  send(SCI_SETTARGETRANGE, (uptr_t)start, (sptr_t)end);
}

int ScintillaIFace::targetText(char *characters) const {
  return (int)send(SCI_GETTARGETTEXT, 0, (sptr_t)characters);
}

void ScintillaIFace::targetFromSelection() {
  send(SCI_TARGETFROMSELECTION, 0, 0);
}

void ScintillaIFace::targetWholeDocument() {
  send(SCI_TARGETWHOLEDOCUMENT, 0, 0);
}

int ScintillaIFace::replaceTarget(const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  return (int)send(SCI_REPLACETARGET, (uptr_t)textUtf8.length(),
                   (sptr_t)textUtf8.constData());
}

int ScintillaIFace::replaceTargetRE(const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  return (int)send(SCI_REPLACETARGETRE, (uptr_t)textUtf8.length(),
                   (sptr_t)textUtf8.constData());
}

int ScintillaIFace::searchInTarget(const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  return (int)send(SCI_SEARCHINTARGET, (uptr_t)textUtf8.length(),
                   (sptr_t)textUtf8.constData());
}

void ScintillaIFace::setSearchFlags(int flags) {
  send(SCI_SETSEARCHFLAGS, (uptr_t)flags, 0);
}

int ScintillaIFace::searchFlags() const {
  return (int)send(SCI_GETSEARCHFLAGS, 0, 0);
}

void ScintillaIFace::callTipShow(int pos, const QString &definition) {
  QByteArray definitionUtf8 = definition.toUtf8();
  send(SCI_CALLTIPSHOW, (uptr_t)pos, (sptr_t)definitionUtf8.constData());
}

void ScintillaIFace::callTipCancel() { send(SCI_CALLTIPCANCEL, 0, 0); }

bool ScintillaIFace::callTipActive() {
  return send(SCI_CALLTIPACTIVE, 0, 0) != 0;
}

int ScintillaIFace::callTipPosStart() {
  return (int)send(SCI_CALLTIPPOSSTART, 0, 0);
}

void ScintillaIFace::callTipSetPosStart(int posStart) {
  send(SCI_CALLTIPSETPOSSTART, (uptr_t)posStart, 0);
}

void ScintillaIFace::callTipSetHlt(int start, int end) {
  send(SCI_CALLTIPSETHLT, (uptr_t)start, (sptr_t)end);
}

void ScintillaIFace::callTipSetBack(QColor back) {
  send(SCI_CALLTIPSETBACK, encodeColor(back), 0);
}

void ScintillaIFace::callTipSetFore(QColor fore) {
  send(SCI_CALLTIPSETFORE, encodeColor(fore), 0);
}

void ScintillaIFace::callTipSetForeHlt(QColor fore) {
  send(SCI_CALLTIPSETFOREHLT, encodeColor(fore), 0);
}

void ScintillaIFace::callTipUseStyle(int tabSize) {
  send(SCI_CALLTIPUSESTYLE, (uptr_t)tabSize, 0);
}

void ScintillaIFace::callTipSetPosition(bool above) {
  send(SCI_CALLTIPSETPOSITION, (uptr_t)above, 0);
}

int ScintillaIFace::visibleFromDocLine(int line) {
  return (int)send(SCI_VISIBLEFROMDOCLINE, (uptr_t)line, 0);
}

int ScintillaIFace::docLineFromVisible(int lineDisplay) {
  return (int)send(SCI_DOCLINEFROMVISIBLE, (uptr_t)lineDisplay, 0);
}

int ScintillaIFace::wrapCount(int line) {
  return (int)send(SCI_WRAPCOUNT, (uptr_t)line, 0);
}

void ScintillaIFace::setFoldLevel(int line, int level) {
  send(SCI_SETFOLDLEVEL, (uptr_t)line, (sptr_t)level);
}

int ScintillaIFace::foldLevel(int line) const {
  return (int)send(SCI_GETFOLDLEVEL, (uptr_t)line, 0);
}

int ScintillaIFace::lastChild(int line, int level) const {
  return (int)send(SCI_GETLASTCHILD, (uptr_t)line, (sptr_t)level);
}

int ScintillaIFace::foldParent(int line) const {
  return (int)send(SCI_GETFOLDPARENT, (uptr_t)line, 0);
}

void ScintillaIFace::showLines(int lineStart, int lineEnd) {
  send(SCI_SHOWLINES, (uptr_t)lineStart, (sptr_t)lineEnd);
}

void ScintillaIFace::hideLines(int lineStart, int lineEnd) {
  send(SCI_HIDELINES, (uptr_t)lineStart, (sptr_t)lineEnd);
}

bool ScintillaIFace::lineVisible(int line) const {
  return send(SCI_GETLINEVISIBLE, (uptr_t)line, 0) != 0;
}

bool ScintillaIFace::allLinesVisible() const {
  return send(SCI_GETALLLINESVISIBLE, 0, 0) != 0;
}

void ScintillaIFace::setFoldExpanded(int line, bool expanded) {
  send(SCI_SETFOLDEXPANDED, (uptr_t)line, (sptr_t)expanded);
}

bool ScintillaIFace::foldExpanded(int line) const {
  return send(SCI_GETFOLDEXPANDED, (uptr_t)line, 0) != 0;
}

void ScintillaIFace::toggleFold(int line) {
  send(SCI_TOGGLEFOLD, (uptr_t)line, 0);
}

void ScintillaIFace::foldLine(int line, int action) {
  send(SCI_FOLDLINE, (uptr_t)line, (sptr_t)action);
}

void ScintillaIFace::foldChildren(int line, int action) {
  send(SCI_FOLDCHILDREN, (uptr_t)line, (sptr_t)action);
}

void ScintillaIFace::expandChildren(int line, int level) {
  send(SCI_EXPANDCHILDREN, (uptr_t)line, (sptr_t)level);
}

void ScintillaIFace::foldAll(int action) {
  send(SCI_FOLDALL, (uptr_t)action, 0);
}

void ScintillaIFace::ensureVisible(int line) {
  send(SCI_ENSUREVISIBLE, (uptr_t)line, 0);
}

void ScintillaIFace::setAutomaticFold(int automaticFold) {
  send(SCI_SETAUTOMATICFOLD, (uptr_t)automaticFold, 0);
}

int ScintillaIFace::automaticFold() const {
  return (int)send(SCI_GETAUTOMATICFOLD, 0, 0);
}

void ScintillaIFace::setFoldFlags(int flags) {
  send(SCI_SETFOLDFLAGS, (uptr_t)flags, 0);
}

void ScintillaIFace::ensureVisibleEnforcePolicy(int line) {
  send(SCI_ENSUREVISIBLEENFORCEPOLICY, (uptr_t)line, 0);
}

void ScintillaIFace::setTabIndents(bool tabIndents) {
  send(SCI_SETTABINDENTS, (uptr_t)tabIndents, 0);
}

bool ScintillaIFace::tabIndents() const {
  return send(SCI_GETTABINDENTS, 0, 0) != 0;
}

void ScintillaIFace::setBackSpaceUnIndents(bool bsUnIndents) {
  send(SCI_SETBACKSPACEUNINDENTS, (uptr_t)bsUnIndents, 0);
}

bool ScintillaIFace::backSpaceUnIndents() const {
  return send(SCI_GETBACKSPACEUNINDENTS, 0, 0) != 0;
}

void ScintillaIFace::setMouseDwellTime(int periodMilliseconds) {
  send(SCI_SETMOUSEDWELLTIME, (uptr_t)periodMilliseconds, 0);
}

int ScintillaIFace::mouseDwellTime() const {
  return (int)send(SCI_GETMOUSEDWELLTIME, 0, 0);
}

int ScintillaIFace::wordStartPosition(int pos, bool onlyWordCharacters) const {
  return (int)send(SCI_WORDSTARTPOSITION, (uptr_t)pos,
                   (sptr_t)onlyWordCharacters);
}

int ScintillaIFace::wordEndPosition(int pos, bool onlyWordCharacters) const {
  return (int)send(SCI_WORDENDPOSITION, (uptr_t)pos,
                   (sptr_t)onlyWordCharacters);
}

bool ScintillaIFace::isRangeWord(int start, int end) {
  return send(SCI_ISRANGEWORD, (uptr_t)start, (sptr_t)end) != 0;
}

void ScintillaIFace::setWrapMode(int mode) {
  send(SCI_SETWRAPMODE, (uptr_t)mode, 0);
}

int ScintillaIFace::wrapMode() const {
  return (int)send(SCI_GETWRAPMODE, 0, 0);
}

void ScintillaIFace::setWrapVisualFlags(int wrapVisualFlags) {
  send(SCI_SETWRAPVISUALFLAGS, (uptr_t)wrapVisualFlags, 0);
}

int ScintillaIFace::wrapVisualFlags() const {
  return (int)send(SCI_GETWRAPVISUALFLAGS, 0, 0);
}

void ScintillaIFace::setWrapVisualFlagsLocation(int wrapVisualFlagsLocation) {
  send(SCI_SETWRAPVISUALFLAGSLOCATION, (uptr_t)wrapVisualFlagsLocation, 0);
}

int ScintillaIFace::wrapVisualFlagsLocation() const {
  return (int)send(SCI_GETWRAPVISUALFLAGSLOCATION, 0, 0);
}

void ScintillaIFace::setWrapStartIndent(int indent) {
  send(SCI_SETWRAPSTARTINDENT, (uptr_t)indent, 0);
}

int ScintillaIFace::wrapStartIndent() const {
  return (int)send(SCI_GETWRAPSTARTINDENT, 0, 0);
}

void ScintillaIFace::setWrapIndentMode(int mode) {
  send(SCI_SETWRAPINDENTMODE, (uptr_t)mode, 0);
}

int ScintillaIFace::wrapIndentMode() const {
  return (int)send(SCI_GETWRAPINDENTMODE, 0, 0);
}

void ScintillaIFace::setLayoutCache(int mode) {
  send(SCI_SETLAYOUTCACHE, (uptr_t)mode, 0);
}

int ScintillaIFace::layoutCache() const {
  return (int)send(SCI_GETLAYOUTCACHE, 0, 0);
}

void ScintillaIFace::setScrollWidth(int pixelWidth) {
  send(SCI_SETSCROLLWIDTH, (uptr_t)pixelWidth, 0);
}

int ScintillaIFace::scrollWidth() const {
  return (int)send(SCI_GETSCROLLWIDTH, 0, 0);
}

void ScintillaIFace::setScrollWidthTracking(bool tracking) {
  send(SCI_SETSCROLLWIDTHTRACKING, (uptr_t)tracking, 0);
}

bool ScintillaIFace::scrollWidthTracking() const {
  return send(SCI_GETSCROLLWIDTHTRACKING, 0, 0) != 0;
}

int ScintillaIFace::textWidth(int style, const QString &text) {
  return (int)send(SCI_TEXTWIDTH, style, (sptr_t)text.toUtf8().constData());
}

void ScintillaIFace::setEndAtLastLine(bool endAtLastLine) {
  send(SCI_SETENDATLASTLINE, (uptr_t)endAtLastLine, 0);
}

bool ScintillaIFace::endAtLastLine() const {
  return send(SCI_GETENDATLASTLINE, 0, 0) != 0;
}

int ScintillaIFace::textHeight(int line) {
  return (int)send(SCI_TEXTHEIGHT, (uptr_t)line, 0);
}

int ScintillaIFace::fontPointSize(int line) {
  return (int)send(SCI_STYLEGETSIZE, (uptr_t)line, 0);
}

void ScintillaIFace::setVScrollBar(bool show) {
  send(SCI_SETVSCROLLBAR, (uptr_t)show, 0);
}

bool ScintillaIFace::vScrollBar() const {
  return send(SCI_GETVSCROLLBAR, 0, 0) != 0;
}

void ScintillaIFace::appendText(const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  send(SCI_APPENDTEXT, (uptr_t)textUtf8.length(), (sptr_t)textUtf8.constData());
}

bool ScintillaIFace::twoPhaseDraw() const {
  return send(SCI_GETPHASESDRAW, 0, 0) != 0;
}

void ScintillaIFace::setTwoPhaseDraw(bool twoPhase) {
  int phases = 1;
  if (twoPhase)
    phases = 2;

  send(SCI_SETPHASESDRAW, phases, 0);
}

int ScintillaIFace::phasesDraw() const {
  return (int)send(SCI_GETPHASESDRAW, 0, 0);
}

void ScintillaIFace::setPhasesDraw(int phases) {
  send(SCI_SETPHASESDRAW, (uptr_t)phases, 0);
}

void ScintillaIFace::setFontQuality(int fontQuality) {
  send(SCI_SETFONTQUALITY, (uptr_t)fontQuality, 0);
}

int ScintillaIFace::fontQuality() const {
  return (int)send(SCI_GETFONTQUALITY, 0, 0);
}

void ScintillaIFace::setFirstVisibleLine(int lineDisplay) {
  send(SCI_SETFIRSTVISIBLELINE, (uptr_t)lineDisplay, 0);
}

void ScintillaIFace::setMultiPaste(int multiPaste) {
  send(SCI_SETMULTIPASTE, (uptr_t)multiPaste, 0);
}

int ScintillaIFace::multiPaste() const {
  return (int)send(SCI_GETMULTIPASTE, 0, 0);
}

int ScintillaIFace::tag(int tagNumber, char *tagValue) const {
  return (int)send(SCI_GETTAG, (uptr_t)tagNumber, (sptr_t)tagValue);
}

void ScintillaIFace::linesJoin() { send(SCI_LINESJOIN, 0, 0); }

void ScintillaIFace::linesSplit(int pixelWidth) {
  send(SCI_LINESSPLIT, (uptr_t)pixelWidth, 0);
}

void ScintillaIFace::setFoldMarginColour(bool useSetting, QColor back) {
  send(SCI_SETFOLDMARGINCOLOUR, (uptr_t)useSetting, encodeColor(back));
}

void ScintillaIFace::setFoldMarginHiColour(bool useSetting, QColor fore) {
  send(SCI_SETFOLDMARGINHICOLOUR, (uptr_t)useSetting, encodeColor(fore));
}

void ScintillaIFace::lineDown() { send(SCI_LINEDOWN, 0, 0); }

void ScintillaIFace::lineDownExtend() { send(SCI_LINEDOWNEXTEND, 0, 0); }

void ScintillaIFace::lineUp() { send(SCI_LINEUP, 0, 0); }

void ScintillaIFace::lineUpExtend() { send(SCI_LINEUPEXTEND, 0, 0); }

void ScintillaIFace::charLeft() { send(SCI_CHARLEFT, 0, 0); }

void ScintillaIFace::charLeftExtend() { send(SCI_CHARLEFTEXTEND, 0, 0); }

void ScintillaIFace::charRight() { send(SCI_CHARRIGHT, 0, 0); }

void ScintillaIFace::charRightExtend() { send(SCI_CHARRIGHTEXTEND, 0, 0); }

void ScintillaIFace::wordLeft() { send(SCI_WORDLEFT, 0, 0); }

void ScintillaIFace::wordLeftExtend() { send(SCI_WORDLEFTEXTEND, 0, 0); }

void ScintillaIFace::wordRight() { send(SCI_WORDRIGHT, 0, 0); }

void ScintillaIFace::wordRightExtend() { send(SCI_WORDRIGHTEXTEND, 0, 0); }

void ScintillaIFace::home() { send(SCI_HOME, 0, 0); }

void ScintillaIFace::homeExtend() { send(SCI_HOMEEXTEND, 0, 0); }

void ScintillaIFace::lineEnd() { send(SCI_LINEEND, 0, 0); }

void ScintillaIFace::lineEndExtend() { send(SCI_LINEENDEXTEND, 0, 0); }

void ScintillaIFace::documentStart() { send(SCI_DOCUMENTSTART, 0, 0); }

void ScintillaIFace::documentStartExtend() {
  send(SCI_DOCUMENTSTARTEXTEND, 0, 0);
}

void ScintillaIFace::documentEnd() { send(SCI_DOCUMENTEND, 0, 0); }

void ScintillaIFace::documentEndExtend() { send(SCI_DOCUMENTENDEXTEND, 0, 0); }

void ScintillaIFace::pageUp() { send(SCI_PAGEUP, 0, 0); }

void ScintillaIFace::pageUpExtend() { send(SCI_PAGEUPEXTEND, 0, 0); }

void ScintillaIFace::pageDown() { send(SCI_PAGEDOWN, 0, 0); }

void ScintillaIFace::pageDownExtend() { send(SCI_PAGEDOWNEXTEND, 0, 0); }

void ScintillaIFace::editToggleOvertype() {
  send(SCI_EDITTOGGLEOVERTYPE, 0, 0);
}

void ScintillaIFace::cancel() { send(SCI_CANCEL, 0, 0); }

void ScintillaIFace::deleteBack() { send(SCI_DELETEBACK, 0, 0); }

void ScintillaIFace::tab() { send(SCI_TAB, 0, 0); }

void ScintillaIFace::backTab() { send(SCI_BACKTAB, 0, 0); }

void ScintillaIFace::newLine() { send(SCI_NEWLINE, 0, 0); }

void ScintillaIFace::formFeed() { send(SCI_FORMFEED, 0, 0); }

void ScintillaIFace::vCHome() { send(SCI_VCHOME, 0, 0); }

void ScintillaIFace::vCHomeExtend() { send(SCI_VCHOMEEXTEND, 0, 0); }

void ScintillaIFace::zoomIn() { send(SCI_ZOOMIN, 0, 0); }

void ScintillaIFace::zoomOut() { send(SCI_ZOOMOUT, 0, 0); }

void ScintillaIFace::delWordLeft() { send(SCI_DELWORDLEFT, 0, 0); }

void ScintillaIFace::delWordRight() { send(SCI_DELWORDRIGHT, 0, 0); }

void ScintillaIFace::delWordRightEnd() { send(SCI_DELWORDRIGHTEND, 0, 0); }

void ScintillaIFace::lineCut() { send(SCI_LINECUT, 0, 0); }

void ScintillaIFace::lineDelete() { send(SCI_LINEDELETE, 0, 0); }

void ScintillaIFace::lineTranspose() { send(SCI_LINETRANSPOSE, 0, 0); }

void ScintillaIFace::lineDuplicate() { send(SCI_LINEDUPLICATE, 0, 0); }

void ScintillaIFace::lowerCase() { send(SCI_LOWERCASE, 0, 0); }

void ScintillaIFace::upperCase() { send(SCI_UPPERCASE, 0, 0); }

void ScintillaIFace::lineScrollDown() { send(SCI_LINESCROLLDOWN, 0, 0); }

void ScintillaIFace::lineScrollUp() { send(SCI_LINESCROLLUP, 0, 0); }

void ScintillaIFace::deleteBackNotLine() { send(SCI_DELETEBACKNOTLINE, 0, 0); }

void ScintillaIFace::homeDisplay() { send(SCI_HOMEDISPLAY, 0, 0); }

void ScintillaIFace::homeDisplayExtend() { send(SCI_HOMEDISPLAYEXTEND, 0, 0); }

void ScintillaIFace::lineEndDisplay() { send(SCI_LINEENDDISPLAY, 0, 0); }

void ScintillaIFace::lineEndDisplayExtend() {
  send(SCI_LINEENDDISPLAYEXTEND, 0, 0);
}

void ScintillaIFace::homeWrap() { send(SCI_HOMEWRAP, 0, 0); }

void ScintillaIFace::homeWrapExtend() { send(SCI_HOMEWRAPEXTEND, 0, 0); }

void ScintillaIFace::lineEndWrap() { send(SCI_LINEENDWRAP, 0, 0); }

void ScintillaIFace::lineEndWrapExtend() { send(SCI_LINEENDWRAPEXTEND, 0, 0); }

void ScintillaIFace::vCHomeWrap() { send(SCI_VCHOMEWRAP, 0, 0); }

void ScintillaIFace::vCHomeWrapExtend() { send(SCI_VCHOMEWRAPEXTEND, 0, 0); }

void ScintillaIFace::lineCopy() { send(SCI_LINECOPY, 0, 0); }

void ScintillaIFace::moveCaretInsideView() {
  send(SCI_MOVECARETINSIDEVIEW, 0, 0);
}

int ScintillaIFace::lineLength(int line) {
  return (int)send(SCI_LINELENGTH, (uptr_t)line, 0);
}

void ScintillaIFace::braceHighlight(int pos1, int pos2) {
  send(SCI_BRACEHIGHLIGHT, (uptr_t)pos1, (sptr_t)pos2);
}

void ScintillaIFace::braceHighlightIndicator(bool useBraceHighlightIndicator,
                                             int indicator) {
  send(SCI_BRACEHIGHLIGHTINDICATOR, (uptr_t)useBraceHighlightIndicator,
       (sptr_t)indicator);
}

void ScintillaIFace::braceBadLight(int pos) {
  send(SCI_BRACEBADLIGHT, (uptr_t)pos, 0);
}

void ScintillaIFace::braceBadLightIndicator(bool useBraceBadLightIndicator,
                                            int indicator) {
  send(SCI_BRACEBADLIGHTINDICATOR, (uptr_t)useBraceBadLightIndicator,
       (sptr_t)indicator);
}

int ScintillaIFace::braceMatch(int pos) {
  return (int)send(SCI_BRACEMATCH, (uptr_t)pos, 0);
}

bool ScintillaIFace::viewEOL() const { return send(SCI_GETVIEWEOL, 0, 0) != 0; }

void ScintillaIFace::setViewEOL(bool visible) {
  send(SCI_SETVIEWEOL, (uptr_t)visible, 0);
}

sptr_t ScintillaIFace::docPointer() const {
  return send(SCI_GETDOCPOINTER, 0, 0);
}

void ScintillaIFace::setDocPointer(sptr_t pointer) {
  send(SCI_SETDOCPOINTER, 0, pointer);
}

void ScintillaIFace::setModEventMask(int mask) {
  send(SCI_SETMODEVENTMASK, (uptr_t)mask, 0);
}

int ScintillaIFace::edgeColumn() const {
  return (int)send(SCI_GETEDGECOLUMN, 0, 0);
}

void ScintillaIFace::setEdgeColumn(int column) {
  send(SCI_SETEDGECOLUMN, (uptr_t)column, 0);
}

int ScintillaIFace::edgeMode() const {
  return (int)send(SCI_GETEDGEMODE, 0, 0);
}

void ScintillaIFace::setEdgeMode(int mode) {
  send(SCI_SETEDGEMODE, (uptr_t)mode, 0);
}

QColor ScintillaIFace::edgeColour() const {
  return decodeColor(send(SCI_GETEDGECOLOUR, 0, 0));
}

void ScintillaIFace::setEdgeColour(QColor edgeColour) {
  send(SCI_SETEDGECOLOUR, encodeColor(edgeColour), 0);
}

void ScintillaIFace::searchAnchor() { send(SCI_SEARCHANCHOR, 0, 0); }

int ScintillaIFace::searchNext(int flags, const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  return (int)send(SCI_SEARCHNEXT, (uptr_t)flags, (sptr_t)textUtf8.constData());
}

int ScintillaIFace::searchPrev(int flags, const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  return (int)send(SCI_SEARCHPREV, (uptr_t)flags, (sptr_t)textUtf8.constData());
}

int ScintillaIFace::linesOnScreen() const {
  return (int)send(SCI_LINESONSCREEN, 0, 0);
}

void ScintillaIFace::usePopUp(bool allowPopUp) {
  send(SCI_USEPOPUP, (uptr_t)allowPopUp, 0);
}

bool ScintillaIFace::selectionIsRectangle() const {
  return send(SCI_SELECTIONISRECTANGLE, 0, 0) != 0;
}

void ScintillaIFace::setZoom(int zoom) { send(SCI_SETZOOM, (uptr_t)zoom, 0); }

int ScintillaIFace::zoom() const { return (int)send(SCI_GETZOOM, 0, 0); }

int ScintillaIFace::createDocument() {
  return (int)send(SCI_CREATEDOCUMENT, 0, 0);
}

void ScintillaIFace::addRefDocument(int doc) {
  send(SCI_ADDREFDOCUMENT, 0, (sptr_t)doc);
}

void ScintillaIFace::releaseDocument(int doc) {
  send(SCI_RELEASEDOCUMENT, 0, (sptr_t)doc);
}

int ScintillaIFace::modEventMask() const {
  return (int)send(SCI_GETMODEVENTMASK, 0, 0);
}

void ScintillaIFace::setFocus(bool focus) {
  send(SCI_SETFOCUS, (uptr_t)focus, 0);
}

bool ScintillaIFace::focus() const { return send(SCI_GETFOCUS, 0, 0) != 0; }

void ScintillaIFace::setStatus(int statusCode) {
  send(SCI_SETSTATUS, (uptr_t)statusCode, 0);
}

int ScintillaIFace::status() const { return (int)send(SCI_GETSTATUS, 0, 0); }

void ScintillaIFace::setMouseDownCaptures(bool captures) {
  send(SCI_SETMOUSEDOWNCAPTURES, (uptr_t)captures, 0);
}

bool ScintillaIFace::mouseDownCaptures() const {
  return send(SCI_GETMOUSEDOWNCAPTURES, 0, 0) != 0;
}

void ScintillaIFace::setCursor(int cursorType) {
  send(SCI_SETCURSOR, (uptr_t)cursorType, 0);
}

int ScintillaIFace::cursor() const { return (int)send(SCI_GETCURSOR, 0, 0); }

void ScintillaIFace::setControlCharSymbol(int symbol) {
  send(SCI_SETCONTROLCHARSYMBOL, (uptr_t)symbol, 0);
}

int ScintillaIFace::controlCharSymbol() const {
  return (int)send(SCI_GETCONTROLCHARSYMBOL, 0, 0);
}

void ScintillaIFace::wordPartLeft() { send(SCI_WORDPARTLEFT, 0, 0); }

void ScintillaIFace::wordPartLeftExtend() {
  send(SCI_WORDPARTLEFTEXTEND, 0, 0);
}

void ScintillaIFace::wordPartRight() { send(SCI_WORDPARTRIGHT, 0, 0); }

void ScintillaIFace::wordPartRightExtend() {
  send(SCI_WORDPARTRIGHTEXTEND, 0, 0);
}

void ScintillaIFace::setVisiblePolicy(int visiblePolicy, int visibleSlop) {
  send(SCI_SETVISIBLEPOLICY, (uptr_t)visiblePolicy, (sptr_t)visibleSlop);
}

void ScintillaIFace::delLineLeft() { send(SCI_DELLINELEFT, 0, 0); }

void ScintillaIFace::delLineRight() { send(SCI_DELLINERIGHT, 0, 0); }

void ScintillaIFace::setXOffset(int newOffset) {
  send(SCI_SETXOFFSET, (uptr_t)newOffset, 0);
}

int ScintillaIFace::xOffset() const { return (int)send(SCI_GETXOFFSET, 0, 0); }

void ScintillaIFace::chooseCaretX() { send(SCI_CHOOSECARETX, 0, 0); }

void ScintillaIFace::grabFocus() { send(SCI_GRABFOCUS, 0, 0); }

void ScintillaIFace::setXCaretPolicy(int caretPolicy, int caretSlop) {
  send(SCI_SETXCARETPOLICY, (uptr_t)caretPolicy, (sptr_t)caretSlop);
}

void ScintillaIFace::setYCaretPolicy(int caretPolicy, int caretSlop) {
  send(SCI_SETYCARETPOLICY, (uptr_t)caretPolicy, (sptr_t)caretSlop);
}

void ScintillaIFace::setPrintWrapMode(int mode) {
  send(SCI_SETPRINTWRAPMODE, (uptr_t)mode, 0);
}

int ScintillaIFace::printWrapMode() const {
  return (int)send(SCI_GETPRINTWRAPMODE, 0, 0);
}

void ScintillaIFace::setHotspotActiveFore(bool useSetting, QColor fore) {
  send(SCI_SETHOTSPOTACTIVEFORE, (uptr_t)useSetting, encodeColor(fore));
}

QColor ScintillaIFace::hotspotActiveFore() const {
  return decodeColor(send(SCI_GETHOTSPOTACTIVEFORE, 0, 0));
}

void ScintillaIFace::setHotspotActiveBack(bool useSetting, QColor back) {
  send(SCI_SETHOTSPOTACTIVEBACK, (uptr_t)useSetting, encodeColor(back));
}

QColor ScintillaIFace::hotspotActiveBack() const {
  return decodeColor(send(SCI_GETHOTSPOTACTIVEBACK, 0, 0));
}

void ScintillaIFace::setHotspotActiveUnderline(bool underline) {
  send(SCI_SETHOTSPOTACTIVEUNDERLINE, (uptr_t)underline, 0);
}

bool ScintillaIFace::hotspotActiveUnderline() const {
  return send(SCI_GETHOTSPOTACTIVEUNDERLINE, 0, 0) != 0;
}

void ScintillaIFace::setHotspotSingleLine(bool singleLine) {
  send(SCI_SETHOTSPOTSINGLELINE, (uptr_t)singleLine, 0);
}

bool ScintillaIFace::hotspotSingleLine() const {
  return send(SCI_GETHOTSPOTSINGLELINE, 0, 0) != 0;
}

void ScintillaIFace::paraDown() { send(SCI_PARADOWN, 0, 0); }

void ScintillaIFace::paraDownExtend() { send(SCI_PARADOWNEXTEND, 0, 0); }

void ScintillaIFace::paraUp() { send(SCI_PARAUP, 0, 0); }

void ScintillaIFace::paraUpExtend() { send(SCI_PARAUPEXTEND, 0, 0); }

int ScintillaIFace::positionBefore(int pos) {
  return (int)send(SCI_POSITIONBEFORE, (uptr_t)pos, 0);
}

int ScintillaIFace::positionAfter(int pos) {
  return (int)send(SCI_POSITIONAFTER, (uptr_t)pos, 0);
}

int ScintillaIFace::positionRelative(int pos, int relative) {
  return (int)send(SCI_POSITIONRELATIVE, (uptr_t)pos, (sptr_t)relative);
}

void ScintillaIFace::copyRange(int start, int end) {
  send(SCI_COPYRANGE, (uptr_t)start, (sptr_t)end);
}

void ScintillaIFace::copyText(const QString &text) {
  QByteArray textUtf8 = text.toUtf8();
  send(SCI_COPYTEXT, (uptr_t)textUtf8.length(), (sptr_t)textUtf8.constData());
}

void ScintillaIFace::setSelectionMode(int mode) {
  send(SCI_SETSELECTIONMODE, (uptr_t)mode, 0);
}

int ScintillaIFace::selectionMode() const {
  return (int)send(SCI_GETSELECTIONMODE, 0, 0);
}

int ScintillaIFace::lineSelStartPosition(int line) {
  return (int)send(SCI_GETLINESELSTARTPOSITION, (uptr_t)line, 0);
}

int ScintillaIFace::lineSelEndPosition(int line) {
  return (int)send(SCI_GETLINESELENDPOSITION, (uptr_t)line, 0);
}

void ScintillaIFace::lineDownRectExtend() {
  send(SCI_LINEDOWNRECTEXTEND, 0, 0);
}

void ScintillaIFace::lineUpRectExtend() { send(SCI_LINEUPRECTEXTEND, 0, 0); }

void ScintillaIFace::charLeftRectExtend() {
  send(SCI_CHARLEFTRECTEXTEND, 0, 0);
}

void ScintillaIFace::charRightRectExtend() {
  send(SCI_CHARRIGHTRECTEXTEND, 0, 0);
}

void ScintillaIFace::homeRectExtend() { send(SCI_HOMERECTEXTEND, 0, 0); }

void ScintillaIFace::vCHomeRectExtend() { send(SCI_VCHOMERECTEXTEND, 0, 0); }

void ScintillaIFace::lineEndRectExtend() { send(SCI_LINEENDRECTEXTEND, 0, 0); }

void ScintillaIFace::pageUpRectExtend() { send(SCI_PAGEUPRECTEXTEND, 0, 0); }

void ScintillaIFace::pageDownRectExtend() {
  send(SCI_PAGEDOWNRECTEXTEND, 0, 0);
}

void ScintillaIFace::stutteredPageUp() { send(SCI_STUTTEREDPAGEUP, 0, 0); }

void ScintillaIFace::stutteredPageUpExtend() {
  send(SCI_STUTTEREDPAGEUPEXTEND, 0, 0);
}

void ScintillaIFace::stutteredPageDown() { send(SCI_STUTTEREDPAGEDOWN, 0, 0); }

void ScintillaIFace::stutteredPageDownExtend() {
  send(SCI_STUTTEREDPAGEDOWNEXTEND, 0, 0);
}

void ScintillaIFace::wordLeftEnd() { send(SCI_WORDLEFTEND, 0, 0); }

void ScintillaIFace::wordLeftEndExtend() { send(SCI_WORDLEFTENDEXTEND, 0, 0); }

void ScintillaIFace::wordRightEnd() { send(SCI_WORDRIGHTEND, 0, 0); }

void ScintillaIFace::wordRightEndExtend() {
  send(SCI_WORDRIGHTENDEXTEND, 0, 0);
}

void ScintillaIFace::setWhitespaceChars(const QByteArray &chars) {
  send(SCI_SETWHITESPACECHARS, 0, (sptr_t)chars.constData());
}

QByteArray ScintillaIFace::whitespaceChars() const {
  QByteArray chars(send(SCI_GETWHITESPACECHARS), '\0');
  send(SCI_GETWHITESPACECHARS, 0, (sptr_t)chars.data());
  return chars;
}

void ScintillaIFace::setPunctuationChars(const QByteArray &chars) {
  send(SCI_SETPUNCTUATIONCHARS, 0, (sptr_t)chars.constData());
}

QByteArray ScintillaIFace::punctuationChars() const {
  QByteArray chars(send(SCI_GETPUNCTUATIONCHARS), '\0');
  send(SCI_GETPUNCTUATIONCHARS, 0, (sptr_t)chars.data());
  return chars;
}

void ScintillaIFace::setCharsDefault() { send(SCI_SETCHARSDEFAULT, 0, 0); }

int ScintillaIFace::autoCCurrent() const {
  return (int)send(SCI_AUTOCGETCURRENT, 0, 0);
}

int ScintillaIFace::autoCCurrentText(char *s) const {
  return (int)send(SCI_AUTOCGETCURRENTTEXT, 0, (sptr_t)s);
}

void ScintillaIFace::autoCSetCaseInsensitiveBehaviour(int behaviour) {
  send(SCI_AUTOCSETCASEINSENSITIVEBEHAVIOUR, (uptr_t)behaviour, 0);
}

int ScintillaIFace::autoCCaseInsensitiveBehaviour() const {
  return (int)send(SCI_AUTOCGETCASEINSENSITIVEBEHAVIOUR, 0, 0);
}

void ScintillaIFace::autoCSetMulti(int multi) {
  send(SCI_AUTOCSETMULTI, (uptr_t)multi, 0);
}

int ScintillaIFace::autoCMulti() const {
  return (int)send(SCI_AUTOCGETMULTI, 0, 0);
}

void ScintillaIFace::autoCSetOrder(int order) {
  send(SCI_AUTOCSETORDER, (uptr_t)order, 0);
}

int ScintillaIFace::autoCOrder() const {
  return (int)send(SCI_AUTOCGETORDER, 0, 0);
}

void ScintillaIFace::allocate(int bytes) {
  send(SCI_ALLOCATE, (uptr_t)bytes, 0);
}

int ScintillaIFace::targetAsUTF8(char *s) {
  return (int)send(SCI_TARGETASUTF8, 0, (sptr_t)s);
}

void ScintillaIFace::setLengthForEncode(int bytes) {
  send(SCI_SETLENGTHFORENCODE, (uptr_t)bytes, 0);
}

int ScintillaIFace::encodedFromUTF8(const char *utf8, char *encoded) {
  return (int)send(SCI_ENCODEDFROMUTF8, (uptr_t)utf8, (sptr_t)encoded);
}

int ScintillaIFace::findColumn(int line, int column) {
  return send(SCI_FINDCOLUMN, line, column);
}

int ScintillaIFace::caretSticky() const {
  return (int)send(SCI_GETCARETSTICKY, 0, 0);
}

void ScintillaIFace::setCaretSticky(int useCaretStickyBehaviour) {
  send(SCI_SETCARETSTICKY, (uptr_t)useCaretStickyBehaviour, 0);
}

void ScintillaIFace::toggleCaretSticky() { send(SCI_TOGGLECARETSTICKY, 0, 0); }

void ScintillaIFace::setPasteConvertEndings(bool convert) {
  send(SCI_SETPASTECONVERTENDINGS, (uptr_t)convert, 0);
}

bool ScintillaIFace::pasteConvertEndings() const {
  return send(SCI_GETPASTECONVERTENDINGS, 0, 0) != 0;
}

void ScintillaIFace::selectionDuplicate() {
  send(SCI_SELECTIONDUPLICATE, 0, 0);
}

void ScintillaIFace::setCaretLineBackAlpha(int alpha) {
  send(SCI_SETCARETLINEBACKALPHA, (uptr_t)alpha, 0);
}

int ScintillaIFace::caretLineBackAlpha() const {
  return (int)send(SCI_GETCARETLINEBACKALPHA, 0, 0);
}

void ScintillaIFace::setCaretStyle(int caretStyle) {
  send(SCI_SETCARETSTYLE, (uptr_t)caretStyle, 0);
}

int ScintillaIFace::caretStyle() const {
  return (int)send(SCI_GETCARETSTYLE, 0, 0);
}

void ScintillaIFace::setIndicatorCurrent(int indicator) {
  send(SCI_SETINDICATORCURRENT, (uptr_t)indicator, 0);
}

int ScintillaIFace::indicatorCurrent() const {
  return (int)send(SCI_GETINDICATORCURRENT, 0, 0);
}

void ScintillaIFace::setIndicatorValue(int value) {
  send(SCI_SETINDICATORVALUE, (uptr_t)value, 0);
}

int ScintillaIFace::indicatorValue() const {
  return (int)send(SCI_GETINDICATORVALUE, 0, 0);
}

void ScintillaIFace::indicatorFillRange(int position, int fillLength) {
  send(SCI_INDICATORFILLRANGE, (uptr_t)position, (sptr_t)fillLength);
}

void ScintillaIFace::indicatorClearRange(int position, int clearLength) {
  send(SCI_INDICATORCLEARRANGE, (uptr_t)position, (sptr_t)clearLength);
}

int ScintillaIFace::indicatorAllOnFor(int position) {
  return (int)send(SCI_INDICATORALLONFOR, (uptr_t)position, 0);
}

int ScintillaIFace::indicatorValueAt(int indicator, int position) {
  return (int)send(SCI_INDICATORVALUEAT, (uptr_t)indicator, (sptr_t)position);
}

int ScintillaIFace::indicatorStart(int indicator, int position) {
  return (int)send(SCI_INDICATORSTART, (uptr_t)indicator, (sptr_t)position);
}

int ScintillaIFace::indicatorEnd(int indicator, int position) {
  return (int)send(SCI_INDICATOREND, (uptr_t)indicator, (sptr_t)position);
}

void ScintillaIFace::setPositionCache(int size) {
  send(SCI_SETPOSITIONCACHE, (uptr_t)size, 0);
}

int ScintillaIFace::positionCache() const {
  return (int)send(SCI_GETPOSITIONCACHE, 0, 0);
}

void ScintillaIFace::copyAllowLine() { send(SCI_COPYALLOWLINE, 0, 0); }

const char *ScintillaIFace::characterPointer() const {
  return reinterpret_cast<const char *>(send(SCI_GETCHARACTERPOINTER, 0, 0));
}

int ScintillaIFace::rangePointer(int position, int rangeLength) const {
  return (int)send(SCI_GETRANGEPOINTER, (uptr_t)position, (sptr_t)rangeLength);
}

int ScintillaIFace::gapPosition() const {
  return (int)send(SCI_GETGAPPOSITION, 0, 0);
}

void ScintillaIFace::indicSetAlpha(int indicator, int alpha) {
  send(SCI_INDICSETALPHA, (uptr_t)indicator, (sptr_t)alpha);
}

int ScintillaIFace::indicAlpha(int indicator) const {
  return (int)send(SCI_INDICGETALPHA, (uptr_t)indicator, 0);
}

void ScintillaIFace::indicSetOutlineAlpha(int indicator, int alpha) {
  send(SCI_INDICSETOUTLINEALPHA, (uptr_t)indicator, (sptr_t)alpha);
}

int ScintillaIFace::indicOutlineAlpha(int indicator) const {
  return (int)send(SCI_INDICGETOUTLINEALPHA, (uptr_t)indicator, 0);
}

void ScintillaIFace::setExtraAscent(int extraAscent) {
  send(SCI_SETEXTRAASCENT, (uptr_t)extraAscent, 0);
}

int ScintillaIFace::extraAscent() const {
  return (int)send(SCI_GETEXTRAASCENT, 0, 0);
}

void ScintillaIFace::setExtraDescent(int extraDescent) {
  send(SCI_SETEXTRADESCENT, (uptr_t)extraDescent, 0);
}

int ScintillaIFace::extraDescent() const {
  return (int)send(SCI_GETEXTRADESCENT, 0, 0);
}

int ScintillaIFace::markerSymbolDefined(int markerNumber) {
  return (int)send(SCI_MARKERSYMBOLDEFINED, (uptr_t)markerNumber, 0);
}

void ScintillaIFace::marginSetText(int line, const QString &text) {
  send(SCI_MARGINSETTEXT, line, (sptr_t)text.toUtf8().constData());
}

QString ScintillaIFace::marginText(int line) const {
  int len = send(SCI_MARGINGETTEXT, line, 0);
  QVarLengthArray<char, 1024> buffer(len);
  send(SCI_MARGINGETTEXT, line, (sptr_t)buffer.data());
  return QString::fromUtf8(buffer.constData(), len);
}

void ScintillaIFace::marginSetStyle(int line, int style) {
  send(SCI_MARGINSETSTYLE, (uptr_t)line, (sptr_t)style);
}

int ScintillaIFace::marginStyle(int line) const {
  return (int)send(SCI_MARGINGETSTYLE, (uptr_t)line, 0);
}

void ScintillaIFace::marginSetStyles(int line, const QString &styles) {
  QByteArray stylesUtf8 = styles.toUtf8();
  send(SCI_MARGINSETSTYLES, (uptr_t)line, (sptr_t)stylesUtf8.constData());
}

int ScintillaIFace::marginStyles(int line, char *styles) const {
  return (int)send(SCI_MARGINGETSTYLES, (uptr_t)line, (sptr_t)styles);
}

void ScintillaIFace::marginTextClearAll() {
  send(SCI_MARGINTEXTCLEARALL, 0, 0);
}

void ScintillaIFace::marginSetStyleOffset(int style) {
  send(SCI_MARGINSETSTYLEOFFSET, (uptr_t)style, 0);
}

int ScintillaIFace::marginStyleOffset() const {
  return (int)send(SCI_MARGINGETSTYLEOFFSET, 0, 0);
}

void ScintillaIFace::setMarginOptions(int marginOptions) {
  send(SCI_SETMARGINOPTIONS, (uptr_t)marginOptions, 0);
}

int ScintillaIFace::marginOptions() const {
  return (int)send(SCI_GETMARGINOPTIONS, 0, 0);
}

void ScintillaIFace::annotationSetText(int line, const QString &text) {
  send(SCI_ANNOTATIONSETTEXT, line, (sptr_t)text.toUtf8().constData());
}

QString ScintillaIFace::annotationText(int line) const {
  int len = send(SCI_ANNOTATIONGETTEXT, line);
  QVarLengthArray<char, 128> buffer(len);
  send(SCI_ANNOTATIONGETTEXT, line, (sptr_t)buffer.data());
  return QString::fromUtf8(buffer.constData(), len);
}

void ScintillaIFace::annotationSetStyle(int line, int style) {
  send(SCI_ANNOTATIONSETSTYLE, line, (sptr_t)style);
}

int ScintillaIFace::annotationStyle(int line) const {
  return send(SCI_ANNOTATIONGETSTYLE, line);
}

void ScintillaIFace::annotationSetStyles(int line, const QByteArray &styles) {
  send(SCI_ANNOTATIONSETSTYLES, line, (sptr_t)styles.constData());
}

int ScintillaIFace::annotationStyles(int line, char *styles) const {
  return send(SCI_ANNOTATIONGETSTYLES, line, (sptr_t)styles);
}

int ScintillaIFace::annotationLines(int line) const {
  return send(SCI_ANNOTATIONGETLINES, line);
}

void ScintillaIFace::annotationClearAll() { send(SCI_ANNOTATIONCLEARALL); }

void ScintillaIFace::annotationSetVisible(int visible) {
  send(SCI_ANNOTATIONSETVISIBLE, visible);
}

int ScintillaIFace::annotationVisible() const {
  return send(SCI_ANNOTATIONGETVISIBLE);
}

void ScintillaIFace::annotationSetStyleOffset(int style) {
  send(SCI_ANNOTATIONSETSTYLEOFFSET, style);
}

int ScintillaIFace::annotationStyleOffset() const {
  return send(SCI_ANNOTATIONGETSTYLEOFFSET);
}

void ScintillaIFace::releaseAllExtendedStyles() {
  send(SCI_RELEASEALLEXTENDEDSTYLES);
}

int ScintillaIFace::allocateExtendedStyles(int numberStyles) {
  return send(SCI_ALLOCATEEXTENDEDSTYLES, numberStyles);
}

void ScintillaIFace::addUndoAction(int token, int flags) {
  send(SCI_ADDUNDOACTION, (uptr_t)token, (sptr_t)flags);
}

int ScintillaIFace::charPositionFromPoint(int x, int y) {
  return (int)send(SCI_CHARPOSITIONFROMPOINT, (uptr_t)x, (sptr_t)y);
}

int ScintillaIFace::charPositionFromPointClose(int x, int y) {
  return (int)send(SCI_CHARPOSITIONFROMPOINTCLOSE, (uptr_t)x, (sptr_t)y);
}

void ScintillaIFace::setMouseSelectionRectangularSwitch(
    bool mouseSelectionRectangularSwitch) {
  send(SCI_SETMOUSESELECTIONRECTANGULARSWITCH,
       (uptr_t)mouseSelectionRectangularSwitch, 0);
}

bool ScintillaIFace::mouseSelectionRectangularSwitch() const {
  return send(SCI_GETMOUSESELECTIONRECTANGULARSWITCH, 0, 0) != 0;
}

void ScintillaIFace::setMultipleSelection(bool multipleSelection) {
  send(SCI_SETMULTIPLESELECTION, (uptr_t)multipleSelection, 0);
}

bool ScintillaIFace::multipleSelection() const {
  return send(SCI_GETMULTIPLESELECTION, 0, 0) != 0;
}

void ScintillaIFace::setAdditionalSelectionTyping(
    bool additionalSelectionTyping) {
  send(SCI_SETADDITIONALSELECTIONTYPING, (uptr_t)additionalSelectionTyping, 0);
}

bool ScintillaIFace::additionalSelectionTyping() const {
  return send(SCI_GETADDITIONALSELECTIONTYPING, 0, 0) != 0;
}

void ScintillaIFace::setAdditionalCaretsBlink(bool additionalCaretsBlink) {
  send(SCI_SETADDITIONALCARETSBLINK, (uptr_t)additionalCaretsBlink, 0);
}

bool ScintillaIFace::additionalCaretsBlink() const {
  return send(SCI_GETADDITIONALCARETSBLINK, 0, 0) != 0;
}

void ScintillaIFace::setAdditionalCaretsVisible(bool additionalCaretsBlink) {
  send(SCI_SETADDITIONALCARETSVISIBLE, (uptr_t)additionalCaretsBlink, 0);
}

bool ScintillaIFace::additionalCaretsVisible() const {
  return send(SCI_GETADDITIONALCARETSVISIBLE, 0, 0) != 0;
}

int ScintillaIFace::selections() const {
  return (int)send(SCI_GETSELECTIONS, 0, 0);
}

bool ScintillaIFace::selectionEmpty() const {
  return send(SCI_GETSELECTIONEMPTY, 0, 0) != 0;
}

void ScintillaIFace::clearSelections() { send(SCI_CLEARSELECTIONS, 0, 0); }

int ScintillaIFace::setSelection(int caret, int anchor) {
  return (int)send(SCI_SETSELECTION, (uptr_t)caret, (sptr_t)anchor);
}

int ScintillaIFace::addSelection(int caret, int anchor) {
  return (int)send(SCI_ADDSELECTION, (uptr_t)caret, (sptr_t)anchor);
}

void ScintillaIFace::dropSelectionN(int selection) {
  send(SCI_DROPSELECTIONN, (uptr_t)selection, 0);
}

void ScintillaIFace::setMainSelection(int selection) {
  send(SCI_SETMAINSELECTION, (uptr_t)selection, 0);
}

int ScintillaIFace::mainSelection() const {
  return (int)send(SCI_GETMAINSELECTION, 0, 0);
}

void ScintillaIFace::setSelectionNCaret(int selection, int pos) {
  send(SCI_SETSELECTIONNCARET, (uptr_t)selection, (sptr_t)pos);
}

int ScintillaIFace::selectionNCaret(int selection) const {
  return (int)send(SCI_GETSELECTIONNCARET, (uptr_t)selection, 0);
}

void ScintillaIFace::setSelectionNAnchor(int selection, int posAnchor) {
  send(SCI_SETSELECTIONNANCHOR, (uptr_t)selection, (sptr_t)posAnchor);
}

int ScintillaIFace::selectionNAnchor(int selection) const {
  return (int)send(SCI_GETSELECTIONNANCHOR, (uptr_t)selection, 0);
}

void ScintillaIFace::setSelectionNCaretVirtualSpace(int selection, int space) {
  send(SCI_SETSELECTIONNCARETVIRTUALSPACE, (uptr_t)selection, (sptr_t)space);
}

int ScintillaIFace::selectionNCaretVirtualSpace(int selection) const {
  return (int)send(SCI_GETSELECTIONNCARETVIRTUALSPACE, (uptr_t)selection, 0);
}

void ScintillaIFace::setSelectionNAnchorVirtualSpace(int selection, int space) {
  send(SCI_SETSELECTIONNANCHORVIRTUALSPACE, (uptr_t)selection, (sptr_t)space);
}

int ScintillaIFace::selectionNAnchorVirtualSpace(int selection) const {
  return (int)send(SCI_GETSELECTIONNANCHORVIRTUALSPACE, (uptr_t)selection, 0);
}

void ScintillaIFace::setSelectionNStart(int selection, int pos) {
  send(SCI_SETSELECTIONNSTART, (uptr_t)selection, (sptr_t)pos);
}

int ScintillaIFace::selectionNStart(int selection) const {
  return (int)send(SCI_GETSELECTIONNSTART, (uptr_t)selection, 0);
}

void ScintillaIFace::setSelectionNEnd(int selection, int pos) {
  send(SCI_SETSELECTIONNEND, (uptr_t)selection, (sptr_t)pos);
}

int ScintillaIFace::selectionNEnd(int selection) const {
  return (int)send(SCI_GETSELECTIONNEND, (uptr_t)selection, 0);
}

void ScintillaIFace::setRectangularSelectionCaret(int pos) {
  send(SCI_SETRECTANGULARSELECTIONCARET, (uptr_t)pos, 0);
}

int ScintillaIFace::rectangularSelectionCaret() const {
  return (int)send(SCI_GETRECTANGULARSELECTIONCARET, 0, 0);
}

void ScintillaIFace::setRectangularSelectionAnchor(int posAnchor) {
  send(SCI_SETRECTANGULARSELECTIONANCHOR, (uptr_t)posAnchor, 0);
}

int ScintillaIFace::rectangularSelectionAnchor() const {
  return (int)send(SCI_GETRECTANGULARSELECTIONANCHOR, 0, 0);
}

void ScintillaIFace::setRectangularSelectionCaretVirtualSpace(int space) {
  send(SCI_SETRECTANGULARSELECTIONCARETVIRTUALSPACE, (uptr_t)space, 0);
}

int ScintillaIFace::rectangularSelectionCaretVirtualSpace() const {
  return (int)send(SCI_GETRECTANGULARSELECTIONCARETVIRTUALSPACE, 0, 0);
}

void ScintillaIFace::setRectangularSelectionAnchorVirtualSpace(int space) {
  send(SCI_SETRECTANGULARSELECTIONANCHORVIRTUALSPACE, (uptr_t)space, 0);
}

int ScintillaIFace::rectangularSelectionAnchorVirtualSpace() const {
  return (int)send(SCI_GETRECTANGULARSELECTIONANCHORVIRTUALSPACE, 0, 0);
}

void ScintillaIFace::setVirtualSpaceOptions(int virtualSpaceOptions) {
  send(SCI_SETVIRTUALSPACEOPTIONS, (uptr_t)virtualSpaceOptions, 0);
}

int ScintillaIFace::virtualSpaceOptions() const {
  return (int)send(SCI_GETVIRTUALSPACEOPTIONS, 0, 0);
}

void ScintillaIFace::setRectangularSelectionModifier(int modifier) {
  send(SCI_SETRECTANGULARSELECTIONMODIFIER, (uptr_t)modifier, 0);
}

int ScintillaIFace::rectangularSelectionModifier() const {
  return (int)send(SCI_GETRECTANGULARSELECTIONMODIFIER, 0, 0);
}

void ScintillaIFace::setAdditionalSelFore(QColor fore) {
  send(SCI_SETADDITIONALSELFORE, encodeColor(fore), 0);
}

void ScintillaIFace::setAdditionalSelBack(QColor back) {
  send(SCI_SETADDITIONALSELBACK, encodeColor(back), 0);
}

void ScintillaIFace::setAdditionalSelAlpha(int alpha) {
  send(SCI_SETADDITIONALSELALPHA, (uptr_t)alpha, 0);
}

int ScintillaIFace::additionalSelAlpha() const {
  return (int)send(SCI_GETADDITIONALSELALPHA, 0, 0);
}

void ScintillaIFace::setAdditionalCaretFore(QColor fore) {
  send(SCI_SETADDITIONALCARETFORE, encodeColor(fore), 0);
}

QColor ScintillaIFace::additionalCaretFore() const {
  return decodeColor(send(SCI_GETADDITIONALCARETFORE, 0, 0));
}

void ScintillaIFace::rotateSelection() { send(SCI_ROTATESELECTION, 0, 0); }

void ScintillaIFace::swapMainAnchorCaret() {
  send(SCI_SWAPMAINANCHORCARET, 0, 0);
}

void ScintillaIFace::multipleSelectAddNext() {
  send(SCI_MULTIPLESELECTADDNEXT, 0, 0);
}

void ScintillaIFace::multipleSelectAddEach() {
  send(SCI_MULTIPLESELECTADDEACH, 0, 0);
}

int ScintillaIFace::changeLexerState(int start, int end) {
  return (int)send(SCI_CHANGELEXERSTATE, (uptr_t)start, (sptr_t)end);
}

int ScintillaIFace::contractedFoldNext(int lineStart) {
  return (int)send(SCI_CONTRACTEDFOLDNEXT, (uptr_t)lineStart, 0);
}

void ScintillaIFace::verticalCentreCaret() {
  send(SCI_VERTICALCENTRECARET, 0, 0);
}

void ScintillaIFace::moveSelectedLinesUp() {
  send(SCI_MOVESELECTEDLINESUP, 0, 0);
}

void ScintillaIFace::moveSelectedLinesDown() {
  send(SCI_MOVESELECTEDLINESDOWN, 0, 0);
}

void ScintillaIFace::setIdentifier(int identifier) {
  send(SCI_SETIDENTIFIER, (uptr_t)identifier, 0);
}

int ScintillaIFace::identifier() const {
  return (int)send(SCI_GETIDENTIFIER, 0, 0);
}

void ScintillaIFace::markerDefineImage(int markerNumber, const QImage &image) {
  QImage argb = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  send(SCI_RGBAIMAGESETWIDTH, argb.width());
  send(SCI_RGBAIMAGESETHEIGHT, argb.height());
  send(SCI_RGBAIMAGESETSCALE, argb.devicePixelRatio() * 100);
  send(SCI_MARKERDEFINERGBAIMAGE, markerNumber, (sptr_t)argb.bits());
}

void ScintillaIFace::registerRGBAImage(int type, const QString &pixels) {
  QByteArray pixelsUtf8 = pixels.toUtf8();
  send(SCI_REGISTERRGBAIMAGE, (uptr_t)type, (sptr_t)pixelsUtf8.constData());
}

void ScintillaIFace::scrollToStart() { send(SCI_SCROLLTOSTART, 0, 0); }

void ScintillaIFace::scrollToEnd() { send(SCI_SCROLLTOEND, 0, 0); }

void ScintillaIFace::setTechnology(int technology) {
  send(SCI_SETTECHNOLOGY, (uptr_t)technology, 0);
}

int ScintillaIFace::technology() const {
  return (int)send(SCI_GETTECHNOLOGY, 0, 0);
}

int ScintillaIFace::createLoader(int bytes) {
  return (int)send(SCI_CREATELOADER, (uptr_t)bytes, 0);
}

void ScintillaIFace::findIndicatorShow(int start, int end) {
  send(SCI_FINDINDICATORSHOW, (uptr_t)start, (sptr_t)end);
}

void ScintillaIFace::findIndicatorFlash(int start, int end) {
  send(SCI_FINDINDICATORFLASH, (uptr_t)start, (sptr_t)end);
}

void ScintillaIFace::findIndicatorHide() { send(SCI_FINDINDICATORHIDE, 0, 0); }

void ScintillaIFace::vCHomeDisplay() { send(SCI_VCHOMEDISPLAY, 0, 0); }

void ScintillaIFace::vCHomeDisplayExtend() {
  send(SCI_VCHOMEDISPLAYEXTEND, 0, 0);
}

bool ScintillaIFace::caretLineVisibleAlways() const {
  return send(SCI_GETCARETLINEVISIBLEALWAYS, 0, 0) != 0;
}

void ScintillaIFace::setCaretLineVisibleAlways(bool alwaysVisible) {
  send(SCI_SETCARETLINEVISIBLEALWAYS, (uptr_t)alwaysVisible, 0);
}

void ScintillaIFace::setLineEndTypesAllowed(int lineEndBitSet) {
  send(SCI_SETLINEENDTYPESALLOWED, (uptr_t)lineEndBitSet, 0);
}

int ScintillaIFace::lineEndTypesAllowed() const {
  return (int)send(SCI_GETLINEENDTYPESALLOWED, 0, 0);
}

int ScintillaIFace::lineEndTypesActive() const {
  return (int)send(SCI_GETLINEENDTYPESACTIVE, 0, 0);
}

void ScintillaIFace::setRepresentation(const QString &encodedCharacter,
                                       const QString &representation) {
  QByteArray encodedCharacterUtf8 = encodedCharacter.toUtf8();
  QByteArray representationUtf8 = representation.toUtf8();
  send(SCI_SETREPRESENTATION, (uptr_t)encodedCharacterUtf8.constData(),
       (sptr_t)representationUtf8.constData());
}

int ScintillaIFace::representation(const QString &encodedCharacter,
                                   char *representation) const {
  QByteArray encodedCharacterUtf8 = encodedCharacter.toUtf8();
  return (int)send(SCI_GETREPRESENTATION,
                   (uptr_t)encodedCharacterUtf8.constData(),
                   (sptr_t)representation);
}

void ScintillaIFace::clearRepresentation(const QString &encodedCharacter) {
  QByteArray encodedCharacterUtf8 = encodedCharacter.toUtf8();
  send(SCI_CLEARREPRESENTATION, (uptr_t)encodedCharacterUtf8.constData(), 0);
}

void ScintillaIFace::startRecord() { send(SCI_STARTRECORD, 0, 0); }

void ScintillaIFace::stopRecord() { send(SCI_STOPRECORD, 0, 0); }

void ScintillaIFace::setLexer(int lexer) {
  send(SCI_SETLEXER, (uptr_t)lexer, 0);
}

int ScintillaIFace::lexer() const { return (int)send(SCI_GETLEXER, 0, 0); }

void ScintillaIFace::colorize(int start, int end) {
  send(SCI_COLOURISE, (uptr_t)start, (sptr_t)end);
}

void ScintillaIFace::setProperty(const QString &key, const QString &value) {
  QByteArray keyUtf8 = key.toUtf8();
  QByteArray valueUtf8 = value.toUtf8();
  send(SCI_SETPROPERTY, (uptr_t)keyUtf8.constData(),
       (sptr_t)valueUtf8.constData());
}

void ScintillaIFace::setKeyWords(int keywordSet, const QString &keyWords) {
  QByteArray keyWordsUtf8 = keyWords.toUtf8();
  send(SCI_SETKEYWORDS, (uptr_t)keywordSet, (sptr_t)keyWordsUtf8.constData());
}

void ScintillaIFace::setLexerLanguage(const QString &language) {
  QByteArray languageUtf8 = language.toUtf8();
  send(SCI_SETLEXERLANGUAGE, 0, (sptr_t)languageUtf8.constData());
}

void ScintillaIFace::loadLexerLibrary(const QString &path) {
  QByteArray pathUtf8 = path.toUtf8();
  send(SCI_LOADLEXERLIBRARY, 0, (sptr_t)pathUtf8.constData());
}

int ScintillaIFace::property(const QString &key, char *buf) const {
  QByteArray keyUtf8 = key.toUtf8();
  return (int)send(SCI_GETPROPERTY, (uptr_t)keyUtf8.constData(), (sptr_t)buf);
}

int ScintillaIFace::propertyExpanded(const QString &key, char *buf) const {
  QByteArray keyUtf8 = key.toUtf8();
  return (int)send(SCI_GETPROPERTYEXPANDED, (uptr_t)keyUtf8.constData(),
                   (sptr_t)buf);
}

int ScintillaIFace::propertyInt(const QString &key) const {
  QByteArray keyUtf8 = key.toUtf8();
  return (int)send(SCI_GETPROPERTYINT, (uptr_t)keyUtf8.constData(), 0);
}

int ScintillaIFace::lexerLanguage(char *text) const {
  return (int)send(SCI_GETLEXERLANGUAGE, 0, (sptr_t)text);
}

uintptr_t ScintillaIFace::privateLexerCall(int operation, uintptr_t pointer) {
  return (uintptr_t)send(SCI_PRIVATELEXERCALL, (uptr_t)operation, pointer);
}

int ScintillaIFace::propertyNames(char *names) {
  return (int)send(SCI_PROPERTYNAMES, 0, (sptr_t)names);
}

int ScintillaIFace::propertyType(const QString &name) {
  QByteArray nameUtf8 = name.toUtf8();
  return (int)send(SCI_PROPERTYTYPE, (uptr_t)nameUtf8.constData(), 0);
}

int ScintillaIFace::describeProperty(const QString &name, char *description) {
  QByteArray nameUtf8 = name.toUtf8();
  return (int)send(SCI_DESCRIBEPROPERTY, (uptr_t)nameUtf8.constData(),
                   (sptr_t)description);
}

int ScintillaIFace::describeKeyWordSets(char *descriptions) {
  return (int)send(SCI_DESCRIBEKEYWORDSETS, 0, (sptr_t)descriptions);
}

int ScintillaIFace::lineEndTypesSupported() const {
  return (int)send(SCI_GETLINEENDTYPESSUPPORTED, 0, 0);
}

int ScintillaIFace::allocateSubStyles(int styleBase, int numberStyles) {
  return (int)send(SCI_ALLOCATESUBSTYLES, (uptr_t)styleBase,
                   (sptr_t)numberStyles);
}

int ScintillaIFace::subStylesStart(int styleBase) const {
  return (int)send(SCI_GETSUBSTYLESSTART, (uptr_t)styleBase, 0);
}

int ScintillaIFace::subStylesLength(int styleBase) const {
  return (int)send(SCI_GETSUBSTYLESLENGTH, (uptr_t)styleBase, 0);
}

int ScintillaIFace::styleFromSubStyle(int subStyle) const {
  return (int)send(SCI_GETSTYLEFROMSUBSTYLE, (uptr_t)subStyle, 0);
}

int ScintillaIFace::primaryStyleFromStyle(int style) const {
  return (int)send(SCI_GETPRIMARYSTYLEFROMSTYLE, (uptr_t)style, 0);
}

void ScintillaIFace::freeSubStyles() { send(SCI_FREESUBSTYLES, 0, 0); }

void ScintillaIFace::setIdentifiers(int style, const QString &identifiers) {
  QByteArray identifiersUtf8 = identifiers.toUtf8();
  send(SCI_SETIDENTIFIERS, (uptr_t)style, (sptr_t)identifiersUtf8.constData());
}

int ScintillaIFace::distanceToSecondaryStyles() const {
  return (int)send(SCI_DISTANCETOSECONDARYSTYLES, 0, 0);
}

int ScintillaIFace::subStyleBases(char *styles) const {
  return (int)send(SCI_GETSUBSTYLEBASES, 0, (sptr_t)styles);
}

//--

} // namespace Scintilla
