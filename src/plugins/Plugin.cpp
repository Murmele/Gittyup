//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Plugin.h"
#include "qtsupport.h"
#include "conf/Settings.h"
#include "editor/TextEditor.h"
#include "git/Config.h"
#include <QCoreApplication>
#include <QTextStream>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace {

const QString kKeyFmt = "plugins.%1.%2";
const QString kSubkeyFmt = "plugins.%1.%2.%3";

int optionsScriptDir(lua_State *L);
int optionsDefineBoolean(lua_State *L);
int optionsDefineInteger(lua_State *L);
int optionsDefineString(lua_State *L);
int optionsDefineList(lua_State *L);
int optionsValue(lua_State *L);

int kindsDefineNote(lua_State *L);
int kindsDefineWarning(lua_State *L);
int kindsDefineError(lua_State *L);

int hunkLines(lua_State *L);
int hunkLexer(lua_State *L);
int hunkTabWidth(lua_State *L);

int lineText(lua_State *L);
int lineOrigin(lua_State *L);
int lineLexemes(lua_State *L);
int lineAddError(lua_State *L);
int lineColumn(lua_State *L);
int lineColumnPos(lua_State *L);

int lexemeEq(lua_State *L);
int lexemePos(lua_State *L);
int lexemeText(lua_State *L);
int lexemeKind(lua_State *L);
int lexemeIsKind(lua_State *L);

const luaL_Reg kOptionsFuncs[] = {{"script_dir", &optionsScriptDir},
                                  {"define_boolean", &optionsDefineBoolean},
                                  {"define_integer", &optionsDefineInteger},
                                  {"define_string", &optionsDefineString},
                                  {"define_list", &optionsDefineList},
                                  {"value", &optionsValue},
                                  {nullptr, nullptr}};

const luaL_Reg kKindsFuncs[] = {{"define_note", &kindsDefineNote},
                                {"define_warning", &kindsDefineWarning},
                                {"define_error", &kindsDefineError},
                                {nullptr, nullptr}};

const luaL_Reg kHunkFuncs[] = {{"lines", &hunkLines},
                               {"lexer", &hunkLexer},
                               {"tab_width", &hunkTabWidth},
                               {nullptr, nullptr}};

const luaL_Reg kLineFuncs[] = {
    {"text", &lineText},       {"origin", &lineOrigin},
    {"lexemes", &lineLexemes}, {"add_error", &lineAddError},
    {"column", &lineColumn},   {"column_pos", &lineColumnPos},
    {nullptr, nullptr}};

const luaL_Reg kLexemeFuncs[] = {
    {"__eq", &lexemeEq},   {"pos", &lexemePos},        {"text", &lexemeText},
    {"kind", &lexemeKind}, {"is_kind", &lexemeIsKind}, {nullptr, nullptr}};

Plugin *plugin(lua_State *L) {
  return static_cast<Plugin *>(lua_touserdata(L, lua_upvalueindex(1)));
}

template <typename T> T member(lua_State *L, const char *member) {
  lua_getfield(L, 1, member);
  void *result = lua_touserdata(L, -1);
  lua_pop(L, 1); // member
  return static_cast<T>(result);
}

template <> int member<int>(lua_State *L, const char *member) {
  lua_getfield(L, 1, member);
  int result = lua_tointeger(L, -1);
  lua_pop(L, 1); // member
  return result;
}

template <typename T>
void setMember(lua_State *L, const char *member, T value) {
  lua_pushlightuserdata(L, value);
  lua_setfield(L, -2, member);
}

template <> void setMember(lua_State *L, const char *member, int value) {
  lua_pushinteger(L, value);
  lua_setfield(L, -2, member);
}

template <>
void setMember(lua_State *L, const char *member, const char *value) {
  lua_pushstring(L, value);
  lua_setfield(L, -2, member);
}

void createInstance(lua_State *L, Plugin *plugin, const char *name,
                    const luaL_Reg functions[]) {
  lua_newtable(L);
  if (luaL_newmetatable(L, name)) {
    lua_pushvalue(L, -1); // metatable
    lua_setfield(L, -2, "__index");
    lua_pushlightuserdata(L, plugin);
    luaL_setfuncs(L, functions, 1);
  }

  lua_setmetatable(L, -2);
}

int optionsScriptDir(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  lua_pushstring(L, plugin(L)->scriptDir().toUtf8());
  return 1;
}

