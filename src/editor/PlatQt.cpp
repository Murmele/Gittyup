//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "PlatQt.h"
#include "Scintilla.h"
#include "FontQuality.h"
#include <QApplication>
#include <QFont>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QVarLengthArray>
#include <QScreen>
#include <QTextLayout>
#include <QTextLine>
#include <QLibrary>
#include <cstdio>

namespace Scintilla {

Font::Font() noexcept : fid(nullptr) {}

Font::~Font() { Release(); }

void Font::Create(const FontParameters &fp) {
  Release();

  QFont::StyleStrategy strategy;
  switch (fp.extraFontFlag) {
    case SC_EFF_QUALITY_NON_ANTIALIASED:
      strategy = QFont::NoAntialias;
      break;

    case SC_EFF_QUALITY_ANTIALIASED:
    case SC_EFF_QUALITY_LCD_OPTIMIZED:
      strategy = QFont::PreferAntialias;
      break;

    case SC_EFF_QUALITY_DEFAULT:
    default:
      strategy = QFont::PreferDefault;
      break;
  }

  QFont *font = new QFont;
  font->setStyleStrategy(strategy);
  font->setFamily(QString::fromUtf8(fp.faceName));
  font->setPointSize(fp.size);
  font->setBold(fp.weight > 500);
  font->setItalic(fp.italic);

  fid = font;
}

void Font::Release() {
  delete static_cast<QFont *>(fid);
  fid = nullptr;
}

SurfaceImpl::SurfaceImpl() {}

SurfaceImpl::~SurfaceImpl() { Release(); }

void SurfaceImpl::Init(WindowID wid) {
  Release();
  device = static_cast<QWidget *>(wid);
}

void SurfaceImpl::Init(SurfaceID sid, WindowID wid) {
  Release();
  device = static_cast<QPaintDevice *>(sid);
}

void SurfaceImpl::InitPixMap(int width, int height, Surface *surface,
                             WindowID wid) {
  Release();
  deviceOwned = true;
  device = new QPixmap(qMax(1, width), qMax(1, height));
}

void SurfaceImpl::Release() {
  delete painter;
  painter = nullptr;

  if (deviceOwned)
    delete device;

  device = nullptr;
  deviceOwned = false;
}

bool SurfaceImpl::Initialised() { return device; }

void SurfaceImpl::PenColour(ColourDesired fore) {
  QPen pen(QColorFromCA(fore));
  pen.setCapStyle(Qt::FlatCap);
  GetPainter()->setPen(pen);
}

int SurfaceImpl::LogPixelsY() { return device->logicalDpiY(); }

int SurfaceImpl::DeviceHeightFont(int points) { return points; }

void SurfaceImpl::MoveTo(int x_, int y_) {
  x = x_;
  y = y_;
}

void SurfaceImpl::LineTo(int x_, int y_) {
  QLineF line(x, y, x_, y_);
  GetPainter()->drawLine(line);
  x = x_;
  y = y_;
}

void SurfaceImpl::Polygon(Point *pts, size_t npts, ColourDesired fore,
                          ColourDesired back) {
  PenColour(fore);

  QVarLengthArray<QPoint, 8> qpts(npts);
  for (size_t i = 0; i < npts; i++)
    qpts[i] = QPoint(pts[i].x, pts[i].y);

  QPainter *painter = GetPainter();
  painter->setBrush(QColorFromCA(back));
  painter->drawPolygon(qpts.data(), npts);
}

void SurfaceImpl::RectangleDraw(PRectangle rc, ColourDesired fore,
                                ColourDesired back) {
  PenColour(fore);

  QPainter *painter = GetPainter();
  painter->setBrush(QColorFromCA(back));
  painter->drawRect(QRectF(rc.left, rc.top, rc.Width() - 1, rc.Height() - 1));
}

void SurfaceImpl::FillRectangle(PRectangle rc, ColourDesired back) {
  GetPainter()->fillRect(QRectFFromPRect(rc), QColorFromCA(back));
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern) {
  // Tile pattern over rectangle
  SurfaceImpl *surface = static_cast<SurfaceImpl *>(&surfacePattern);
  // Currently assumes 8x8 pattern
  int widthPat = 8;
  int heightPat = 8;
  for (int xTile = rc.left; xTile < rc.right; xTile += widthPat) {
    int widthx = (xTile + widthPat > rc.right) ? rc.right - xTile : widthPat;
    for (int yTile = rc.top; yTile < rc.bottom; yTile += heightPat) {
      int heighty =
          (yTile + heightPat > rc.bottom) ? rc.bottom - yTile : heightPat;
      QRect source(0, 0, widthx, heighty);
      QRect target(xTile, yTile, widthx, heighty);
      QPixmap *pixmap = static_cast<QPixmap *>(surface->device);
      GetPainter()->drawPixmap(target, *pixmap, source);
    }
  }
}

void SurfaceImpl::RoundedRectangle(PRectangle rc, ColourDesired fore,
                                   ColourDesired back) {
  PenColour(fore);

  QPainter *painter = GetPainter();
  painter->setBrush(QColorFromCA(back));
  painter->drawRoundedRect(QRectFFromPRect(rc), 25, 25, Qt::RelativeSize);
}

void SurfaceImpl::AlphaRectangle(PRectangle rc, int cornerSize,
                                 ColourDesired fill, int alphaFill,
                                 ColourDesired outline, int alphaOutline,
                                 int flags) {
  QPainter *painter = GetPainter();

  QColor qOutline = QColorFromCA(outline);
  qOutline.setAlpha(alphaOutline);
  painter->setPen(QPen(qOutline));

  QColor qFill = QColorFromCA(fill);
  qFill.setAlpha(alphaFill);
  painter->setBrush(qFill);

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);
  painter->drawRoundedRect(QRectFFromPRect(rc), cornerSize, cornerSize);
  painter->restore();
}

