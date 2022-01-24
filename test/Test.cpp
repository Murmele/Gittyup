//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Test.h"
#include "ui/RepoView.h"

using namespace QTest;

namespace Test {

ScratchRepository::ScratchRepository(bool autoRemove)
{
  mDir.setAutoRemove(autoRemove);
  mRepo = git::Repository::init(mDir.path());
}

ScratchRepository::operator git::Repository()
{
  return mRepo;
}

git::Repository *ScratchRepository::operator->()
{
  return &mRepo;
}

void refresh(RepoView *view, bool expectDirty)
{
  // Setup post refresh trigger.
  bool finished = false;
  auto connection = QObject::connect(view, &RepoView::statusChanged,
  [&finished, expectDirty](bool dirty) {
    QCOMPARE(expectDirty, dirty);
    finished = true;
  });

  view->refresh();

  // Wait for the refresh to finish.
  while (!finished)
    qWait(100);

  QObject::disconnect(connection);

  // Select status index.
  if (expectDirty)
    view->selectFirstCommit();
}

void fetch(RepoView *view, git::Remote remote)
{
  QEventLoop loop;
  QFutureWatcher<git::Result> watcher;
  QObject::connect(&watcher, &QFutureWatcher<git::Result>::finished,
                   &loop, &QEventLoop::quit);

  watcher.setFuture(view->fetch(remote));
  loop.exec();

  QVERIFY(!watcher.result().error());
}

Application createApp(int &argc, char *argv[])
{
  auto new_argv = new char*[argc + 1];
  memcpy(new_argv, argv, sizeof(char*) * argc);

  // Make string comparisons with messages fail less
  new_argv[argc] = "--no-translation";

  argc += 1;
  return Application(argc, new_argv);
}

} // namespace Test
