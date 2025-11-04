#include "reduce.h"
#include "idstorage.h"

void Reduce::run() {
  Index::PostingMap result;
  std::optional<Intermediate> intermediateOpt = mQueue.dequeue();
  while (!canceled && intermediateOpt.has_value()) {
    auto intermediate = intermediateOpt.value();
    mProcessedElemenets++;
    log("reduce: %1", intermediate.id);

    quint32 id = mIds.append(intermediate.id);

    Intermediate::FieldMap::const_iterator it;
    Intermediate::FieldMap::const_iterator end = intermediate.fields.end();
    for (it = intermediate.fields.begin(); it != end; ++it) {
      Intermediate::TermMap::const_iterator termIt;
      Intermediate::TermMap::const_iterator termEnd = it.value().end();
      for (termIt = it.value().begin(); termIt != termEnd; ++termIt) {
        Index::Posting posting;
        posting.id = id;
        posting.field = it.key();
        posting.positions = termIt.value();
        result[termIt.key()].append(posting);
      }
    }

    if (mProcessedElemenets >= 8192) {
      mResults.enqueue(std::move(result));
      result = Index::PostingMap();
      mProcessedElemenets = 0;
    }

    // Grab next value
    intermediateOpt = mQueue.dequeue();
  }
  if (mProcessedElemenets > 0)
    mResults.enqueue(std::move(result));
}
