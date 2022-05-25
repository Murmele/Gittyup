//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "Test.h"
#include "ui/RepoView.h"
//#include <JlCompress.h>
#include <exception>
#include <QFileInfo>
#include "zip.h"
using namespace QTest;

namespace Test {

QTemporaryDir tempDir;

QString zipReturnValueToString(int value) {
  switch (value) {
    case 0:
      return "OK";
    case -1:
      return "enoinit";
    case -2:
      return "einventname";
    case -3:
      return "enoent";
    case -4:
      return "einvmode";
    case -5:
      return "einvlvl";
    case -6:
      return "enosup64";
    case -7:
      return "ememset";
    case -8:
      return "ewrtent";
    case -9:
      return "etdeflinit";
    case -10:
      return "einvidx";
    case -11:
      return "enohdr";
    case -12:
      return "etdeflbuf";
    case -13:
      return "ecrthdr";
    case -14:
      return "ewrthdr";
    case -15:
      return "ewrtdir";
    case -16:
      return "eopnfile";
    case -17:
      return "einventtype";
    case -18:
      return "ememnoalloc";
    case -19:
      return "enofile";
    case -20:
      return "enoperm";
    case -21:
      return "eoomem";
    case -22:
      return "einvzipname";
    case -23:
      return "emkdir";
    case -24:
      return "esymlink";
    case -25:
      return "eclszip";
    case -26:
      return "ecapsize";
    case -27:
      return "efseek";
    case -28:
      return "efread";
    case -29:
      return "efwrite";
    default:
      return "Unknown";
  }
}

int on_extract_entry(const char *filename, void *arg) {
  static int i = 0;
  int n = *(int *)arg;
  printf("Extracted: %s (%d of %d)\n", filename, ++i, n);

  return 0;
}

/*!
 * \brief extractRepository
 * Hacky method to extract a repository
 * \param filename
 * \return
 */
QString extractRepository(const QString &filename, bool useTempDir) {

  QDir repoPath(TESTREPOSITORIES_PATH);
  QFileInfo f(repoPath.filePath(filename));
  QByteArray exportPath;
  if (useTempDir)
    exportPath = tempDir.path().toLatin1();
  else
    exportPath = repoPath.path().toLatin1();
  QString exportFolder = QDir(exportPath).filePath(f.baseName());

  if (!QDir(exportFolder).exists() && !f.exists()) {
    qDebug() << "Zip file does not exist: " << f;
    return "";
  }

  if (useTempDir && !tempDir.isValid()) {
    qDebug() << "Not able to create temporary directory.";
    return "";
  }

  auto folder = QDir(exportFolder);

  if (folder.exists()) {
    // Delete folder, because if the testrepo is used by multiple tests
    // there should be a clean environment
    folder.removeRecursively();
  }
  const char *path_c = exportPath.data();
  // Copy data because for some reason the pointer to the char array might change otherwise
  QByteArray ba_filename = f.absoluteFilePath().toLatin1();
  const char *filename_c = ba_filename.data();


  int arg = 2;
  auto res = zip_extract(filename_c, path_c, on_extract_entry, &arg);
  if (res < 0) {
    qDebug() << "Error opening zip file: " << zipReturnValueToString(res);
    return "";
  }
  return exportFolder; // successfully extracted
}

ScratchRepository::ScratchRepository(bool autoRemove) {
  mDir.setAutoRemove(autoRemove);
  mRepo = git::Repository::init(mDir.path());
}

ScratchRepository::operator git::Repository() { return mRepo; }

git::Repository *ScratchRepository::operator->() { return &mRepo; }

Timeout::Timeout(int milliseconds, QString message) {
  timer.callOnTimeout(this, &Timeout::onTimeout, Qt::DirectConnection);
  this->message = message;

  timer.setSingleShot(true);
  timer.setInterval(milliseconds);
  timer.start();
}

Timeout::~Timeout() { timer.stop(); }

void Timeout::dismiss() { timer.stop(); }

void Timeout::onTimeout() {
  std::cerr << "Hard timeout: " << message.toStdString() << std::endl;
  abort();
}

void refresh(RepoView *view, bool expectDirty) {
  // Setup post refresh trigger.
  bool finished = false;
  auto connection = QObject::connect(view, &RepoView::statusChanged,
                                     [&finished, expectDirty](bool dirty) {
                                       QCOMPARE(dirty, expectDirty);
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

void fetch(RepoView *view, git::Remote remote) {
  QEventLoop loop;
  QFutureWatcher<git::Result> watcher;
  QObject::connect(&watcher, &QFutureWatcher<git::Result>::finished, &loop,
                   &QEventLoop::quit);

  watcher.setFuture(view->fetch(remote));
  loop.exec();

  QVERIFY(!watcher.result().error());
}

Application createApp(int &argc, char *argv[]) {
#define OFFSCREEN 1

#if OFFSCREEN == 1
  auto new_argv = new char *[argc + 1 + 2];
#else
  auto new_argv = new char *[argc + 1];
#endif
  memcpy(new_argv, argv, sizeof(char *) * argc);

  // Make string comparisons with messages fail less
  new_argv[argc] = (char *)"--no-translation";

#if OFFSCREEN == 1
  new_argv[argc + 1] = (char *)"--platform";
  new_argv[argc + 2] = (char *)"offscreen";
  argc += 2;
#endif

  argc += 1;
  return Application(argc, new_argv);
}

} // namespace Test
