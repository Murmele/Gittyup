//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "MainWindow.h"
#include "RepoView.h"
#include "app/Application.h"
#include <QPainter>
#include <QString>
#import <AppKit/AppKit.h>

namespace {

// Icon sizes correspond to the logical preferred and maximum sizes
// allowed in the touch bar. Actual physical dimensions are double.
const qreal kRatio = 2.0;
const int kIconSize = 22;
const int kMaxIconSize = 30;

NSImage *image(const QString &path, int badge = -1)
{
  QIcon icon(path);
  QPixmap pixmap = icon.pixmap(kIconSize);
  if (badge > 0) {
    int extent = kMaxIconSize * kRatio;
    QPixmap composite(extent, extent);
    composite.setDevicePixelRatio(kRatio);
    composite.fill(Qt::transparent);

    QPainter painter(&composite);
    painter.setRenderHints(QPainter::Antialiasing);

    int offset = (kMaxIconSize - kIconSize) / kRatio;
    painter.drawPixmap(offset, offset, pixmap);

    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    QFontMetrics fm = painter.fontMetrics();

    QString text = (badge > 999) ? "999+" : QString::number(badge);
    int width = fm.horizontalAdvance(text) + 8;
    QRect rect(kMaxIconSize - width, 4, width, fm.lineSpacing() + 2);

    Theme *theme = Application::theme();
    auto state = Theme::BadgeState::Notification;
    QColor color = theme->badge(Theme::BadgeRole::Background, state);
    color.setAlphaF(0.6);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect, 6, 6);

    painter.setPen(theme->badge(Theme::BadgeRole::Foreground, state));
    painter.drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, text);

    pixmap = composite;
  }

  QImage image = pixmap.toImage();
  CGImageRef cgImage = image.toCGImage();
  NSImage *nsImage = [[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize];
  nsImage.size = (pixmap.size() / pixmap.devicePixelRatioF()).toCGSize();
  CGImageRelease(cgImage);
  return nsImage;
}

} // anon. namespace

@interface TouchBarProvider: NSResponder <NSTouchBarDelegate, NSWindowDelegate>

@property (strong) NSCustomTouchBarItem *remoteItem;
@property (strong) NSSegmentedControl *remote;

@property (strong) NSObject *delegate;

@end

static const NSTouchBarItemIdentifier kRemote = @"com.gittyup.Remote";

@implementation TouchBarProvider {
  MainWindow *_window;
  int _ahead, _behind;
}

- (id)initWithWindow:(MainWindow *)window
{
  if (self = [super init]) {
    _window = window;
    _ahead = -1;
    _behind = -1;

    NSView *view = reinterpret_cast<NSView *>(window->winId());
    _delegate = view.window.delegate;
    view.window.delegate = self;
  }

  return self;
}

- (void)updateRemoteAhead:(int)ahead behind:(int)behind
{
  NSImage *pull = image(":/pull.png", behind);
  [self.remote setImage:pull forSegment:1];
  [pull release];

  NSImage *push = image(":/push.png", ahead);
  [self.remote setImage:push forSegment:2];
  [push release];

  [self.remote setEnabled:(_window->currentView() != nullptr)];

  _ahead = ahead;
  _behind = behind;
}

- (NSTouchBar *)makeTouchBar
{
  NSTouchBar *bar = [[NSTouchBar alloc] init];
  bar.delegate = self;
  bar.defaultItemIdentifiers = @[kRemote];
  return bar;
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar
    makeItemForIdentifier:(NSTouchBarItemIdentifier)ident
{
  Q_UNUSED(touchBar);

  if ([ident isEqualToString:kRemote]) {
    NSImage *fetch = image(":/fetch.png");
    NSImage *pull = image(":/pull.png");
    NSImage *push = image(":/push.png");

    self.remoteItem =
      [[[NSCustomTouchBarItem alloc] initWithIdentifier:kRemote] autorelease];
    self.remote =
      [[NSSegmentedControl segmentedControlWithImages:@[fetch, pull, push]
        trackingMode:NSSegmentSwitchTrackingMomentary
        target:self action:@selector(remoteClicked)] autorelease];
    self.remote.segmentStyle = NSSegmentStyleSeparated;
    self.remoteItem.view = self.remote;

    [fetch release];
    [pull release];
    [push release];

    [self updateRemoteAhead:_ahead behind:_behind];

    return self.remoteItem;
  }

  return nil;
}

- (BOOL)respondsToSelector:(SEL)selector
{
  return [_delegate respondsToSelector:selector] ||
    [super respondsToSelector:selector];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
  [invocation invokeWithTarget:_delegate];
}

- (void)remoteClicked
{
  switch ([self.remote selectedSegment]) {
    case 0:
      _window->currentView()->fetch();
      break;

    case 1:
      _window->currentView()->pull();
      break;

    case 2:
      _window->currentView()->push();
      break;
  }
}

@end

void MainWindow::installTouchBar()
{
  [[TouchBarProvider alloc] initWithWindow:this];
}

void MainWindow::updateTouchBar(int ahead, int behind)
{
  NSView *view = reinterpret_cast<NSView *>(winId());
  if (view && [view.window.delegate isKindOfClass:[TouchBarProvider class]]) {
    TouchBarProvider *provider = (TouchBarProvider *) view.window.delegate;
    [provider updateRemoteAhead:ahead behind:behind];
  }
}
