#include "indexer.h"
#include "common.h"
#include "index/Lexer.h"
#include "map.h"
#include "diffgrabber.h"
#include "../GenericLexer.h"

#include <QSocketNotifier>

#ifndef Q_OS_WIN
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#else
#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>

static LPTOP_LEVEL_EXCEPTION_FILTER defaultFilter = nullptr;

static LONG WINAPI exceptionFilter(PEXCEPTION_POINTERS info) {
  // Protect against reentering.
  static bool entered = false;
  if (entered)
    ExitProcess(1);
  entered = true;

  // Write dump file.
  SYSTEMTIME localTime;
  GetLocalTime(&localTime);

  wchar_t temp[MAX_PATH];
  GetTempPath(MAX_PATH, temp);

  wchar_t dir[MAX_PATH];
  const wchar_t *gittyup_name = L"%sGittyup";
  StringCchPrintf(dir, MAX_PATH, gittyup_name, temp);
  CreateDirectory(dir, NULL);

  wchar_t fileName[MAX_PATH];
  const wchar_t *s = L"%s\\%s-%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp";
  StringCchPrintf(fileName, MAX_PATH, s, dir, "indexer", GITTYUP_VERSION,
                  localTime.wYear, localTime.wMonth, localTime.wDay,
                  localTime.wHour, localTime.wMinute, localTime.wSecond,
                  GetCurrentProcessId(), GetCurrentThreadId());

  HANDLE dumpFile =
      CreateFile(fileName, GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

  MINIDUMP_EXCEPTION_INFORMATION expParam;
  expParam.ThreadId = GetCurrentThreadId();
  expParam.ExceptionPointers = info;
  expParam.ClientPointers = TRUE;

  MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile,
                    MiniDumpWithDataSegs, &expParam, NULL, NULL);

  return defaultFilter ? defaultFilter(info) : EXCEPTION_CONTINUE_SEARCH;
}
#endif

namespace {
    #ifdef Q_OS_UNIX
    // signal handler
    int fds[2];
    void term(int) {
      char ch = 1;
      write(fds[0], &ch, sizeof(ch));
    }
    #endif
}

void index(const Lexer::Lexeme &lexeme, Intermediate::FieldMap &fields,
           quint8 field, quint32 &pos) {
  QByteArray text = lexeme.text;
  switch (lexeme.token) {
    // Lex further.
    case Lexer::String:
      text.remove(0, 1);
      text.chop(1);
      // fall through

    case Lexer::Comment:
    case Lexer::Preprocessor:
    case Lexer::Constant:
    case Lexer::Variable:
    case Lexer::Function:
    case Lexer::Class:
    case Lexer::Type:
    case Lexer::Label: {
      switch (lexeme.token) {
        case Lexer::String:
          field |= Index::String;
          break;
        case Lexer::Comment:
          field |= Index::Comment;
          break;
        case Lexer::Nothing:      // fall through
        case Lexer::Whitespace:   // fall through
        case Lexer::Number:       // fall through
        case Lexer::Keyword:      // fall through
        case Lexer::Identifier:   // fall through
        case Lexer::Operator:     // fall through
        case Lexer::Error:        // fall through
        case Lexer::Preprocessor: // fall through
        case Lexer::Constant:     // fall through
        case Lexer::Variable:     // fall through
        case Lexer::Function:     // fall through
        case Lexer::Class:        // fall through
        case Lexer::Type:         // fall through
        case Lexer::Label:        // fall through
        case Lexer::Regex:        // fall through
        case Lexer::Embedded:     // fall through
          break;
      }

      GenericLexer sublexer;
      if (sublexer.lex(text)) {
        while (sublexer.hasNext())
          index(sublexer.next(), fields, field, pos);
      }
      break;
    }

    // Add directly.
    case Lexer::Keyword:
    case Lexer::Identifier:
      // Limit term length.
      if (text.length() <= 64) {
        if (field < Index::Any)
          field |= Index::Identifier;
        fields[field][text.toLower()].append(pos++);
      }
      break;

    // Ignore everything else.
    case Lexer::Nothing:    // fall through
    case Lexer::Whitespace: // fall through
    case Lexer::Number:     // fall through
    case Lexer::Operator:   // fall through
    case Lexer::Error:      // fall through
    case Lexer::Regex:      // fall through
    case Lexer::Embedded:   // fall through
      break;
  }
}

