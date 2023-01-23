//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef PLATQT_H
#define PLATQT_H

#include <cstddef>
#include <vector>
#include <string_view>
#include <memory>
#include "Platform.h"
#include <QColor>
#include <QRect>

class QPaintDevice;
class QPainter;

namespace Scintilla {

inline QColor QColorFromCA(ColourDesired ca) {
  int c = ca.AsInteger();
  return QColor(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
}

inline QColor QColorFromCA(ColourAlpha ca) {
  return QColor(ca.GetRed(), ca.GetGreen(), ca.GetBlue(), ca.GetAlpha());
}

inline QRect QRectFromPRect(PRectangle pr) {
  return QRect(pr.left, pr.top, pr.Width(), pr.Height());
}

inline QRectF QRectFFromPRect(PRectangle pr) {
  return QRectF(pr.left, pr.top, pr.Width(), pr.Height());
}

inline PRectangle PRectFromQRect(QRect qr) {
  return PRectangle(qr.x(), qr.y(), qr.x() + qr.width(), qr.y() + qr.height());
}

inline Point PointFromQPoint(QPoint qp) { return Point(qp.x(), qp.y()); }

class SurfaceImpl : public Surface {
private:
  QPaintDevice *device = nullptr;
  QPainter *painter = nullptr;
  bool deviceOwned = false;
  float x = 0.0, y = 0.0;

public:
  SurfaceImpl();
  virtual ~SurfaceImpl();

  void Init(WindowID wid) override;
  void Init(SurfaceID sid, WindowID wid) override;
  void InitPixMap(int width, int height, Surface *surface,
                  WindowID wid) override;

  void Release() override;
  bool Initialised() override;
  void PenColour(ColourDesired fore) override;
  int LogPixelsY() override;
  int DeviceHeightFont(int points) override;
  void MoveTo(int x, int y) override;
  void LineTo(int x, int y) override;
  void Polygon(Point *pts, size_t npts, ColourDesired fore,
               ColourDesired back) override;
  void RectangleDraw(PRectangle rc, ColourDesired fore,
                     ColourDesired back) override;
  void FillRectangle(PRectangle rc, ColourDesired back) override;
  void FillRectangle(PRectangle rc, Surface &surfacePattern) override;
  void RoundedRectangle(PRectangle rc, ColourDesired fore,
                        ColourDesired back) override;
  void AlphaRectangle(PRectangle rc, int corner, ColourDesired fill,
                      int alphaFill, ColourDesired outline, int alphaOutline,
                      int flags) override;
  void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops,
                         GradientOptions options) override;
  void DrawRGBAImage(PRectangle rc, int width, int height,
                     const unsigned char *pixelsImage) override;
  void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) override;
  void Copy(PRectangle rc, Point from, Surface &surfaceSource) override;

  virtual std::unique_ptr<IScreenLineLayout>
  Layout(const IScreenLine *screenLine) override { /* TODO: implement */
    return nullptr;
  }

  void DrawTextNoClip(PRectangle rc, Font &font, XYPOSITION ybase,
                      std::string_view text, ColourDesired fore,
                      ColourDesired back) override;
  void DrawTextClipped(PRectangle rc, Font &font, XYPOSITION ybase,
                       std::string_view text, ColourDesired fore,
                       ColourDesired back) override;
  void DrawTextTransparent(PRectangle rc, Font &font, XYPOSITION ybase,
                           std::string_view text, ColourDesired fore) override;
  void MeasureWidths(Font &font, std::string_view s,
                     XYPOSITION *positions) override;
  XYPOSITION WidthText(Font &font, std::string_view s) override;
  XYPOSITION Ascent(Font &font) override;
  XYPOSITION Descent(Font &font) override;
  XYPOSITION InternalLeading(Font &font) override;
  XYPOSITION Height(Font &font) override;
  XYPOSITION AverageCharWidth(Font &font) override;

  void SetClip(PRectangle rc) override;
  void FlushCachedState() override;

  void SetUnicodeMode(bool unicodeMode) override {}
  void SetDBCSMode(int codePage) override {}
  void SetBidiR2L(bool bidiR2L_) override{/* TODO: implement */};

  QPainter *GetPainter();
};

} // namespace Scintilla

#endif