void SurfaceImpl::GradientRectangle(PRectangle rc,
                                    const std::vector<ColourStop> &stops,
                                    GradientOptions options) {
  QLinearGradient gradient;
  switch (options) {
    case GradientOptions::leftToRight:
      gradient = QLinearGradient(rc.left, rc.top, rc.right, rc.top);
      break;

    case GradientOptions::topToBottom:
    default:
      gradient = QLinearGradient(rc.left, rc.top, rc.left, rc.bottom);
      break;
  }

  gradient.setSpread(QGradient::RepeatSpread);
  for (const ColourStop &stop : stops)
    gradient.setColorAt(stop.position, QColorFromCA(stop.colour));

  GetPainter()->fillRect(QRectFFromPRect(rc), QBrush(gradient));
}

void SurfaceImpl::DrawRGBAImage(PRectangle rc, int width, int height,
                                const unsigned char *pixels) {
  QImage image(pixels, width, height, QImage::Format_ARGB32_Premultiplied);
  image.setDevicePixelRatio(width / rc.Width());
  GetPainter()->drawImage(QPointF(rc.left, rc.top), image);
}

void SurfaceImpl::Ellipse(PRectangle rc, ColourDesired fore,
                          ColourDesired back) {
  PenColour(fore);

  QPainter *painter = GetPainter();
  painter->setBrush(QColorFromCA(back));
  painter->drawEllipse(QRectFFromPRect(rc));
}

void SurfaceImpl::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
  SurfaceImpl *source = static_cast<SurfaceImpl *>(&surfaceSource);
  QPixmap *pixmap = static_cast<QPixmap *>(source->device);
  GetPainter()->drawPixmap(rc.left, rc.top, *pixmap, from.x, from.y, -1, -1);
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc, Font &font, XYPOSITION ybase,
                                 std::string_view s, ColourDesired fore,
                                 ColourDesired back) {
  PenColour(fore);

  QString(s.data());

  QPainter *painter = GetPainter();
  if (FontID id = font.GetID())
    painter->setFont(*static_cast<QFont *>(id));

  painter->setBackground(QColorFromCA(back));
  painter->setBackgroundMode(Qt::OpaqueMode);
  painter->drawText(QPointF(rc.left, ybase),
                    QString::fromUtf8(s.data(), s.length()));
}

