//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TEST_H
#define TEST_H

#include "app/Application.h"
#include "git/Repository.h"
#include <iostream>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#define TEST_MAIN(TestClass)                                                   \
  int main(int argc, char *argv[]) {                                           \
    QTEST_SET_MAIN_SOURCE_PATH                                                 \
    return ::Test::runTest<TestClass>(argc, argv);                             \
  }

class RepoView;

namespace Test {

class ScratchRepository {
public:
  ScratchRepository(bool autoRemove = true);

  // Treat this class a wrapper.
  operator git::Repository();
  git::Repository *operator->();

private:
  QTemporaryDir mDir;
  git::Repository mRepo;
};

class Timeout : public QObject {
  // Q_OBJECT

public:
  Timeout(int milliseconds, QString message);
  ~Timeout();
  void dismiss();

private:
  QString message;
  QTimer timer;

  void onTimeout();
};

void refresh(RepoView *repoView, bool expectDirty = true);
void fetch(RepoView *repoView, git::Remote remote);
QString extractRepository(const QString &filename, bool useTempDir);
void initRepo(git::Repository &repo);

Application createApp(int &argc, char *argv[]);

template <typename T> int runTest(int argc, char *argv[]) {
  Application::setInTest();

  int orig_argc = argc;
  auto app = createApp(argc, argv);

  T testObj;

  // disable maxwarnings
  auto newSize = orig_argc + 2;
  auto new_argv = new char *[newSize];
  memcpy(new_argv, argv, sizeof(char *) * orig_argc);
  new_argv[orig_argc] = (char *)"-maxwarnings";
  new_argv[orig_argc + 1] = (char *)"0";
  return QTest::qExec(&testObj, newSize, new_argv);
}

} // namespace Test

#endif
