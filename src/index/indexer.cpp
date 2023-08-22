//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Index.h"
#include "GenericLexer.h"
#include "LPegLexer.h"
#include "qtsupport.h"
#include "conf/Settings.h"
#include "git/Config.h"
#include "git/Index.h"
#include "git/Patch.h"
#include "git/Repository.h"
#include "git/RevWalk.h"
#include "git/Signature.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QFutureWatcher>
#include <QLockFile>
#include <QMap>
#include <QRegularExpression>
#include <QTextStream>
#include <QtConcurrent>

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

  char temp[MAX_PATH];
  GetTempPath(MAX_PATH, temp);

  char dir[MAX_PATH];
  StringCchPrintf(dir, MAX_PATH, "%sGittyup", temp);
  CreateDirectory(dir, NULL);

  char fileName[MAX_PATH];
  StringCchPrintf(
      fileName, MAX_PATH, "%s\\%s-%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
      dir, "indexer", GITTYUP_VERSION, localTime.wYear, localTime.wMonth,
      localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond,
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

const QString kLogFile = "log";

const QRegularExpression kWsRe("\\s+");

// global cancel flag
bool canceled = false;

#ifdef Q_OS_UNIX
// signal handler
int fds[2];
void term(int) {
  char ch = 1;
  write(fds[0], &ch, sizeof(ch));
}
#endif

void log(QFile *out, const QString &text) {
  if (!out)
    return;

  QString time = QTime::currentTime().toString(Qt::ISODateWithMs);
  QTextStream(out) << time << " - " << text << Qt::endl;
}

void log(QFile *out, const QString &fmt, const git::Id &id) {
  if (!out)
    return;

  log(out, fmt.arg(id.toString()));
}

struct Intermediate {
  using TermMap = QHash<QByteArray, QVector<quint32>>;
  using FieldMap = QMap<quint8, TermMap>;

  git::Id id;
  FieldMap fields;
};

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
        default:
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
    default:
      break;
  }
}

class LexerPool {
public:
  LexerPool() : mHome(Settings::lexerDir().path().toUtf8()) {}

  ~LexerPool() { qDeleteAll(mLexers); }

  Lexer *acquire(const QByteArray &name) {
    QMutexLocker locker(&mMutex);
    (void)locker;

    return mLexers.contains(name) ? mLexers.take(name)
                                  : new LPegLexer(mHome, name);
  }

  void release(Lexer *lexer) {
    QMutexLocker locker(&mMutex);
    (void)locker;

    mLexers.insert(lexer->name(), lexer);
  }

private:
  QByteArray mHome;
  QMutex mMutex;
  QMultiMap<QByteArray, Lexer *> mLexers;
};

class Map {
public:
  typedef Intermediate result_type;

  Map(const git::Repository &repo, LexerPool &lexers, QFile *out)
      : mLexers(lexers), mOut(out) {
    git::Config config = repo.appConfig();
    mTermLimit = config.value<int>("index.termlimit", mTermLimit);
    mContextLines = config.value<int>("index.contextlines", mContextLines);
  }

