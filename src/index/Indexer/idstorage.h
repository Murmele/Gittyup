#ifndef IDSTORAGE_H
#define IDSTORAGE_H

#include "git/Id.h"

#include <QReadWriteLock>
#include <QSet>

class Index;

/// @brief Overlay over Index::ids() in order to allow multi-threading and
/// unnecessary copying of the list
class IdStorage {
public:
  IdStorage(Index &index);

  bool contains(const git::Id &id);
  qsizetype append(const git::Id &id);

private:
  QReadWriteLock mLock;
  Index &mIndex;
  QSet<git::Id> mIds;
};

#endif // IDSTORAGE_H