int optionsDefineBoolean(lua_State *L) {
  if (lua_gettop(L) != 4 || !lua_istable(L, 1) || !lua_isstring(L, 2) ||
      !lua_isstring(L, 3) || !lua_isboolean(L, 4))
    luaL_error(L, "invalid arguments");

  plugin(L)->defineOption(lua_tostring(L, 2), Plugin::Boolean,
                          lua_tostring(L, 3), lua_toboolean(L, 4));

  return 0;
}

int optionsDefineInteger(lua_State *L) {
  if (lua_gettop(L) != 4 || !lua_istable(L, 1) || !lua_isstring(L, 2) ||
      !lua_isstring(L, 3) || !lua_isinteger(L, 4))
    luaL_error(L, "invalid arguments");

  plugin(L)->defineOption(lua_tostring(L, 2), Plugin::Integer,
                          lua_tostring(L, 3), lua_tointeger(L, 4));

  return 0;
}

int optionsDefineString(lua_State *L) {
  if (lua_gettop(L) != 4 || !lua_istable(L, 1) || !lua_isstring(L, 2) ||
      !lua_isstring(L, 3) || !lua_isstring(L, 4))
    luaL_error(L, "invalid arguments");

  plugin(L)->defineOption(lua_tostring(L, 2), Plugin::String,
                          lua_tostring(L, 3), lua_tostring(L, 4));

  return 0;
}

int optionsDefineList(lua_State *L) {
  if (lua_gettop(L) != 5 || !lua_istable(L, 1) || !lua_isstring(L, 2) ||
      !lua_isstring(L, 3) || !lua_istable(L, 4) || !lua_isinteger(L, 5))
    luaL_error(L, "invalid arguments");

  QStringList opts;
  for (int i = 1;; ++i) {
    lua_rawgeti(L, 4, i);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      break;
    }

    opts.append(lua_tostring(L, -1));
    lua_pop(L, 1);
  }

  plugin(L)->defineOption(lua_tostring(L, 2), Plugin::List, lua_tostring(L, 3),
                          lua_tointeger(L, 5), opts);

  return 0;
}

int optionsValue(lua_State *L) {
  if (lua_gettop(L) != 2 || !lua_istable(L, 1) || !lua_isstring(L, 2))
    luaL_error(L, "invalid arguments");

  QString key = lua_tostring(L, 2);
  QVariant value = plugin(L)->optionValue(key);
  if (!value.isValid())
    luaL_error(L, "invalid option");

  switch (plugin(L)->optionKind(key)) {
    case Plugin::Boolean:
      lua_pushboolean(L, value.toBool());
      break;

    case Plugin::List:
    case Plugin::Integer:
      lua_pushinteger(L, value.toInt());
      break;

    case Plugin::String:
      lua_pushstring(L, value.toString().toUtf8());
      break;
  }

  return 1;
}

int defineDiagnostic(lua_State *L, Plugin::DiagnosticKind kind) {
  if (lua_gettop(L) < 5 || lua_gettop(L) > 6 || !lua_istable(L, 1) ||
      !lua_isstring(L, 2) || !lua_isstring(L, 3) || !lua_isstring(L, 4) ||
      !lua_isstring(L, 5) || (lua_gettop(L) == 6 && !lua_isboolean(L, 6)))
    luaL_error(L, "invalid arguments");

  plugin(L)->defineDiagnostic(lua_tostring(L, 2), kind, lua_tostring(L, 3),
                              lua_tostring(L, 4), lua_tostring(L, 5),
                              (lua_gettop(L) == 6 && lua_toboolean(L, 6)));

  return 0;
}

int kindsDefineNote(lua_State *L) { return defineDiagnostic(L, Plugin::Note); }

int kindsDefineWarning(lua_State *L) {
  return defineDiagnostic(L, Plugin::Warning);
}

int kindsDefineError(lua_State *L) {
  return defineDiagnostic(L, Plugin::Error);
}

int hunkLines(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  TextEditor *editor = member<TextEditor *>(L, "_editor");

  // Create lines table.
  int count = editor->lineCount();
  lua_createtable(L, count, 0);
  for (int i = 0; i < count; ++i) {
    // index
    lua_pushinteger(L, i + 1);

    // Create line table.
    createInstance(L, plugin(L), "Line", kLineFuncs);
    setMember(L, "_editor", editor);
    setMember(L, "_line", i);

    // Add to hunk.
    lua_settable(L, -3);
  }

  return 1;
}