void SurfaceImpl::DrawTextClipped(PRectangle rc, Font &font, XYPOSITION ybase,
                                  std::string_view s, ColourDesired fore,
                                  ColourDesired back) {
  SetClip(rc);
  DrawTextNoClip(rc, font, ybase, s, fore, back);
  GetPainter()->setClipping(false);
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc, Font &font,
                                      XYPOSITION ybase, std::string_view s,
                                      ColourDesired fore) {
  PenColour(fore);

  QPainter *painter = GetPainter();
  if (FontID id = font.GetID())
    painter->setFont(*static_cast<QFont *>(id));

  painter->setBackgroundMode(Qt::TransparentMode);
  painter->drawText(QPointF(rc.left, ybase),
                    QString::fromUtf8(s.data(), s.length()));
}

void SurfaceImpl::SetClip(PRectangle rc) {
  GetPainter()->setClipRect(QRectFFromPRect(rc));
}

static size_t utf8LengthFromLead(unsigned char uch) {
  if (uch >= (0x80 + 0x40 + 0x20 + 0x10)) {
    return 4;
  } else if (uch >= (0x80 + 0x40 + 0x20)) {
    return 3;
  } else if (uch >= (0x80)) {
    return 2;
  } else {
    return 1;
  }
}

void SurfaceImpl::MeasureWidths(Font &font, std::string_view s,
                                XYPOSITION *positions) {
  if (!font.GetID())
    return;
  QString su = QString::fromUtf8(s.data(), s.length());
  QTextLayout tlay(su, *static_cast<QFont *>(font.GetID()), device);
  tlay.beginLayout();
  QTextLine tl = tlay.createLine();
  tlay.endLayout();

  size_t i = 0;
  int ui = 0;
  int fit = su.size();
  const unsigned char *us = reinterpret_cast<const unsigned char *>(s.data());
  while (ui < fit) {
    size_t lenChar = utf8LengthFromLead(us[i]);
    int codeUnits = (lenChar < 4) ? 1 : 2;
    qreal xPosition = tl.cursorToX(ui + codeUnits);
    for (unsigned int bytePos = 0; (bytePos < lenChar) && (i < s.length());
         ++bytePos)
      positions[i++] = xPosition;
    ui += codeUnits;
  }

  XYPOSITION lastPos = 0;
  if (i > 0)
    lastPos = positions[i - 1];
  while (i < s.length())
    positions[i++] = lastPos;
}

XYPOSITION SurfaceImpl::WidthText(Font &font, std::string_view s) {
  QFontMetricsF metrics(*static_cast<QFont *>(font.GetID()), device);
  return metrics.horizontalAdvance(QString::fromUtf8(s.data(), s.length()));
}

XYPOSITION SurfaceImpl::Ascent(Font &font) {
  QFontMetricsF metrics(*static_cast<QFont *>(font.GetID()), device);
  return metrics.ascent();
}

XYPOSITION SurfaceImpl::Descent(Font &font) {
  QFontMetricsF metrics(*static_cast<QFont *>(font.GetID()), device);
  // Qt returns 1 less than true descent
  // See: QFontEngineWin::descent which says:
  // ### we subtract 1 to even out the historical +1 in QFontMetrics's
  // ### height=asc+desc+1 equation. Fix in Qt5.
  return metrics.descent() + 1;
}

XYPOSITION SurfaceImpl::InternalLeading(Font &font) { return 0; }

XYPOSITION SurfaceImpl::Height(Font &font) {
  QFontMetricsF metrics(*static_cast<QFont *>(font.GetID()), device);
  return metrics.height();
}

XYPOSITION SurfaceImpl::AverageCharWidth(Font &font) {
  QFontMetricsF metrics(*static_cast<QFont *>(font.GetID()), device);
  return metrics.averageCharWidth();
}

void SurfaceImpl::FlushCachedState() {
  if (device->paintingActive()) {
    QPainter *painter = GetPainter();
    painter->setPen(QPen());
    painter->setBrush(QBrush());
  }
}

