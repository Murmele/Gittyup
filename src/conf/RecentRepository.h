//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef RECENTREPOSITORY_H
#define RECENTREPOSITORY_H

#include <QObject>

class RecentRepository : public QObject {
  Q_OBJECT

public:
  RecentRepository(const QString &path, QObject *parent = nullptr);

  QString path() const;
  QString name() const;

private:
  void increment();

  QString mPath;
  int mSections = 1;

  friend class RecentRepositories;
};

#endif
