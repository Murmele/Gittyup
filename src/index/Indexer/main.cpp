//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "indexer.h"
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
#include <sys/resource.h>
#else
#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>
#endif

namespace {

const QString kLogFile = "log";

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
