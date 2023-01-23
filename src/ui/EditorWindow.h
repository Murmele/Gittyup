//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include "git/Blob.h"
#include "git/Commit.h"
#include "git/Repository.h"

class BlameEditor;

class EditorWindow : public QMainWindow {
  Q_OBJECT

public:
  EditorWindow(const git::Repository &repo = git::Repository(),
               QWidget *parent = nullptr);

  BlameEditor *widget() const;

  void updateWindowTitle();

  static EditorWindow *open(const QString &path,
                            const git::Blob &blob = git::Blob(),
                            const git::Commit &commit = git::Commit(),
                            const git::Repository &repo = git::Repository());

protected:
  void showEvent(QShowEvent *event) override;
  void closeEvent(QCloseEvent *event) override;
};

#endif
