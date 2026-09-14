//
//          Copyright (c) 2026
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
// Derived from: https://github.com/gitahead/gitahead/pull/560
//

#include "DiffFileDialog.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QString>

namespace {

const QString kLastDirKey = "diff/lastdir";

} // namespace

QString DiffFileDialog::getApplyFileName(QWidget *parent) {
  QString path = QFileDialog::getOpenFileName(parent, tr("Apply Diff File"),
                                              lastDir(), filter());
  saveLastDir(path);
  return path;
}

QString DiffFileDialog::getSaveFileName(QWidget *parent) {
  QString path = QFileDialog::getSaveFileName(parent, tr("Save Diff File"),
                                              lastDir(), filter());
  saveLastDir(path);
  return path;
}

QString DiffFileDialog::filter() {
  return tr("Git Diff (*.diff *.patch);;All files (*)");
}

QString DiffFileDialog::lastDir() {
  QString dir = QSettings().value(kLastDirKey).toString();

  if (dir.isEmpty() || !QDir(dir).exists())
    dir = QDir::homePath();

  return dir;
}

void DiffFileDialog::saveLastDir(const QString &path) {
  if (path.isEmpty())
    return;

  QString dir = QFileInfo(path).absolutePath();
  QSettings().setValue(kLastDirKey, dir);
}