  Intermediate operator()(const git::Commit &commit) {
    log(mOut, "map: %1", commit.id());

    quint32 filePos = 0;
    quint32 hunkPos = 0;

    Intermediate result;
    result.id = commit.id();

    // Index id.
    result.fields[Index::Id][commit.id().toString().toUtf8()].append(0);

    // Index committer date.
    QDateTime time = commit.committer().date();
    QByteArray date = time.date().toString(Index::dateFormat()).toUtf8();
    result.fields[Index::Date][date].append(0);

    // Index author name and email.
    git::Signature author = commit.author();
    QByteArray email = author.email().toUtf8().toLower();
    result.fields[Index::Email][email].append(0);

    quint32 namePos = 0;
    foreach (const QString &name, author.name().split(kWsRe)) {
      QByteArray key = name.toUtf8().toLower();
      result.fields[Index::Author][key].append(namePos++);
    }

    // Index message.
    GenericLexer generic;
    quint32 messagePos = 0;
    generic.lex(commit.message().toUtf8());
    while (generic.hasNext())
      index(generic.next(), result.fields, Index::Message, messagePos);

    // Index diff.
    quint32 diffPos = 0;
    git::Diff diff = commit.diff(git::Commit(), mContextLines, true);
    int patches = diff.count();
    for (int pidx = 0; pidx < patches; ++pidx) {
      // Truncate commits after term limit.
      if (canceled || diffPos > mTermLimit)
        break;

      // Skip binary deltas.
      if (diff.isBinary(pidx))
        continue;

      // Generate patch.
      git::Patch patch = diff.patch(pidx);
      if (!patch.isValid())
        continue;

      // Index file name and path.
      QFileInfo info(patch.name().toLower());
      result.fields[Index::Path][info.filePath().toUtf8()].append(filePos);
      result.fields[Index::File][info.fileName().toUtf8()].append(filePos++);

      // Look up lexer.
      QByteArray name = Settings::instance()->lexer(patch.name()).toUtf8();
      Lexer *lexer = (name == "null") ? &generic : mLexers.acquire(name);

      // Lex one line at a time.
      int hunks = patch.count();
      for (int hidx = 0; hidx < hunks; ++hidx) {
        if (canceled || diffPos > mTermLimit)
          break;

        // Index hunk header.
        QByteArray header = patch.header(hidx);
        if (lexer->lex(header)) {
          while (lexer->hasNext())
            index(lexer->next(), result.fields, Index::Scope, hunkPos);
        }

        // Index content.
        int lines = patch.lineCount(hidx);
        for (int line = 0; line < lines; ++line) {
          if (canceled || diffPos > mTermLimit)
            break;

          Index::Field field;
          switch (patch.lineOrigin(hidx, line)) {
            case GIT_DIFF_LINE_CONTEXT:
              field = Index::Context;
              break;
            case GIT_DIFF_LINE_ADDITION:
              field = Index::Addition;
              break;
            case GIT_DIFF_LINE_DELETION:
              field = Index::Deletion;
              break;
            default:
              continue;
          }

          if (lexer->lex(patch.lineContent(hidx, line))) {
            while (!canceled && lexer->hasNext())
              index(lexer->next(), result.fields, field, diffPos);
          }
        }
      }

      // Return lexer to the pool.
      if (lexer != &generic)
        mLexers.release(lexer);
    }

    return result;
  }

private:
  LexerPool &mLexers;
  QFile *mOut;

  int mContextLines = 3;
  int mTermLimit = 1000000;
};

class Reduce {
public:
  Reduce(Index::IdList &ids, QFile *out) : mIds(ids), mOut(out) {}

  void operator()(Index::PostingMap &result, const Intermediate &intermediate) {
    if (canceled || intermediate.fields.isEmpty())
      return;

    log(mOut, "reduce: %1", intermediate.id);

    quint32 id = mIds.size();
    mIds.append(intermediate.id);

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
  }

private:
  Index::IdList &mIds;
  QFile *mOut;
};

