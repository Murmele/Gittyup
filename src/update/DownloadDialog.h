//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Updater.h"
#include <QDialog>

class DownloadDialog : public QDialog {
  Q_OBJECT

public:
  DownloadDialog(const Updater::DownloadRef &download,
                 QWidget *parent = nullptr);
};