QPainter *SurfaceImpl::GetPainter() {
  Q_ASSERT(device);

  if (!painter) {
    painter = new QPainter(device);

    // Set text antialiasing unconditionally.
    // The font's style strategy will override.
    painter->setRenderHint(QPainter::TextAntialiasing, true);
  }

  return painter;
}

Surface *Surface::Allocate(int) { return new SurfaceImpl; }

//----------------------------------------------------------------------

static QWidget *window(WindowID wid) { return static_cast<QWidget *>(wid); }

Window::~Window() {}

void Window::Destroy() {
  if (wid)
    delete window(wid);

  wid = 0;
}

PRectangle Window::GetPosition() const {
  // Before any size allocated pretend its 1000 wide so not scrolled
  return wid ? PRectFromQRect(window(wid)->frameGeometry())
             : PRectangle(0, 0, 1000, 1000);
}

void Window::SetPosition(PRectangle rc) {
  if (wid)
    window(wid)->setGeometry(QRectFromPRect(rc));
}

void Window::SetPositionRelative(PRectangle rc, const Window *relativeTo) {
  QPoint oPos = window(relativeTo->wid)->mapToGlobal(QPoint(0, 0));
  int ox = oPos.x();
  int oy = oPos.y();
  ox += rc.left;
  oy += rc.top;

  int sizex = rc.right - rc.left;
  int sizey = rc.bottom - rc.top;

  if (QScreen *screen = QApplication::screenAt(QPoint(ox, oy))) {
    QRect rectDesk = screen->availableGeometry();
    /* do some corrections to fit into screen */
    int screenWidth = rectDesk.width();
    if (ox < rectDesk.x())
      ox = rectDesk.x();
    if (sizex > screenWidth)
      ox = rectDesk.x(); /* the best we can do */
    else if (ox + sizex > rectDesk.right())
      ox = rectDesk.right() - sizex;
    if (oy + sizey > rectDesk.bottom())
      oy = rectDesk.bottom() - sizey;
  }

  Q_ASSERT(wid);
  window(wid)->move(ox, oy);
  window(wid)->resize(sizex, sizey);
}

PRectangle Window::GetClientPosition() const {
  // The client position is the window position
  return GetPosition();
}

void Window::Show(bool show) {
  if (wid)
    window(wid)->setVisible(show);
}

void Window::InvalidateAll() {
  if (wid)
    window(wid)->update();
}

void Window::InvalidateRectangle(PRectangle rc) {
  if (wid)
    window(wid)->update(QRectFromPRect(rc));
}

void Window::SetFont(Font &font) {
  if (wid)
    window(wid)->setFont(*static_cast<QFont *>(font.GetID()));
}

void Window::SetCursor(Cursor curs) {
  if (wid) {
    Qt::CursorShape shape;

    switch (curs) {
      case cursorText:
        shape = Qt::IBeamCursor;
        break;
      case cursorUp:
        shape = Qt::UpArrowCursor;
        break;
      case cursorWait:
        shape = Qt::WaitCursor;
        break;
      case cursorHoriz:
        shape = Qt::SizeHorCursor;
        break;
      case cursorVert:
        shape = Qt::SizeVerCursor;
        break;
      case cursorHand:
        shape = Qt::PointingHandCursor;
        break;
      default:
        shape = Qt::ArrowCursor;
        break;
    }

    QCursor cursor = QCursor(shape);

    if (curs != cursorLast) {
      window(wid)->setCursor(cursor);
      cursorLast = curs;
    }
  }
}

/* Returns rectangle of monitor pt is on, both rect and pt are in Window's
   window coordinates */
PRectangle Window::GetMonitorRect(Point pt) {
  QPoint originGlobal = window(wid)->mapToGlobal(QPoint(0, 0));
  QPoint posGlobal = window(wid)->mapToGlobal(QPoint(pt.x, pt.y));
  if (QScreen *screen = QApplication::screenAt(posGlobal)) {
    QRect rectScreen = screen->availableGeometry();
    rectScreen.translate(-originGlobal.x(), -originGlobal.y());
    return PRectangle(rectScreen.left(), rectScreen.top(), rectScreen.right(),
                      rectScreen.bottom());
  }

  return PRectangle();
}

