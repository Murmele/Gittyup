//
//          Copyright (c) 2026, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
//

#include "Test.h"

#include "watcher/RepositoryWatcher.h"

#include <QSignalSpy>

using namespace Test;

namespace {

// Timeouts are generous so a starved CI runner can't cause false failures.
const int kAttemptMs = 4000;
const int kAttempts = 6;
const int kSignalMs = 15000;

// Longer than the watcher's 2s debounce, so late notifications land before the
// spy is cleared.
const int kSettleMs = 3000;

bool writeFile(const QString &path) {
  static int counter = 0;
  QFile file(path);
  if (!file.open(QFile::WriteOnly | QFile::Truncate))
    return false;

  file.write(QByteArray::number(++counter));
  return true;
}

void settle(QSignalSpy &spy) {
  QTest::qWait(kSettleMs);
  spy.clear();
}

// The watcher installs its watches on a thread, so retry a root-level change
// until one is reported; by then every existing directory is watched too.
bool waitUntilWatching(QSignalSpy &spy, const QDir &workdir) {
  // Give the thread a head start; the retries below cover a slow one.
  QTest::qWait(500);

  for (int i = 0; i < kAttempts; ++i) {
    if (!writeFile(workdir.filePath("canary")))
      return false;

    if (spy.wait(kAttemptMs)) {
      settle(spy);
      return true;
    }
  }

  return false;
}

} // namespace

class TestRepositoryWatcher : public QObject {
  Q_OBJECT

private slots:
  void existingDirectory_data();
  void existingDirectory();
  void newHiddenDirectory();
};

void TestRepositoryWatcher::existingDirectory_data() {
  QTest::addColumn<QString>("dir");

  QTest::newRow("root") << "";
  QTest::newRow("visible") << "src";
  QTest::newRow("hidden") << ".github";
  QTest::newRow("under hidden") << ".github/workflows";
  QTest::newRow("hidden under visible") << "src/.cache";
}

void TestRepositoryWatcher::existingDirectory() {
  QFETCH(QString, dir);

  ScratchRepository repo;
  QDir workdir = repo->workdir();
  if (!dir.isEmpty())
    QVERIFY(workdir.mkpath(dir));

  RepositoryWatcher watcher(repo);
  QSignalSpy spy(repo->notifier(), &git::RepositoryNotifier::workdirChanged);
  QVERIFY2(waitUntilWatching(spy, workdir),
           "watcher never reported a root-level change");

  QVERIFY(writeFile(QDir(workdir.filePath(dir)).filePath("file")));
  QVERIFY2(
      spy.wait(kSignalMs),
      qPrintable(QString("no notification for a change in '%1'").arg(dir)));
}

void TestRepositoryWatcher::newHiddenDirectory() {
  ScratchRepository repo;
  QDir workdir = repo->workdir();

  RepositoryWatcher watcher(repo);
  QSignalSpy spy(repo->notifier(), &git::RepositoryNotifier::workdirChanged);
  QVERIFY2(waitUntilWatching(spy, workdir),
           "watcher never reported a root-level change");

  QVERIFY(workdir.mkdir(".late"));
  QVERIFY2(spy.wait(kSignalMs), "no notification for the new directory");
  settle(spy);

  QVERIFY(writeFile(workdir.filePath(".late/file")));
  QVERIFY2(spy.wait(kSignalMs), "no notification for a change in '.late'");
}

TEST_MAIN(TestRepositoryWatcher)
#include "RepositoryWatcher.moc"
