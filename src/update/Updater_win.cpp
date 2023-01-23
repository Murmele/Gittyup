#include "Updater.h"
#include <QApplication>
#include <QDir>
#include <QTemporaryFile>
#include <windows.h>

bool Updater::install(const DownloadRef &download, QString &error) {
  // Destroy the temporary file object to release the file handle.
  QString file = download->file()->fileName();
  download->file()->setAutoRemove(false);
  delete download->file();
  download->setFile(nullptr);

  // Derive install destination from previous executable location.
  QString path = QApplication::applicationDirPath();
  QString arg = QString("/D=%1").arg(QDir::toNativeSeparators(path));
  const wchar_t *argPtr = reinterpret_cast<const wchar_t *>(arg.utf16());
  const wchar_t *filePtr = reinterpret_cast<const wchar_t *>(file.utf16());

  // Start the installer with ShellExecute instead of CreateProcess.
  HINSTANCE result = ShellExecuteW(0, 0, filePtr, argPtr, 0, SW_SHOWNORMAL);
  if (reinterpret_cast<quintptr>(result) <= 32) {
    error = tr("Installer failed to start");
    return false;
  }

  return true;
}
