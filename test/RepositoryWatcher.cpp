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
#include <filesystem>
#include <memory>

using namespace Test;

namespace {

const int kDebounceMs = 100;

// Late notifications from the previous change must land before the spy is
// cleared, so this has to comfortably exceed the debounce.
const int kSettleMs = 250;

// Timeouts are generous so a starved CI runner can't cause false failures.
const int kAttemptMs = 1000;
const int kAttempts = 15;
const int kSignalMs = 15000;

struct Row {
  const char *name;
  const char *dir;
};

// Directories that exist before the watcher starts.
const Row kRows[] = {
    {"root", ""},
    {"visible", "src"},
    {"hidden", ".github"},
    {"under hidden", ".github/workflows"},
    {"hidden under visible", "src/.cache"},
};

bool writeFile(const QString &path, const QByteArray &data = QByteArray()) {
  static int counter = 0;
  QFile file(path);
  if (!file.open(QFile::WriteOnly | QFile::Truncate))
    return false;

  file.write(data.isEmpty() ? QByteArray::number(++counter) : data);
  return true;
}

// Overwrites the target the way an atomic save does.
bool replaceFile(const QString &from, const QString &to) {
  std::error_code ec;
  std::filesystem::rename(from.toStdString(), to.toStdString(), ec);
  return !ec;
}

void settle(QSignalSpy &spy) {
  QTest::qWait(kSettleMs);
  spy.clear();
}

// The watcher installs its watches on a thread, so retry a root-level change
// until one is reported; by then every existing directory is watched too.
bool waitUntilWatching(QSignalSpy &spy, const QDir &workdir) {
  // Give the thread a head start; the retries below cover a slow one.
  QTest::qWait(kDebounceMs);

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
  void initTestCase();
  void existingDirectory_data();
  void existingDirectory();
  void newHiddenDirectory();
  void atomicReplace_data();
  void atomicReplace();

private:
  // Order matters: the watcher must be destroyed before the repository.
  std::unique_ptr<ScratchRepository> mRepo;
  std::unique_ptr<RepositoryWatcher> mWatcher;
  std::unique_ptr<QSignalSpy> mSpy;
  QDir mWorkdir;
  QTemporaryDir mOutside;
};

void TestRepositoryWatcher::initTestCase() {
  mRepo = std::make_unique<ScratchRepository>();
  mWorkdir = (*mRepo)->workdir();
  for (const Row &row : kRows) {
    if (*row.dir)
      QVERIFY(mWorkdir.mkpath(row.dir));
  }

  // The rename target for atomicReplace(), and a rule that ignores temp files.
  QVERIFY(writeFile(mWorkdir.filePath(".gitignore"), "*.tmp\n"));
  QVERIFY(writeFile(mWorkdir.filePath("atomic")));

  mWatcher.reset(RepositoryWatcher::create(*mRepo));
  mWatcher->setDebounceInterval(kDebounceMs);
  mSpy = std::make_unique<QSignalSpy>((*mRepo)->notifier(),
                                      &git::RepositoryNotifier::workdirChanged);
  QVERIFY2(waitUntilWatching(*mSpy, mWorkdir),
           "watcher never reported a root-level change");
}

void TestRepositoryWatcher::existingDirectory_data() {
  QTest::addColumn<QString>("dir");

  for (const Row &row : kRows)
    QTest::newRow(row.name) << QString(row.dir);
}

void TestRepositoryWatcher::existingDirectory() {
  QFETCH(QString, dir);

  QVERIFY(writeFile(QDir(mWorkdir.filePath(dir)).filePath("file")));
  QVERIFY2(
      mSpy->wait(kSignalMs),
      qPrintable(QString("no notification for a change in '%1'").arg(dir)));
  settle(*mSpy);
}

void TestRepositoryWatcher::newHiddenDirectory() {
  QVERIFY(mWorkdir.mkdir(".late"));
  QVERIFY2(mSpy->wait(kSignalMs), "no notification for the new directory");
  settle(*mSpy);

  QVERIFY(writeFile(mWorkdir.filePath(".late/file")));
  QVERIFY2(mSpy->wait(kSignalMs), "no notification for a change in '.late'");
}

void TestRepositoryWatcher::atomicReplace_data() {
  QTest::addColumn<QString>("from");

  QTest::newRow("temp outside repository") << mOutside.filePath("atomic.tmp");
  QTest::newRow("gitignored temp") << mWorkdir.filePath("atomic.tmp");
  QTest::newRow("plain rename") << mWorkdir.filePath("atomic.new");
}

void TestRepositoryWatcher::atomicReplace() {
  QFETCH(QString, from);

  QVERIFY(writeFile(from));
  settle(*mSpy);

  QVERIFY(replaceFile(from, mWorkdir.filePath("atomic")));
  QVERIFY2(mSpy->wait(kSignalMs),
           "no notification for a rename onto an existing file");
  settle(*mSpy);
}

TEST_MAIN(TestRepositoryWatcher)
#include "RepositoryWatcher.moc"
