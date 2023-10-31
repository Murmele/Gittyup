//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CustomTheme.h"
#include <QWindow>
#import <Cocoa/Cocoa.h>

void CustomTheme::polishWindow(QWindow *window) const
{
  QVariantMap titlebar = mMap.value("titlebar").toMap();
  QVariant variant = titlebar.value("background");
  if (!variant.canConvert<QColor>())
    return;

  NSWindow *win = [reinterpret_cast<NSView *>(window->winId()) window];
  win.titlebarAppearsTransparent = YES;

  QColor color = variant.value<QColor>();
  if (color.lightnessF() < 0.5)
    win.appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark];

  float r,g,b,a;
  color.getRgbF(&r,&g,&b,&a);
  win.backgroundColor = [NSColor colorWithDeviceRed:r green:g blue:b alpha:a];
}
