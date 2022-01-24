//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Application.h"
#include "ui/MainWindow.h"
#include <QUrl>
#import <AppKit/AppKit.h>

@interface Launcher : NSObject
- (void)openInGittyup:(NSPasteboard *)pboard
  userData:(NSString *)userData
  error:(NSString **)error;
@end

@implementation Launcher
- (void)openInGitAhead:(NSPasteboard *)pboard
  userData:(NSString *)userData
  error:(NSString **)error
{
  NSArray *classes = [NSArray arrayWithObject:[NSURL class]];
  NSDictionary *options = [NSDictionary dictionary];
  NSArray *urls = [pboard readObjectsForClasses:classes options:options];
  for (id url in urls)
    MainWindow::open(QUrl::fromNSURL([url filePathURL]).toLocalFile(), false);
  [pboard clearContents];
}
@end

void Application::registerService()
{
  Launcher *launcher = [[Launcher alloc] init];
  [[NSApplication sharedApplication] setServicesProvider:launcher];
}
