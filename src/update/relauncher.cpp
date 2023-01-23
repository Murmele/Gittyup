//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include <QCoreApplication>
#include <QProcess>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <signal.h>
#endif

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  QStringList args = app.arguments();
  args.removeFirst();
  if (args.size() < 2)
    return 1;

  int pid = args.last().toInt();

#ifdef Q_OS_WIN
  // Wait for the process to die.
  if (HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid)) {
    DWORD ret = WaitForSingleObject(process, 5000);
    CloseHandle(process);
    if (ret == WAIT_TIMEOUT)
      return 1;
  }
#else
  // Poll the process until it dies.
  int tries = 0;
  while (!kill(pid, 0)) {
    if (++tries > 10)
      return 1;
    QThread::msleep(500);
  }
#endif

  // Restart from the path argument.
  QProcess::startDetached(args.first());

  return 0;
}
