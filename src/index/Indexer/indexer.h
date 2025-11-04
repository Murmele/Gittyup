#ifndef INDEXER
#define INDEXER

#include "lexerpool.h"
#include "git/RevWalk.h"
#include "idstorage.h"
#include "resultwriter.h"
#include "reduce.h"
#include "index/Lexer.h"

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QThreadPool>

class Index;
class QFile;

void index(const Lexer::Lexeme &lexeme, Intermediate::FieldMap &fields, quint8 field, quint32 &pos);

class Indexer : public QObject, public QAbstractNativeEventFilter {
public:
  Indexer(Index &index, QFile *out, bool notify, QObject *parent = nullptr);

  bool start();
  void finish();
  bool nativeEventFilter(const QByteArray &type, void *message, qintptr *result) override;
private:
  void cancel();

  Index &mIndex;
  QFile *mOut;

  IdStorage mIds;
  git::RevWalk mWalker;
  LexerPool mLexers;

  // Variables to hold the threads
  Reduce mReduce;
  QThreadPool mGrabbers;
  QThreadPool mWorkers;
  ResultWriter mResultWriter;
  // Various worker queues to synchronise data between threads
  WorkerQueue<git::Commit> mCommits = WorkerQueue<git::Commit>(256);
  WorkerQueue<QPair<git::Commit, git::Diff>> mDiffedCommits =
      WorkerQueue<QPair<git::Commit, git::Diff>>(256);
  WorkerQueue<Intermediate> mIntermediateQueue = WorkerQueue<Intermediate>(256);
  // NOTE: The posting map is rather large, so it's limited to one item
  WorkerQueue<Index::PostingMap> mResults = WorkerQueue<Index::PostingMap>(1);
};

#endif // INDEXER
