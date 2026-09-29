//
//          Copyright (c) 2026, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
//

#include "Test.h"

#include "watcher/PathFilter.h"

#include <memory>

using namespace Test;

namespace {

bool writeFile(const QDir &workdir, const QString &path,
               const QByteArray &data = "x") {
  QFile file(workdir.filePath(path));
  return file.open(QFile::WriteOnly | QFile::Truncate) &&
         file.write(data) == data.size();
}

} // namespace

class TestPathFilter : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void isRelevant_data();
  void isRelevant();
  void noticesLaterTracking();

private:
  ScratchRepository mRepo;
  QDir mWorkdir;
  std::unique_ptr<PathFilter> mFilter;
};

void TestPathFilter::initTestCase() {
  mWorkdir = mRepo->workdir();
  QVERIFY(mWorkdir.mkpath("sub/dir"));

  QVERIFY(writeFile(mWorkdir, ".gitignore", "*.ign\n"));
  for (const char *name : {"plain.txt", "tracked.ign", "untracked.ign",
                           "sub/dir/tracked.ign", "sub/dir/untracked.ign"})
    QVERIFY(writeFile(mWorkdir, name));

  QVERIFY(forceAdd(mWorkdir.path(), "tracked.ign", "x"));
  QVERIFY(forceAdd(mWorkdir.path(), "sub/dir/tracked.ign", "x"));

  mFilter = std::make_unique<PathFilter>(mRepo);
}

void TestPathFilter::isRelevant_data() {
  QTest::addColumn<QString>("path");
  QTest::addColumn<bool>("relevant");

  QTest::newRow("not ignored") << "plain.txt" << true;
  QTest::newRow("ignored, untracked") << "untracked.ign" << false;
  QTest::newRow("ignored, tracked") << "tracked.ign" << true;
  QTest::newRow("ignored, untracked, in subdirectory")
      << "sub/dir/untracked.ign" << false;
  QTest::newRow("ignored, tracked, in subdirectory")
      << "sub/dir/tracked.ign" << true;
  QTest::newRow("ignored, missing") << "gone.ign" << false;
  QTest::newRow("git directory") << ".git" << false;
  QTest::newRow("absolute, not ignored")
      << mWorkdir.filePath("plain.txt") << true;
  QTest::newRow("absolute, ignored, untracked")
      << mWorkdir.filePath("untracked.ign") << false;
  QTest::newRow("absolute, ignored, tracked")
      << mWorkdir.filePath("tracked.ign") << true;
}

void TestPathFilter::isRelevant() {
  QFETCH(QString, path);
  QFETCH(bool, relevant);

  QCOMPARE(mFilter->isRelevant(path), relevant);
}

void TestPathFilter::noticesLaterTracking() {
  QVERIFY(writeFile(mWorkdir, "later.ign"));
  QVERIFY(!mFilter->isRelevant("later.ign"));

  QVERIFY(forceAdd(mWorkdir.path(), "later.ign", "x"));
  QVERIFY(mFilter->isRelevant("later.ign"));
}

TEST_MAIN(TestPathFilter)
#include "PathFilter.moc"
