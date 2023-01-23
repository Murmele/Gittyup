//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ConfFile.h"
#include <QFileInfo>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#if defined(Q_OS_MAC)
#define PLATFORM "mac"
#elif defined(Q_OS_WIN)
#define PLATFORM "win"
#else
#define PLATFORM "x11"
#endif

namespace {

// Recursively decompose the table at the top of the stack.
QVariantMap table(lua_State *L) {
  QVariantMap map;

  lua_pushnil(L);
  while (lua_next(L, -2)) {
    // Convert the key.
    QString key;
    if (lua_isinteger(L, -2)) {
      key = QString::number(lua_tointeger(L, -2));
    } else if (lua_isstring(L, -2)) {
      key = lua_tostring(L, -2);
    }

    // Convert the value.
    QVariant val;
    if (lua_istable(L, -1)) {
      val = table(L);
    } else if (lua_isboolean(L, -1)) {
      val = lua_toboolean(L, -1) ? true : false;
    } else if (lua_isinteger(L, -1)) {
      val = lua_tointeger(L, -1);
    } else if (lua_isstring(L, -1)) {
      val = lua_tostring(L, -1);
    }

    // Add the pair to the map.
    if (!key.isEmpty() && val.isValid())
      map.insert(key, val);

    lua_pop(L, 1);
  }

  return map;
}

} // namespace

ConfFile::ConfFile(const QString &filename) : mFilename(filename) {}

ConfFile::~ConfFile() {}

QVariantMap ConfFile::parse(const QString &name) {
  // Verify the existence of the file.
  QFileInfo info(mFilename);
  QString canPath = info.canonicalPath();
  if (canPath.isEmpty())
    return QVariantMap();

  QByteArray tableName = name.toUtf8();
  QByteArray localPath = canPath.toLocal8Bit();
  QByteArray localName = mFilename.toLocal8Bit();

  // Create the lua state.
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  // Prepend this script's directory to the package path.
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");
  QByteArray path = lua_tostring(L, -1);
  lua_pop(L, 1);
  lua_pushstring(L, localPath + "/?.lua;" + path);
  lua_setfield(L, -2, "path");
  lua_pop(L, 1);

  // Set a global platform name.
  lua_pushstring(L, PLATFORM);
  lua_setglobal(L, "platform");

  // Create global table.
  if (!tableName.isEmpty()) {
    lua_newtable(L);
    lua_newtable(L);
    lua_setfield(L, -2, "property");
    lua_setglobal(L, tableName);
  }

  // Execute the configuration script.
  if (luaL_dofile(L, localName))
    lua_error(L);

  // Push global table.
  if (!tableName.isEmpty())
    lua_getglobal(L, tableName);

  // The script returned a single table.
  QVariantMap map = table(L);
  lua_close(L);

  return map;
}