int hunkLexer(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  lua_pushstring(L, member<TextEditor *>(L, "_editor")->lexer().toUtf8());
  return 1;
}

int hunkTabWidth(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  lua_pushinteger(L, member<TextEditor *>(L, "_editor")->tabWidth());
  return 1;
}

int lineText(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  TextEditor *editor = member<TextEditor *>(L, "_editor");
  int line = member<int>(L, "_line");

  lua_pushstring(L, editor->line(line).toUtf8());
  return 1;
}

int lineOrigin(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  TextEditor *editor = member<TextEditor *>(L, "_editor");
  int line = member<int>(L, "_line");

  QByteArray marker = " ";
  int markers = editor->markers(line);
  if (markers & (1 << TextEditor::Addition)) {
    marker = "+";
  } else if (markers & (1 << TextEditor::Deletion)) {
    marker = "-";
  }

  lua_pushstring(L, marker);
  return 1;
}

void addLexeme(lua_State *L, Plugin *plugin, int index, TextEditor *editor,
               int pos, int style, const QByteArray &text) {
  // index
  lua_pushinteger(L, index);

  // Create lexeme table.
  createInstance(L, plugin, "Lexeme", kLexemeFuncs);
  setMember(L, "_editor", editor);
  setMember(L, "_pos", pos + 1);
  setMember(L, "_style", style);
  setMember(L, "_text", text.constData());

  lua_settable(L, -3);
}

int lineLexemes(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  TextEditor *editor = member<TextEditor *>(L, "_editor");
  int line = member<int>(L, "_line");

  // Create lexemes table.
  lua_newtable(L);
  int max = editor->lineEndPosition(line);
  int pos = editor->positionFromLine(line);
  if (pos == max)
    return 1;

  // Ensure styled to end of line.
  int endStyled = editor->endStyled();
  if (max > endStyled)
    editor->colorize(endStyled, max);

  int count = 0;
  int current = 0;
  QByteArray text(1, editor->charAt(pos));
  int style = editor->styleAt(pos);
  for (int i = pos + 1; i < max; ++i) {
    int nextStyle = editor->styleAt(i);
    if (nextStyle != style) {
      addLexeme(L, plugin(L), ++count, editor, current, style, text);
      current = i - pos;
      style = nextStyle;
      text = QByteArray();
    }

    text.append(editor->charAt(i));
  }

  addLexeme(L, plugin(L), ++count, editor, current, style, text);

  return 1;
}

int lineAddError(lua_State *L) {
  if (lua_gettop(L) < 4 || lua_gettop(L) > 5 || !lua_istable(L, 1) ||
      !lua_isstring(L, 2) || !lua_isinteger(L, 3) || !lua_isinteger(L, 4) ||
      (lua_gettop(L) == 5 && !lua_isstring(L, 5)))
    luaL_error(L, "invalid arguments");

  // Check if this error is enabled.
  QString key = lua_tostring(L, 2);
  if (!plugin(L)->isEnabled(key))
    return 0;

  TextEditor *editor = member<TextEditor *>(L, "_editor");
  int line = member<int>(L, "_line");

  // Add diagnostic.
  int len = lua_tointeger(L, 4);
  int pos = lua_tointeger(L, 3) - 1;
  QString msg = plugin(L)->diagnosticMessage(key);
  QString desc = plugin(L)->diagnosticDescription(key);
  TextEditor::DiagnosticKind kind =
      static_cast<TextEditor::DiagnosticKind>(plugin(L)->diagnosticKind(key));
  QString replacement = (lua_gettop(L) == 5) ? lua_tostring(L, 5) : QString();
  editor->addDiagnostic(line, {kind, msg, desc, {pos, len}, replacement});

  return 0;
}

int lineColumn(lua_State *L) {
  if (lua_gettop(L) != 2 || !lua_istable(L, 1) || !lua_isinteger(L, 2))
    luaL_error(L, "invalid arguments");

  TextEditor *editor = member<TextEditor *>(L, "_editor");
  int line = member<int>(L, "_line");

  int pos = editor->positionFromLine(line) + lua_tointeger(L, 2) - 1;
  lua_pushinteger(L, editor->column(pos) + 1);
  return 1;
}

int lineColumnPos(lua_State *L) {
  if (lua_gettop(L) != 2 || !lua_istable(L, 1) || !lua_isinteger(L, 2))
    luaL_error(L, "invalid arguments");

  TextEditor *editor = member<TextEditor *>(L, "_editor");
  int line = member<int>(L, "_line");

  int pos = editor->findColumn(line, lua_tointeger(L, 2) - 1);
  lua_pushinteger(L, pos - editor->positionFromLine(line) + 1);
  return 1;
}

