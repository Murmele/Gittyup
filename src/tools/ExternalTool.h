//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef EXTERNALTOOL_H
#define EXTERNALTOOL_H

#include <QObject>
#include <QString>
#include "git/Diff.h"
#include "git/Repository.h"

class ExternalTool : public QObject {
  Q_OBJECT

public:
  enum Kind { Show, Edit, Diff, Merge };

  enum Error { BashNotFound };

  struct Info {
    QString name;
    QString command;
    QString arguments;
    bool found;

    bool operator<(const Info &rhs) const {
      if (found != rhs.found)
        return found;

      return (name < rhs.name);
    }
  };

  ExternalTool(const QStringList &files, const git::Diff &diff,
               const git::Repository &repo, QObject *parent);

  virtual bool isValid() const;

  virtual Kind kind() const = 0;
  virtual QString name() const = 0;

  virtual bool start() = 0;

  bool isConflicted(const QString &file) const;

  static QString lookupCommand(const QString &key, bool &shell);
  static QList<Info> readGlobalTools(const QString &key);
  static QList<Info> readBuiltInTools(const QString &key);

signals:
  void error(Error error);

protected:
  QStringList mFiles;
  git::Diff mDiff;
  git::Repository mRepo;
};

#endif
