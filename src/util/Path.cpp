#include "Path.h"
#include <optional>

#ifdef Q_OS_WIN
#include <memory>
#include <windows.h>
#endif

#if defined(FLATPAK) && defined(Q_OS_LINUX)
#include <QByteArray>
#include <QFile>

#include <cerrno>
#include <sys/types.h>
#include <sys/xattr.h>
#endif

namespace util {
QString canonicalizePath(QString path) {
#ifdef Q_OS_WIN
  // Convert from potential 8.3 paths to full paths on Windows
  {
    auto len = GetLongPathNameW((LPCWSTR)path.utf16(), nullptr, 0);

    // GetLongPathNameW() returns 0 if the given path doesn't exist (yet)
    if (len != 0) {
      std::unique_ptr<wchar_t[]> buf{new wchar_t[len]};
      len = GetLongPathNameW((LPCWSTR)path.utf16(), buf.get(), len);
      path = QString::fromWCharArray(buf.get(), len);
    }
  }
#endif
  return path;
}

#if defined(FLATPAK) && defined(Q_OS_LINUX)
namespace {

// The xdg-document-portal fuse filesystem tags every entry it exposes with the
// real location of that entry on the host in this extended attribute
std::optional<QString> readHostPathXattr(const QString &path) {
  const QByteArray name = QFile::encodeName(path);

  for (int size = 512; size <= 64 * 1024; size *= 8) {
    QByteArray value(size, Qt::Uninitialized);
    const ssize_t len =
        getxattr(name.constData(), "user.document-portal.host-path",
                 value.data(), static_cast<size_t>(value.size()));
    if (len >= 0) {
      value.truncate(static_cast<int>(len));
      // The attribute is stored without a trailing NUL, but just in case...
      while (value.endsWith('\0'))
        value.chop(1);
      return value.isEmpty() ? QString() : QFile::decodeName(value);
    }

    if (errno != ERANGE)
      return {};
  }

  return {};
}

QString resolvePortalPath(const QString &path) {
  // Fast path: the portal tags every existing entry directly.
  std::optional<QString> result = readHostPathXattr(path);
  if (result.has_value())
    return result.value();

  // We're not guaranteed that the path exists since the path might not resolve
  // in the checked out work directory. In those cases we need to find an
  // ancestor that does resolve, and patch the path
  QString ancestor = path;
  QString tail;
  while (true) {
    const int slash = ancestor.lastIndexOf(QLatin1Char('/'));
    if (slash <= 0)
      break;
    tail = ancestor.mid(slash) + tail;
    ancestor.truncate(slash);

    result = readHostPathXattr(ancestor);
    if (result.has_value())
      return result.value() + tail;
  }

  return path;
}

} // namespace
#endif // FLATPAK && Q_OS_LINUX

QString sandboxPathToHost(const QString &path) {
#if defined(FLATPAK) && defined(Q_OS_LINUX)
  return resolvePortalPath(path);
#else
  return path;
#endif
}
} // namespace util
