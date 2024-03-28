//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Application.h"
#include "git/Config.h"
#include "ui/MainWindow.h"
#include <QMessageBox>

int main(int argc, char *argv[]) {
  Application app(argc, argv, true);

  // Check if only one running instance is allowed and already running
  if (app.runSingleInstance())
    return 0;

  if (!git::Config::global().isValid()) {
    QMessageBox::warning(
        nullptr, GITTYUP_NAME,
        QObject::tr("Your global GIT configuration is invalid, Gittyup won't "
                    "run properly until this is fixed"));
  }

  // Restore windows before checking for updates so that
  // the update dialog pops up on top of the other windows.
  if (!app.restoreWindows())
    MainWindow::open();

  // Check for updates.
  app.autoUpdate();

  return app.exec();
}
