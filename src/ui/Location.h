//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef LOCATION_H
#define LOCATION_H

#include "RepoView.h"
#include <QByteArray>
#include <QString>

namespace git {
class Repository;
}

class Location {
  Q_DECLARE_TR_FUNCTIONS(Location)

public:
  Location();
  Location(RepoView::ViewMode mode, const QString &ref, const QString &id,
           const QString &file);

  bool isValid() const { return mValid; }
  bool isUncommitted() const { return mId.isEmpty(); }

  RepoView::ViewMode mode() const { return mMode; }
  QString ref() const { return mRef; }
  QString id() const { return mId; }
  QString file() const { return mFile; }

  QString toString(const git::Repository &repo) const;

  bool operator==(const Location &rhs) const;

private:
  RepoView::ViewMode mMode = RepoView::DoubleTree;
  QString mRef;
  QString mId;
  QString mFile;
  bool mValid = false;
};

#endif
