//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Test.h"
#include "conf/Settings.h"
#include "git/Config.h"

using namespace QTest;

class TestConfig : public QObject {
  Q_OBJECT

private slots:
  void mergetools();
};

void TestConfig::mergetools() {
  git::Config config = git::Config::global();

  // Add mergetools config file for default external merge tools.
  QString path = Settings::confDir().filePath("mergetools");
  config.addFile(QDir::toNativeSeparators(path), GIT_CONFIG_LEVEL_PROGRAMDATA);

  // Iterate over difftool cmd entries.
  QSet<QString> names;
  git::Config::Iterator it = config.glob("difftool\\..*\\.cmd");
  while (git::Config::Entry entry = it.next()) {
    names.insert(entry.name());
    qDebug() << entry.name() << entry.value<QString>();
  }

  QVERIFY(names.contains("difftool.araxis.cmd"));
}

TEST_MAIN(TestConfig)

#include "config.moc"
