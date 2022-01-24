//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QMainWindow>

class SettingsDialog : public QMainWindow
{
  Q_OBJECT

public:
  enum Index
  {
    General,
    Diff,
    Tools,
    Window,
    Editor,
    Update,
    Plugins,
    Misc,
    Hotkeys,
    Terminal
  };

  SettingsDialog(Index index, QWidget *parent = nullptr);

  static void openSharedInstance(Index index = General);
};

#endif
