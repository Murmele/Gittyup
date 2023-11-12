//
//          Copyright (c) 2021, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: rlbxku1r
//

#ifndef PATCHDIALOG_H
#define PATCHDIALOG_H

#include "git/Commit.h"
#include "log/LogEntry.h"
#include <QCoreApplication>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>

class ApplyPatchDialog {
  Q_DECLARE_TR_FUNCTIONS(ApplyPatchDialog)

public:
  static QString getOpenFileName(QWidget *parent);

private:
  static QString filter();
};

class SavePatchDialog : public QDialog {
  Q_OBJECT

public:
  SavePatchDialog(const QList<git::Commit> &commits, QWidget *parent);
  SavePatchDialog(const QByteArray &buffer, const QStringList &files,
                  QWidget *parent);

private:
  enum class Format { Diff, Mailbox, SingleMailbox };

  void setupUI(bool patch);
  void save() const;
  void savePatch() const;
  void saveMailbox() const;
  bool saveFile(LogEntry *parent, const QString &path,
                const QByteArray &buffer) const;

  QByteArray generatePatch() const;

  void selectFormat(Format format);
  void updateSaveButton();

  QString mailboxFileName(int num, const git::Commit &commit) const;
  QString fileExtension() const;
  QString outputFilePath() const;

  void walkCommits(const std::function<void(int num, const git::Commit &commit)>
                       &callback) const;

  Format mFormat;
  QList<git::Commit> mCommits;
  QStringList mFiles;
  QByteArray mBuffer;

  QLineEdit *mDir, *mFile;
  QLabel *mFileExt;
  QListView *mFileList;
  QPushButton *mSaveButton;
};

#endif