int lexemeEq(lua_State *L) {
  if (lua_gettop(L) != 2 || !lua_istable(L, 1) || !lua_istable(L, 2))
    luaL_error(L, "invalid arguments");

  lua_getfield(L, 1, "_pos");
  int lhs = lua_tointeger(L, -1);
  lua_pop(L, 1); // lhs

  lua_getfield(L, 2, "_pos");
  int rhs = lua_tointeger(L, -1);
  lua_pop(L, 1); // rhs

  lua_pushboolean(L, lhs == rhs);
  return 1;
}

int lexemePos(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  lua_getfield(L, 1, "_pos");
  return 1;
}

int lexemeText(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  lua_getfield(L, 1, "_text");
  return 1;
}

QByteArray kind(TextEditor *editor, int style) {
  uintptr_t ptr = editor->privateLexerCall(style, 0);
  QByteArray name(reinterpret_cast<char *>(ptr));
  return name.endsWith("_whitespace") ? "whitespace" : name;
}

int lexemeKind(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_istable(L, 1))
    luaL_error(L, "invalid arguments");

  TextEditor *editor = member<TextEditor *>(L, "_editor");
  int style = member<int>(L, "_style");

  lua_pushstring(L, kind(editor, style));
  return 1;
}

int lexemeIsKind(lua_State *L) {
  if (lua_gettop(L) != 2 || !lua_istable(L, 1) || !lua_isstring(L, 2))
    luaL_error(L, "invalid arguments");

  TextEditor *editor = member<TextEditor *>(L, "_editor");
  int style = member<int>(L, "_style");

  lua_pushboolean(L, kind(editor, style) == QByteArray(lua_tostring(L, 2)));
  return 1;
}

} // namespace

Plugin::Plugin(const QString &file, const git::Repository &repo,
               QObject *parent)
    : QObject(parent), mRepo(repo), L(luaL_newstate()) {
  QFileInfo info(file);
  mDir = info.dir().path();
  mName = info.baseName();

  // Print error messages to the console.
  connect(this, &Plugin::error, [](const QString &msg) {
    QTextStream(stderr) << "plugin error: " << msg << Qt::endl;
  });

  // Load libraries.
  luaL_openlibs(L);

  // Add script dir to path.
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");
  QByteArray path = lua_tostring(L, -1);
  lua_pop(L, 1); // path
  lua_pushstring(L, path + ";" + mDir.toUtf8() + "/?.lua");
  lua_setfield(L, -2, "path");
  lua_pop(L, 1); // package

  // Load script.
  if (luaL_dofile(L, file.toLocal8Bit())) {
    setError(lua_tostring(L, -1));
    return;
  }

  // Read options.
  if (lua_getglobal(L, "options")) {
    // Create options table.
    createInstance(L, this, "Options", kOptionsFuncs);

    // Call options.
    if (lua_pcall(L, 1, 0, 0)) {
      setError(lua_tostring(L, -1));
      return;
    }

  } else {
    lua_pop(L, 1); // nil
  }

  // Read kinds.
  if (!lua_getglobal(L, "kinds")) {
    setError("global 'kinds' function not found");
    return;
  }

  // Create kinds and options tables.
  createInstance(L, this, "Kinds", kKindsFuncs);
  createInstance(L, this, "Options", kOptionsFuncs);

  // Call kinds.
  if (lua_pcall(L, 2, 0, 0))
    setError(lua_tostring(L, -1));
}

Plugin::~Plugin() { lua_close(L); }

bool Plugin::isValid() const { return mError.isEmpty(); }

QString Plugin::name() const { return mName; }

QString Plugin::scriptDir() const { return mDir; }

QString Plugin::errorString() const { return mError; }

bool Plugin::isEnabled() const {
  foreach (const QString &key, mDiagnostics.keys()) {
    if (isEnabled(key))
      return true;
  }

  return false;
}

bool Plugin::isEnabled(const QString &key) const {
  bool enabled = mDiagnostics.value(key).enabled;
  return config().value<bool>(kSubkeyFmt.arg(mName, key, "enabled"), enabled);
}

void Plugin::setEnabled(const QString &key, bool enabled) {
  if (enabled != isEnabled(key))
    config().setValue(kSubkeyFmt.arg(mName, key, "enabled"), enabled);
}

