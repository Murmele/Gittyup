#ifndef MARKINGREPOSITORY_H
#define MARKINGREPOSITORY_H

#include "RecentRepository.h"

class MarkingRepository : public RecentRepository {
  Q_OBJECT
public:
  MarkingRepository(const QString &gitpath, QObject *parent = nullptr)
      : RecentRepository(gitpath, parent) {}
};

#endif // MARKINGREPOSITORY_H
