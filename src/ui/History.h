//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef HISTORY_H
#define HISTORY_H

#include "Location.h"
#include <QObject>

class RepoView;
class QMenu;

class History : public QObject {
  Q_OBJECT

public:
  History(RepoView *view);

  bool hasNext() const;
  bool hasPrev() const;

  void next();
  void prev();
  void setIndex(int index);

  void update(const Location &location, bool spontaneous);
  void updatePrevMenu(QMenu *menu);
  void updateNextMenu(QMenu *menu);

  // Remove uncommitted entries.
  void clean();

signals:
  void changed();

private:
  RepoView *mView;

  int mIndex;
  QList<Location> mLocs;
  Location mPendingLoc;
};

#endif