void Plugin::defineOption(const QString &key, OptionKind kind,
                          const QString &text, const QVariant &value,
                          const QStringList &opts) {
  mOptions.insert(key, {kind, text, value, opts});
}

void Plugin::setOptionValue(const QString &key, const QVariant &value) {
  switch (optionKind(key)) {
    case Boolean:
      config().setValue(kKeyFmt.arg(mName, key), value.toBool());
      break;

    case List:
    case Integer:
      config().setValue(kKeyFmt.arg(mName, key), value.toInt());
      break;

    case String:
      config().setValue(kKeyFmt.arg(mName, key), value.toString());
      break;
  }
}

QStringList Plugin::optionKeys() const { return mOptions.keys(); }

QString Plugin::optionText(const QString &key) const {
  return mOptions.value(key).text;
}

QVariant Plugin::optionValue(const QString &key) const {
  QVariant value = mOptions.value(key).value;
  switch (optionKind(key)) {
    case Boolean:
      return config().value<bool>(kKeyFmt.arg(mName, key), value.toBool());

    case List:
    case Integer:
      return config().value<int>(kKeyFmt.arg(mName, key), value.toInt());

    case String:
      return config().value<QString>(kKeyFmt.arg(mName, key), value.toString());

    default:
      throw std::runtime_error(
          "Not Implemented or invalid enum" +
          std::to_string(static_cast<int>(optionKind(key))));
  }
}

Plugin::OptionKind Plugin::optionKind(const QString &key) const {
  return mOptions.value(key).kind;
}

QStringList Plugin::optionOpts(const QString &key) const {
  return mOptions.value(key).opts;
}

void Plugin::defineDiagnostic(const QString &key, DiagnosticKind kind,
                              const QString &name, const QString &msg,
                              const QString &desc, bool enabled) {
  mDiagnostics.insert(key, {kind, name, msg, desc, enabled});
}

void Plugin::setDiagnosticKind(const QString &key, DiagnosticKind kind) {
  config().setValue<int>(kSubkeyFmt.arg(mName, key, "kind"), kind);
}

QStringList Plugin::diagnosticKeys() const { return mDiagnostics.keys(); }

Plugin::DiagnosticKind Plugin::diagnosticKind(const QString &key) const {
  DiagnosticKind defaultKind = mDiagnostics.value(key).kind;
  return static_cast<DiagnosticKind>(
      config().value<int>(kSubkeyFmt.arg(mName, key, "kind"), defaultKind));
}

QString Plugin::diagnosticName(const QString &key) const {
  return mDiagnostics.value(key).name;
}

QString Plugin::diagnosticMessage(const QString &key) const {
  return mDiagnostics.value(key).message;
}

QString Plugin::diagnosticDescription(const QString &key) const {
  return mDiagnostics.value(key).description;
}

bool Plugin::hunk(TextEditor *editor) const {
  if (!lua_getglobal(L, "hunk")) {
    const_cast<Plugin *>(this)->setError("global 'hunk' function not found");
    return false;
  }

  // Create hunk table.
  createInstance(L, const_cast<Plugin *>(this), "Hunk", kHunkFuncs);
  setMember(L, "_editor", editor);

  // Create options table.
  createInstance(L, const_cast<Plugin *>(this), "Options", kOptionsFuncs);

  // Call hunk function.
  if (lua_pcall(L, 2, 0, 0)) {
    const_cast<Plugin *>(this)->setError(lua_tostring(L, -1));
    return false;
  }

  return true;
}

QList<PluginRef> Plugin::plugins(const git::Repository &repo) {
  QList<PluginRef> plugins;
  QDir dir = Settings::pluginsDir();
  foreach (const QString &name, dir.entryList({"*.lua"}, QDir::Files))
    plugins.append(PluginRef(new Plugin(dir.filePath(name), repo)));

  QDir user = Settings::userDir();
  if (user.cd("plugins")) {
    foreach (const QString &name, user.entryList({"*.lua"}, QDir::Files))
      plugins.append(PluginRef(new Plugin(user.filePath(name), repo)));
  }

  return plugins;
}

git::Config Plugin::config() const {
  return mRepo.isValid() ? mRepo.appConfig() : git::Config::appGlobal();
}

void Plugin::setError(const QString &err) {
  lua_pop(L, 1); // error or nil
  mError = err;
  emit error(err);
}
