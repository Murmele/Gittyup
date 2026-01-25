//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Updater.h"
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QProcess>
#include <QSharedPointer>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <AppKit/NSWorkspace.h>
#include <Foundation/NSBundle.h>
#include <Foundation/NSFileManager.h>

namespace {

const QString kBundleFmt = "%1.app";

void unmount(const QString &point)
{
  QProcess::execute("hdiutil", {"detach", "-quiet", point});
}

bool mount(const QString &path, const QString &root)
{
  // Mount the disk image.
  QProcess process;
  process.setStandardOutputFile(QProcess::nullDevice());
  process.start("hdiutil", {"attach", "-mountroot", root, path});
  process.closeWriteChannel();
  process.waitForFinished();
  return (!process.exitStatus() && !process.exitCode());
}

class DiskImage
{
public:
  DiskImage(const QString &path)
  {
    // Mount the disk image.
    if (mDir.isValid())
      mValid = mount(path, mDir.path());
  }

  ~DiskImage()
  {
    // Unmount the disk image.
    if (mValid)
      unmount(mountPoint());
  }

  bool isValid() const { return mValid; }

  QString mountPoint() const
  {
    return QDir(mDir.path()).filePath(QCoreApplication::applicationName());
  }

private:
  bool mValid = false;
  QTemporaryDir mDir;
};

} // anon. namespace

bool Updater::install(const DownloadRef &download, QString &error)
{
  DiskImage image(download->file()->fileName());
  if (!image.isValid()) {
    error = tr("The disk image failed to mount successfully");
    return false;
  }

  // Move the existing bundle to the trash.
  __block NSError *recycleError = nil;
  QSharedPointer<QEventLoop> loop(new QEventLoop);
  NSBundle *bundle = [NSBundle mainBundle];
  NSURL *url = [bundle bundleURL];
  NSString *path = [bundle bundlePath];
  [[NSWorkspace sharedWorkspace] recycleURLs:@[url] completionHandler:
  ^(NSDictionary<NSURL *, NSURL *> *newURLs, NSError *error) {
    recycleError = error;
    loop->quit();
  }];

  // Wait for recycle.
  loop->exec();
  if (recycleError) {
    error = tr("The existing bundle could not be moved to the trash");
    return false;
  }

  // Copy the new bundle from the disk image.
  // FIXME: There's no way to undo the recycle if copy fails.
  NSError *copyError = nil;
  NSFileManager *mgr = [NSFileManager defaultManager];
  QString name = kBundleFmt.arg(QCoreApplication::applicationName());
  QString src = QDir(image.mountPoint()).filePath(name);
  [mgr copyItemAtPath:src.toNSString() toPath:path error:&copyError];
  if (copyError) {
    error = tr("The new bundle could not be copied into place");
    return false;
  }

  // Start the relaunch helper.
  QDir dir(QCoreApplication::applicationDirPath());
  QString app = QCoreApplication::applicationFilePath();
  QString pid = QString::number(QCoreApplication::applicationPid());
  if (!QProcess::startDetached(dir.filePath("gittyup-relauncher"), {app, pid})) {
    error = tr("Helper application failed to start");
    return false;
  }

  return true;
}
