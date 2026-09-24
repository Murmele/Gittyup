//
//          Copyright (c) 2022, Gittyup authors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Kas
//

#ifndef UTIL_PATH_H
#define UTIL_PATH_H

#include <QString>

namespace util {
QString canonicalizePath(QString path);

/// @brief Convert a potentially sandboxed path into a host path. This can
/// happen for instances with Flatpak and paths outside home directory
/// @param path Potentially sandboxed path
/// @return Host path
QString sandboxPathToHost(const QString &path);
} // namespace util

#endif