class Indexer : public QObject, public QAbstractNativeEventFilter {
public:
  Indexer(Index &index, QFile *out, bool notify, QObject *parent = nullptr)
      : QObject(parent), mIndex(index), mOut(out), mNotify(notify) {
    mWalker = mIndex.repo().walker();
    connect(&mWatcher, &QFutureWatcher<Index::PostingMap>::finished, this,
            &Indexer::finish);

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

  bool start() {
    log(mOut, "start");

    // Get list of commits.
    int count = 0;
    QList<git::Commit> commits;
    git::Commit commit = mWalker.next();

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QSet<git::Id> ids(mIndex.ids().begin(), mIndex.ids().end());
#else
    QSet<git::Id> ids = QSet<git::Id>::fromList(mIndex.ids());
#endif
    while (commit.isValid() && count < 8192) {
      // Don't index merge commits.
      if (!commit.isMerge() && !ids.contains(commit.id())) {
        commits.append(commit);
        ++count;
      }

      commit = mWalker.next();
    }

    if (commits.isEmpty()) {
      log(mOut, "nothing to index");
      QCoreApplication::quit();
      return false;
    }

    // Start map-reduce.
    using CommitList = QList<git::Commit>;
    mWatcher.setFuture(
        QtConcurrent::mappedReduced<Index::PostingMap, CommitList, Map, Reduce>(
            commits, Map(mIndex.repo(), mLexers, mOut),
            Reduce(mIndex.ids(), mOut)));
    return true;
  }

  void finish() {
    log(mOut, "finish");

    if (canceled) {
      QCoreApplication::exit(1);
    } else {
      // Write to disk.
      log(mOut, "start write");
      if (mIndex.write(mWatcher.result()) && mNotify)
        QTextStream(stdout) << "write" << Qt::endl;
      log(mOut, "end write");

      // Restart.
      start();
    }
  }

  bool nativeEventFilter(const QByteArray &type, void *message,
                         long *result) override {
#ifdef Q_OS_WIN
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_CLOSE)
      cancel();
#endif

    return false;
  }

private:
  void cancel() {
    canceled = true;
    mWatcher.cancel();
    mWatcher.waitForFinished();
  }

  Index &mIndex;
  QFile *mOut;
  bool mNotify;

  git::RevWalk mWalker;
  LexerPool mLexers;
  QFutureWatcher<Index::PostingMap> mWatcher;
};

class RepoInit {
public:
  RepoInit() {
    git::Repository::init();

    // Initialize settings on this thread.
    (void)Settings::instance();
  }

  ~RepoInit() { git::Repository::shutdown(); }
};

} // namespace

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
  // Install exception filter.
  defaultFilter = SetUnhandledExceptionFilter(&exceptionFilter);
#endif

  QCoreApplication app(argc, argv);

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addPositionalArgument("repo", "path to repository", "repo");
  parser.addOption({{"l", "log"}, "Write indexer progress to log."});
  parser.addOption({{"v", "verbose"}, "Print indexer progress to stdout."});
  parser.addOption({{"n", "notify"}, "Notify when data is written to disk."});
  parser.addOption({{"b", "background"}, "Start with background priority."});
  parser.process(app);

  QStringList args = parser.positionalArguments();
  if (args.isEmpty())
    parser.showHelp(1);

  // Initialize global git state.
  RepoInit init;
  (void)init;

  git::Repository repo = git::Repository::open(args.first());
  if (!repo.isValid())
    parser.showHelp(1);

  // Set empty index to prevent going to the index on disk.
  repo.setIndex(git::Index::create());

  // Set output file.
  QFile *out = nullptr;
  if (parser.isSet("log")) {
    out = new QFile(Index::indexDir(repo).filePath(kLogFile), &app);
    if (!out->open(QIODevice::WriteOnly | QIODevice::Append)) {
      delete out;
      out = nullptr;
    }
  } else if (parser.isSet("verbose")) {
    out = new QFile(&app);
    if (!out->open(stdout, QIODevice::WriteOnly | QIODevice::Append)) {
      delete out;
      out = nullptr;
    }
  }

  // Set priority.
  if (parser.isSet("background")) {
#ifdef Q_OS_WIN
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#else
    setpriority(PRIO_PROCESS, 0, 15);
#endif
  }

  // Try to lock the index for writing.
  QLockFile lock(Index::lockFile(repo));
  lock.setStaleLockTime(Index::staleLockTime());
  if (!lock.tryLock())
    return 0;

  // Start the indexer.
  Index index(repo);
  Indexer indexer(index, out, parser.isSet("notify"));
  app.installNativeEventFilter(&indexer);
  return indexer.start() ? app.exec() : 0;
}
