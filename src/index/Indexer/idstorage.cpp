#include "idstorage.h"
#include "index/Index.h"
#include "git/Id.h"

IdStorage::IdStorage(Index &index)
    : mIndex(index), mIds(mIndex.ids().begin(), mIndex.ids().end()) {}

bool IdStorage::contains(const git::Id &id) {
  bool result;
  mLock.lockForRead();
  result = mIds.contains(id);
  mLock.unlock();
  return result;
}

qsizetype IdStorage::append(const git::Id &id) {
  auto &listIds = mIndex.ids();
  qsizetype size;
  mLock.lockForWrite();
  size = listIds.size();
  listIds.append(id);
  mIds.insert(id);
  mLock.unlock();
  return size;
}