Indexer::Indexer(Index &index, QFile *out, bool notify, QObject *parent)
    : QObject(parent), mIndex(index), mOut(out), mIds(index),
      // mIntermediateQueue will be filled by the workers spawned in the start() method below.
      mReduce(mIds, out, mIntermediateQueue, mResults, mIndex), // Receives intermediate and converts them to results
      mResultWriter(out, index, mResults, notify) { // Receives results and writes them down to file
  mWalker = mIndex.repo().walker();

#ifdef Q_OS_UNIX
  if (!socketpair(AF_UNIX, SOCK_STREAM, 0, fds)) {
    // Create notifier.
    QSocketNotifier *notifier =
        new QSocketNotifier(fds[1], QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, [this, notifier] {
      notifier->setEnabled(false);
      char ch;
      read(fds[1], &ch, sizeof(ch));
      cancel();
      notifier->setEnabled(true);
    });

    // Install handler.
    struct sigaction sa;
    sa.sa_handler = &term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, 0);
  }
#endif
}

bool Indexer:: start() {
  // The distribution of threads is as follows
  // This thread iterates over mWalker
  // 1x thread runs the Reduction class
  // 1x thread runs the ResultWriter
  // Up-to 4 threads running a DiffGrabber. Even three seems to oversature
  // libgit2. On top of this, one worker is spawned per cpu thread
  // NOTE: This will always overcommit by up-to 7 threads. This is
  // intentional since the non-worker threads are lighter than the
  // rest of the threads. Additionally, for low-core cout machines, we need to
  // ensure there's at least one thread of each type. In some cases, the
  // processing done in the worker threads are the limiting factor. Aside from
  // using more RAM, the over-commitment doesn't seem to impact performance
  int numGabbers = std::min(4, std::max(1, QThread::idealThreadCount() / 2));
  int numWorkers = QThread::idealThreadCount();

  log(mOut, "start");
  mReduce.start();
  mResultWriter.start();
  for (int i = 0; i < numWorkers; i++) {
    mWorkers.start(new Map(mIndex.repo(), mLexers, mOut, mDiffedCommits,
                           mIntermediateQueue));
  }
  for (int i = 0; i < numGabbers; i++) {
    mGrabbers.start(new DiffGrabber(mCommits, mDiffedCommits));
  }

  git::Commit commit = mWalker.next();

  while (commit.isValid()) {
    // Don't index merge commits.
    if (!commit.isMerge() && !mIds.contains(commit.id())) {
      mCommits.enqueue(std::move(commit));
    }
    commit = mWalker.next();
  }

  finish();
  return false;
}

void Indexer::finish() {
  log(mOut, "finish");

  // NOTE: This requires a staged, sequenced shutdown, otherwise data is
  // ignored. The order is thus intentional and 'if (!canceled)' is necessary

  // Since this thread was the one feeding mCommits, we know we're
  //  done processing if it gets empty
  // If we're not aborting, wait for the queue to be empty, then stop/wake up
  // threads blocked on it, and lastly wait for the grabbers to finish up
  if (!canceled)
    mCommits.awaitEmpty();
  mCommits.stop();
  mGrabbers.waitForDone();

  // Now we can do the same thing with the next queue in the line, and stop
  // the worker threads
  if (!canceled)
    mDiffedCommits.awaitEmpty();
  mDiffedCommits.stop();
  mWorkers.waitForDone();

  // Next is the intermediate queue and the reduction thread
  if (!canceled)
    mIntermediateQueue.awaitEmpty();
  mIntermediateQueue.stop();
  mReduce.wait();

  // Before we finish up with the results queue and writer thread
  if (!canceled)
    mResults.awaitEmpty();
  mResults.stop();
  mResultWriter.wait();

  if (canceled) {
    QCoreApplication::exit(1);
  }
  QCoreApplication::quit();
}

bool Indexer::nativeEventFilter(const QByteArray &type, void *message,
                       qintptr *result) {
  Q_UNUSED(result);
#ifdef Q_OS_WIN
  MSG *msg = static_cast<MSG *>(message);
  if (msg->message == WM_CLOSE)
    cancel();
#else
  Q_UNUSED(type);
  Q_UNUSED(message);
#endif

  return false;
}

void Indexer::cancel() {
  canceled = true;
  finish();
}