//----------------------------------------------------------------------

class ListBoxImpl : public ListBox {
public:
  void SetFont(Font &) override {}
  void Create(Window &, int, Point, int, bool, int) override {}
  void SetAverageCharWidth(int) override {}
  void SetVisibleRows(int) override {}
  int GetVisibleRows() const override { return 0; }
  PRectangle GetDesiredRect() override { return PRectangle(); }
  int CaretFromEdge() override { return 0; }
  void Clear() override {}
  void Append(char *, int = -1) override {}
  int Length() override { return 0; }
  void Select(int) override {}
  int GetSelection() override { return 0; }
  int Find(const char *) override { return 0; }
  void GetValue(int, char *, int) override {}
  void RegisterImage(int, const char *) override {}
  void RegisterRGBAImage(int, int, int, const unsigned char *) override {}
  void ClearRegisteredImages() override {}
  void SetDelegate(IListBoxDelegate *) override {}
  void SetList(const char *, char, char) override {}
};

ListBox::ListBox() noexcept {}

ListBox::~ListBox() {}

ListBox *ListBox::Allocate() { return new ListBoxImpl; }

//----------------------------------------------------------------------

Menu::Menu() noexcept : mid(0) {}

void Menu::CreatePopUp() {
  Destroy();
  mid = new QMenu;
}

void Menu::Destroy() {
  if (mid) {
    QMenu *menu = static_cast<QMenu *>(mid);
    delete menu;
  }
  mid = 0;
}

void Menu::Show(Point pt, Window & /*w*/) {
  QMenu *menu = static_cast<QMenu *>(mid);
  menu->exec(QPoint(pt.x, pt.y));
  Destroy();
}

//----------------------------------------------------------------------

class DynamicLibraryImpl : public DynamicLibrary {
protected:
  QLibrary *lib;

public:
  explicit DynamicLibraryImpl(const char *modulePath) {
    QString path = QString::fromUtf8(modulePath);
    lib = new QLibrary(path);
  }

  virtual ~DynamicLibraryImpl() {
    if (lib)
      lib->unload();
    lib = 0;
  }

  Function FindFunction(const char *name) override {
    return lib ? reinterpret_cast<Function>(lib->resolve(name)) : NULL;
  }

  bool IsValid() override { return lib != NULL; }
};

DynamicLibrary *DynamicLibrary::Load(const char *modulePath) {
  return static_cast<DynamicLibrary *>(new DynamicLibraryImpl(modulePath));
}

ColourDesired Platform::Chrome() {
  QColor c(Qt::gray);
  return ColourDesired(c.red(), c.green(), c.blue());
}

ColourDesired Platform::ChromeHighlight() {
  QColor c(Qt::lightGray);
  return ColourDesired(c.red(), c.green(), c.blue());
}

const char *Platform::DefaultFont() {
  static char fontNameDefault[200] = "";
  if (!fontNameDefault[0]) {
    QFont font = QApplication::font();
    strcpy(fontNameDefault, font.family().toUtf8());
  }
  return fontNameDefault;
}

int Platform::DefaultFontSize() {
  QFont font = QApplication::font();
  return font.pointSize();
}

unsigned int Platform::DoubleClickTime() {
  return QApplication::doubleClickInterval();
}

void Platform::DebugDisplay(const char *s) { qWarning("Scintilla: %s", s); }

void Platform::DebugPrintf(const char *format, ...) {
  char buffer[2000];
  va_list pArguments;
  va_start(pArguments, format);
  vsnprintf(buffer, sizeof(buffer), format, pArguments);
  va_end(pArguments);
  Platform::DebugDisplay(buffer);
}

void Platform::Assert(const char *c, const char *file, int line) {
  char buffer[2000];
  snprintf(buffer, sizeof(buffer), "Assertion [%s] failed at %s %d\n", c, file, line);
  Platform::DebugDisplay(buffer);
}

} // namespace Scintilla
