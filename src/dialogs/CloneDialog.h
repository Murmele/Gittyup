//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CLONEDIALOG_H
#define CLONEDIALOG_H

#include "host/Repository.h"
#include <QFutureWatcher>
#include <QWizard>

class LogEntry;
class LogView;
class RemoteCallbacks;
class QComboBox;
class QLineEdit;
class QPushButton;

namespace git {
class Result;
}

class CloneDialog : public QWizard {
  Q_OBJECT

public:
  enum Kind { Init, Clone };

  CloneDialog(Kind kind, QWidget *parent = nullptr, Repository *repo = nullptr);

  void accept() override;

  QString path() const;
  QString message() const;
  QString messageTitle() const;
};

#endif
