//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef BLAMEEDITOR_H
#define BLAMEEDITOR_H

#include "FindWidget.h"
#include "git/Blame.h"
#include "git/Blob.h"
#include "git/Commit.h"
#include "git/Repository.h"
#include <QFutureWatcher>
#include <QScopedPointer>
#include <QWidget>

class BlameMargin;
class TextEditor;

class BlameEditor : public QWidget, public EditorProvider {
  Q_OBJECT

public:
  BlameEditor(const git::Repository &repo = git::Repository(),
              QWidget *parent = nullptr);

  QString name() const;
  QString path() const;
  QString revision() const;

  TextEditor *editor() const { return mEditor; }
  QList<TextEditor *> editors() override { return {mEditor}; }
  void ensureVisible(TextEditor *editor, int pos) override {}

  bool load(const QString &name, const git::Blob &blob, const git::Commit &from,
            const git::Commit &to);

  void cancelBlame();

  void save();
  void clear();

  void find();
  void findNext();
  void findPrevious();

signals:
  void saved();
  void linkActivated(const QString &link);

private:
  void adjustLineMarginWidth();

  git::Repository mRepo;

  TextEditor *mEditor;
  FindWidget *mFind;
  BlameMargin *mMargin;

  QString mName;
  QString mRevision;

  QScopedPointer<git::Blame::Callbacks> mCallbacks;
  QFutureWatcher<git::Blame> mBlame;
};

#endif
