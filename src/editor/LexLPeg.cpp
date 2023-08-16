//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

// This is a stripped down version of the LPeg lexer. It's optimized for
// lexing lots of small editors. The most important optimization is sharing
// the lua state among all lexers. It's also not thread safe.

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "LexAccessor.h"
#include "LexerModule.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
LUALIB_API int luaopen_lpeg(lua_State *L);
}

#if _WIN32
#define strcasecmp _stricmp
#define PLATFORM "WIN32"
#elif __APPLE__
#define PLATFORM "OSX"
#else
#define PLATFORM "LINUX"
#endif

#define streq(s1, s2) (strcasecmp((s1), (s2)) == 0)

#define l_setmetatable(l, k, mtf)                                              \
  {                                                                            \
    if (luaL_newmetatable(l, k)) {                                             \
      lua_pushcfunction(l, mtf), lua_setfield(l, -2, "__index");               \
      lua_pushcfunction(l, mtf), lua_setfield(l, -2, "__newindex");            \
    }                                                                          \
    lua_setmetatable(l, -2);                                                   \
  }
#define l_pushlexerp(l, mtf)                                                   \
  {                                                                            \
    lua_newtable(l);                                                           \
    lua_pushvalue(l, 2), lua_setfield(l, -2, "property");                      \
    l_setmetatable(l, "sci_lexerp", mtf);                                      \
  }
#define l_getlexerobj(l)                                                       \
  {                                                                            \
    lua_getfield(l, LUA_REGISTRYINDEX, "sci_lexers");                          \
    lua_pushlightuserdata(l, reinterpret_cast<void *>(this));                  \
    lua_gettable(l, -2), lua_replace(l, -2);                                   \
  }
#define l_getlexerfield(l, k)                                                  \
  {                                                                            \
    l_getlexerobj(l);                                                          \
    lua_getfield(l, -1, k), lua_replace(l, -2);                                \
  }
#define l_openlib(f, s) (luaL_requiref(L, s, f, 1), lua_pop(L, 1))
#define LUA_BASELIBNAME "_G"
#define l_setconstant(l, c, k) (lua_pushinteger(l, c), lua_setfield(l, -2, k))

using namespace Scintilla;

class LexerLPeg : public ILexer5 {
  // Shared lua state.
  static lua_State *L;

  // The set of properties for the lexer.
  // The `lexer.name`, `lexer.lpeg.home`, and `lexer.lpeg.color.theme`
  // properties must be defined before running the lexer.
  PropSetSimple props;

  // The function to send Scintilla messages with.
  SciFnDirect fn;

  // The Scintilla object the lexer belongs to.
  sptr_t sci;

  // The flag indicating whether or not the lexer language has embedded lexers.
  bool multilang;

  // The list of style numbers considered to be whitespace styles.
  // This is used in multi-language lexers when backtracking to whitespace to
  // determine which lexer grammar to use.
  bool ws[STYLE_MAX + 1];

  /**
   * Prints the given message or a Lua error message and clears the stack.
   * @param str The error message to print. If `NULL`, prints the Lua error
   *   message at the top of the stack.
   */
  static void l_error(lua_State *L, const char *str = NULL) {
    fprintf(stderr, "Lua Error: %s.\n", str ? str : lua_tostring(L, -1));
    lua_settop(L, 0);
  }

  /** The lexer's `__index` Lua metatable. */
  static int llexer_property(lua_State *L) {
    int newindex = (lua_gettop(L) == 3);
    luaL_getmetatable(L, "sci_lexer");
    lua_getmetatable(L, 1); // metatable can be either sci_lexer or sci_lexerp
    int is_lexer = lua_compare(L, -1, -2, LUA_OPEQ);
    lua_pop(L, 2); // metatable, metatable

    lua_getfield(L, LUA_REGISTRYINDEX, "sci_buffer");
    IDocument *buffer = static_cast<IDocument *>(lua_touserdata(L, -1));
    lua_getfield(L, LUA_REGISTRYINDEX, "sci_props");
    PropSetSimple *props = static_cast<PropSetSimple *>(lua_touserdata(L, -1));
    lua_pop(L, 2); // sci_props and sci_buffer

    if (is_lexer)
      lua_pushvalue(L, 2); // key is given
    else
      lua_getfield(L, 1, "property"); // indexible property
    const char *key = lua_tostring(L, -1);
    if (strcmp(key, "fold_level") == 0) {
      luaL_argcheck(L, !newindex, 3, "read-only property");
      if (is_lexer) {
        l_pushlexerp(L, llexer_property);
      } else
        lua_pushinteger(L, buffer->GetLevel(luaL_checkinteger(L, 2)));
    } else if (strcmp(key, "indent_amount") == 0) {
      luaL_argcheck(L, !newindex, 3, "read-only property");
      if (is_lexer) {
        l_pushlexerp(L, llexer_property);
      } else
        lua_pushinteger(L, buffer->GetLineIndentation(luaL_checkinteger(L, 2)));
    } else if (strcmp(key, "property") == 0) {
      luaL_argcheck(L, !is_lexer || !newindex, 3, "read-only property");
      if (is_lexer) {
        l_pushlexerp(L, llexer_property);
      } else if (!newindex) {
        lua_pushstring(L, props->Get(luaL_checkstring(L, 2)));
      } else {
        const char *key = luaL_checkstring(L, 2);
        const char *val = luaL_checkstring(L, 3);
        props->Set(key, val, strlen(key), strlen(val));
      }
    } else if (strcmp(key, "property_int") == 0) {
      luaL_argcheck(L, !newindex, 3, "read-only property");
      if (is_lexer) {
        l_pushlexerp(L, llexer_property);
      } else {
        lua_pushstring(L, props->Get(luaL_checkstring(L, 2)));
        lua_pushinteger(L, lua_tointeger(L, -1));
      }
    } else if (strcmp(key, "style_at") == 0) {
      luaL_argcheck(L, !newindex, 3, "read-only property");
      if (is_lexer) {
        l_pushlexerp(L, llexer_property);
      } else {
        int style = buffer->StyleAt(luaL_checkinteger(L, 2) - 1);
        lua_getfield(L, LUA_REGISTRYINDEX, "sci_lexer_obj");
        lua_getfield(L, -1, "_TOKENSTYLES"), lua_replace(L, -2);
        lua_pushnil(L);
        while (lua_next(L, -2)) {
          if (luaL_checkinteger(L, -1) == style)
            break;
          lua_pop(L, 1); // value
        }
        lua_pop(L, 1); // style_num
      }
    } else
      return !newindex ? (lua_rawget(L, 1), 1) : (lua_rawset(L, 1), 0);
    return 1;
  }

  /**
   * Expands value of the string property key at index *index* and pushes the
   * result onto the stack.
   * @param L The Lua State.
   * @param index The index the string property key.
   */
  void lL_getexpanded(lua_State *L, int index) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED"), lua_getfield(L, -1, "lexer");
    lua_getfield(L, -1, "property_expanded");
    lua_pushvalue(L, (index > 0) ? index : index - 3), lua_gettable(L, -2);
    lua_replace(L, -4), lua_pop(L, 2); // property_expanded and lexer module
  }

  /**
   * Parses the given style string to set the properties for the given style
   * number.
   * @param num The style number to set properties for.
   * @param style The style string containing properties to set.
   */
  void SetStyle(int num, const char *style) {
    char *style_copy = static_cast<char *>(malloc(strlen(style) + 1));
    char *option = strcpy(style_copy, style), *next = NULL, *p = NULL;
    while (option) {
      if ((next = strchr(option, ',')))
        *next++ = '\0';
      if ((p = strchr(option, ':')))
        *p++ = '\0';
      if (streq(option, "font"))
        fn(sci, SCI_STYLESETFONT, num, reinterpret_cast<sptr_t>(p));
      else if (streq(option, "size"))
        fn(sci, SCI_STYLESETSIZE, num, static_cast<int>(atoi(p)));
      else if (streq(option, "bold") || streq(option, "notbold")) {
        fn(sci, SCI_STYLESETBOLD, num, *option == 'b');
      } else if (streq(option, "italics") || streq(option, "notitalics"))
        fn(sci, SCI_STYLESETITALIC, num, *option == 'i');
      else if (streq(option, "underlined") || streq(option, "notunderlined")) {
        fn(sci, SCI_STYLESETUNDERLINE, num, *option == 'u');
      } else if (streq(option, "fore") || streq(option, "back")) {
        int msg = (*option == 'f') ? SCI_STYLESETFORE : SCI_STYLESETBACK;
        int color = static_cast<int>(strtol(p, NULL, 0));
        if (*p == '#') { // #RRGGBB format; Scintilla format is 0xBBGGRR
          color = static_cast<int>(strtol(p + 1, NULL, 16));
          color = ((color & 0xFF0000) >> 16) | (color & 0xFF00) |
                  ((color & 0xFF) << 16); // convert to 0xBBGGRR
        }
        fn(sci, msg, num, color);
      } else if (streq(option, "eolfilled") || streq(option, "noteolfilled"))
        fn(sci, SCI_STYLESETEOLFILLED, num, *option == 'e');
      else if (streq(option, "characterset"))
        fn(sci, SCI_STYLESETCHARACTERSET, num, static_cast<int>(atoi(p)));
      else if (streq(option, "case") && p) {
        if (*p == 'u')
          fn(sci, SCI_STYLESETCASE, num, SC_CASE_UPPER);
        else if (*p == 'l')
          fn(sci, SCI_STYLESETCASE, num, SC_CASE_LOWER);
      } else if (streq(option, "visible") || streq(option, "notvisible"))
        fn(sci, SCI_STYLESETVISIBLE, num, *option == 'v');
      else if (streq(option, "changeable") || streq(option, "notchangeable"))
        fn(sci, SCI_STYLESETCHANGEABLE, num, *option == 'c');
      else if (streq(option, "hotspot") || streq(option, "nothotspot"))
        fn(sci, SCI_STYLESETHOTSPOT, num, *option == 'h');
      option = next;
    }
    free(style_copy);
  }

  /**
   * Iterates through the lexer's `_TOKENSTYLES`, setting the style properties
   * for all defined styles, or for SciTE, generates the set of style properties
   * instead of directly setting style properties.
   */
  bool SetStyles() {
    // If the lexer defines additional styles, set their properties first (if
    // the user has not already defined them).
    l_getlexerfield(L, "_EXTRASTYLES");
    lua_pushnil(L);
    while (lua_next(L, -2)) {
      if (lua_isstring(L, -2) && lua_isstring(L, -1)) {
        lua_pushstring(L, "style."), lua_pushvalue(L, -3), lua_concat(L, 2);
        if (!*props.Get(lua_tostring(L, -1))) {
          const char *key = lua_tostring(L, -1);
          const char *val = lua_tostring(L, -2);
          props.Set(key, val, strlen(key), strlen(val));
        }
        lua_pop(L, 1); // style name
      }
      lua_pop(L, 1); // value
    }
    lua_pop(L, 1); // _EXTRASTYLES

    l_getlexerfield(L, "_TOKENSTYLES");
    if (!fn || !sci) {
      lua_pop(L, 1); // _TOKENSTYLES
      // Skip, but do not report an error since `reinit` would remain `false`
      // and subsequent calls to `Lex()` and `Fold()` would repeatedly call this
      // function and error.
      return true;
    }
    lua_pushstring(L, "style.default"), lL_getexpanded(L, -1);
    SetStyle(STYLE_DEFAULT, lua_tostring(L, -1));
    lua_pop(L, 2);                    // style and "style.default"
    fn(sci, SCI_STYLECLEARALL, 0, 0); // set default styles
    lua_pushnil(L);
    while (lua_next(L, -2)) {
      if (lua_isstring(L, -2) && lua_isnumber(L, -1) &&
          lua_tointeger(L, -1) != STYLE_DEFAULT) {
        lua_pushstring(L, "style."), lua_pushvalue(L, -3), lua_concat(L, 2);
        lL_getexpanded(L, -1), lua_replace(L, -2);
        SetStyle(lua_tointeger(L, -2), lua_tostring(L, -1));
        lua_pop(L, 1); // style
      }
      lua_pop(L, 1); // value
    }
    lua_pop(L, 1); // _TOKENSTYLES
    return true;
  }

  /**
   * Initializes the lexer once the `lexer.lpeg.home` and `lexer.name`
   * properties are set.
   */
  bool init(const char *lexer) {
    char home[FILENAME_MAX], themes[FILENAME_MAX], theme[FILENAME_MAX];
    props.GetExpanded("lexer.lpeg.home", home);
    props.GetExpanded("lexer.lpeg.themes", themes);
    props.GetExpanded("lexer.lpeg.theme", theme);
    if (!*home || !*lexer)
      return false;

    lua_pushlightuserdata(L, reinterpret_cast<void *>(&props));
    lua_setfield(L, LUA_REGISTRYINDEX, "sci_props");

    // Set `package.path` to find lexers.
    lua_getglobal(L, "package");
    lua_pushstring(L, home);
    lua_pushstring(L, "/?.lua");
    lua_concat(L, 2);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1); // package

    // Load the lexer module.
    lua_getglobal(L, "require");
    lua_pushstring(L, "lexer");
    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
      return (l_error(L), false);
    lua_pushvalue(L, -1), lua_setglobal(L, "lexer");
    l_setconstant(L, SC_FOLDLEVELBASE, "FOLD_BASE");
    l_setconstant(L, SC_FOLDLEVELWHITEFLAG, "FOLD_BLANK");
    l_setconstant(L, SC_FOLDLEVELHEADERFLAG, "FOLD_HEADER");
    l_setmetatable(L, "sci_lexer", llexer_property);
    if (*theme) {
      char mode[FILENAME_MAX];
      props.GetExpanded("lexer.lpeg.theme.mode", mode);

      lua_newtable(L);
      lua_pushboolean(L, strcmp(mode, "dark") == 0);
      lua_setfield(L, -2, "dark"); // system palette
      l_setmetatable(L, "sci_lexer", llexer_property);
      lua_setglobal(L, "theme");

      // Load the theme.
      if (!(strstr(theme, "/") || strstr(theme, "\\"))) { // theme name
        lua_pushstring(L, themes);
        lua_pushstring(L, "/");
        lua_pushstring(L, theme);
        lua_pushstring(L, ".lua");
        lua_concat(L, 4);
      } else
        lua_pushstring(L, theme); // path to theme
      if (luaL_loadfile(L, lua_tostring(L, -1)) != LUA_OK ||
          lua_pcall(L, 0, 0, 0) != LUA_OK)
        return (l_error(L), false);
      lua_pop(L, 1); // theme
    }

    // Load the language lexer.
    lua_getfield(L, -1, "load");
    if (lua_isfunction(L, -1)) {
      lua_pushstring(L, lexer);
      if (lua_pcall(L, 1, 1, 0) != LUA_OK)
        return (l_error(L), false);
    } else
      return (l_error(L, "'lexer.load' function not found"), false);
    lua_getfield(L, LUA_REGISTRYINDEX, "sci_lexers");
    lua_pushlightuserdata(L, reinterpret_cast<void *>(this));
    lua_pushvalue(L, -3), lua_settable(L, -3), lua_pop(L, 1); // sci_lexers
    lua_pushvalue(L, -1), lua_setfield(L, LUA_REGISTRYINDEX, "sci_lexer_obj");
    lua_remove(L, -2); // lexer module
    if (!SetStyles())
      return false;

    // If the lexer is a parent, it will have children in its _CHILDREN table.
    lua_getfield(L, -1, "_CHILDREN");
    if (lua_istable(L, -1)) {
      multilang = true;
      // Determine which styles are language whitespace styles
      // ([lang]_whitespace). This is necessary for determining which language
      // to start lexing with.
      char style_name[50];
      for (int i = 0; i <= STYLE_MAX; i++) {
        PrivateCall(i, reinterpret_cast<void *>(style_name));
        ws[i] = strstr(style_name, "whitespace") ? true : false;
      }
    }
    lua_pop(L, 2); // _CHILDREN and lexer object

    return true;
  }

public:
  LexerLPeg() : fn(nullptr), sci(0), multilang(false) {
    // Lua state is shared.
    if (L)
      return;

    // Create new state.
    L = luaL_newstate();
    if (!L) {
      fprintf(stderr, "Lua failed to initialize.\n");
      return;
    }

    // Load libraries and set platform.
    l_openlib(luaopen_base, LUA_BASELIBNAME);
    l_openlib(luaopen_table, LUA_TABLIBNAME);
    l_openlib(luaopen_string, LUA_STRLIBNAME);
    l_openlib(luaopen_package, LUA_LOADLIBNAME);
    l_openlib(luaopen_lpeg, "lpeg");
    lua_pushboolean(L, 1), lua_setglobal(L, PLATFORM);
    lua_newtable(L), lua_setfield(L, LUA_REGISTRYINDEX, "sci_lexers");
  }

  virtual ~LexerLPeg() {}

  /** Destroys the lexer object. */
  void SCI_METHOD Release() override {
    lua_getfield(L, LUA_REGISTRYINDEX, "sci_lexers");
    lua_pushlightuserdata(L, reinterpret_cast<void *>(this));
    lua_pushnil(L), lua_settable(L, -3), lua_pop(L, 1); // sci_lexers

    delete this;
  }

  /**
   * Lexes the Scintilla document.
   * @param startPos The position in the document to start lexing at.
   * @param lengthDoc The number of bytes in the document to lex.
   * @param initStyle The initial style at position *startPos* in the document.
   * @param buffer The document interface.
   */
  void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc,
                      int initStyle, IDocument *buffer) override {
    lua_pushlightuserdata(L, reinterpret_cast<void *>(&props));
    lua_setfield(L, LUA_REGISTRYINDEX, "sci_props");
    lua_pushlightuserdata(L, reinterpret_cast<void *>(buffer));
    lua_setfield(L, LUA_REGISTRYINDEX, "sci_buffer");
    LexAccessor styler(buffer);

    // Ensure the lexer has a grammar.
    // This could be done in the lexer module's `lex()`, but for large files,
    // passing string arguments from C to Lua is expensive.
    l_getlexerfield(L, "_GRAMMAR");
    int has_grammar = !lua_isnil(L, -1);
    lua_pop(L, 1); // _GRAMMAR
    if (!has_grammar) {
      // Style everything in the default style.
      styler.StartAt(startPos);
      styler.StartSegment(startPos);
      styler.ColourTo(startPos + lengthDoc - 1, STYLE_DEFAULT);
      styler.Flush();
      return;
    }

    // Start from the beginning of the current style so LPeg matches it.
    // For multilang lexers, start at whitespace since embedded languages have
    // [lang]_whitespace styles. This is so LPeg can start matching child
    // languages instead of parent ones if necessary.
    if (startPos > 0) {
      Sci_PositionU i = startPos;
      while (i > 0 && styler.StyleAt(i - 1) == initStyle)
        i--;
      if (multilang)
        while (i > 0 && !ws[static_cast<size_t>(styler.StyleAt(i))])
          i--;
      lengthDoc += startPos - i, startPos = i;
    }

    Sci_PositionU startSeg = startPos, endSeg = startPos + lengthDoc;
    int style = 0;
    l_getlexerfield(L, "lex") if (lua_isfunction(L, -1)) {
      l_getlexerobj(L);
      lua_pushlstring(L, buffer->BufferPointer() + startPos, lengthDoc);
      lua_pushinteger(L, styler.StyleAt(startPos));
      if (lua_pcall(L, 3, 1, 0) != LUA_OK)
        l_error(L);
      // Style the text from the token table returned.
      if (lua_istable(L, -1)) {
        int len = lua_rawlen(L, -1);
        if (len > 0) {
          styler.StartAt(startPos);
          styler.StartSegment(startPos);
          l_getlexerfield(L, "_TOKENSTYLES");
          // Loop through token-position pairs.
          for (int i = 1; i < len; i += 2) {
            style = STYLE_DEFAULT;
            lua_rawgeti(L, -2, i), lua_rawget(L, -2); // _TOKENSTYLES[token]
            if (!lua_isnil(L, -1))
              style = lua_tointeger(L, -1);
            lua_pop(L, 1);             // _TOKENSTYLES[token]
            lua_rawgeti(L, -2, i + 1); // pos
            unsigned int position = lua_tointeger(L, -1) - 1;
            lua_pop(L, 1); // pos
            if (style >= 0 && style <= STYLE_MAX)
              styler.ColourTo(startSeg + position - 1, style);
            else
              l_error(L, "Bad style number");
            if (position > endSeg)
              break;
          }
          lua_pop(L, 2); // _TOKENSTYLES and token table returned
          styler.ColourTo(endSeg - 1, style);
          styler.Flush();
        }
      } else
        l_error(L, "Table of tokens expected from 'lexer.lex'");
    }
    else l_error(L, "'lexer.lex' function not found");
  }

  /**
   * Folds the Scintilla document.
   * @param startPos The position in the document to start folding at.
   * @param lengthDoc The number of bytes in the document to fold.
   * @param initStyle The initial style at position *startPos* in the document.
   * @param buffer The document interface.
   */
  void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position lengthDoc,
                       int initStyle, IDocument *buffer) override {
    lua_pushlightuserdata(L, reinterpret_cast<void *>(&props));
    lua_setfield(L, LUA_REGISTRYINDEX, "sci_props");
    lua_pushlightuserdata(L, reinterpret_cast<void *>(buffer));
    lua_setfield(L, LUA_REGISTRYINDEX, "sci_buffer");
    LexAccessor styler(buffer);

    l_getlexerfield(L, "fold");
    if (lua_isfunction(L, -1)) {
      l_getlexerobj(L);
      Sci_Position currentLine = styler.GetLine(startPos);
      lua_pushlstring(L, buffer->BufferPointer() + startPos, lengthDoc);
      lua_pushinteger(L, startPos);
      lua_pushinteger(L, currentLine);
      lua_pushinteger(L, styler.LevelAt(currentLine) & SC_FOLDLEVELNUMBERMASK);
      if (lua_pcall(L, 5, 1, 0) != LUA_OK)
        l_error(L);
      // Fold the text from the fold table returned.
      if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2)) { // line = level
          styler.SetLevel(lua_tointeger(L, -2), lua_tointeger(L, -1));
          lua_pop(L, 1); // level
        }
        lua_pop(L, 1); // fold table returned
      } else
        l_error(L, "Table of folds expected from 'lexer.fold'");
    } else
      l_error(L, "'lexer.fold' function not found");
  }

  /**
   * Sets the *key* lexer property to *value*.
   * If *key* starts with "style.", also set the style for the token.
   * @param key The string keyword.
   * @param val The string value.
   */
  Sci_Position SCI_METHOD PropertySet(const char *key, const char *value) override {
    const char *val = *value ? value : " ";
    props.Set(key, val, strlen(key), strlen(val)); // ensure property is cleared
    return -1;                                     // no need to re-lex
  }

  /**
   * Allows for direct communication between the application and the lexer.
   * The application uses this to set `SS`, `sci`, `L`, and lexer properties,
   * and to retrieve style names.
   * @param code The communication code.
   * @param arg The argument.
   * @return void *data
   */
  void *SCI_METHOD PrivateCall(int code, void *arg) override {
    switch (code) {
      case SCI_GETDIRECTFUNCTION:
        fn = reinterpret_cast<SciFnDirect>(arg);
        return nullptr;

      case SCI_SETDOCPOINTER:
        sci = reinterpret_cast<sptr_t>(arg);
        return nullptr;

      case SCI_SETLEXERLANGUAGE:
        init(reinterpret_cast<const char *>(arg));
        return nullptr;

      default:
        if (code < 0 || code > STYLE_MAX)
          return nullptr;

        // Look up style name.
        const char *name = nullptr;
        l_getlexerfield(L, "_TOKENSTYLES");
        lua_pushnil(L);
        while (lua_next(L, -2)) {
          if (lua_tointeger(L, -1) == code) {
            name = lua_tostring(L, -2);
            lua_pop(L, 2); // value and key
            break;
          } else {
            lua_pop(L, 1); // value
          }
        }

        lua_pop(L, 1); // _TOKENSTYLES
        return const_cast<char *>(name);
    }
  }

  int SCI_METHOD Version() const override { return 0; }
  const char *SCI_METHOD PropertyNames() override { return ""; }
  int SCI_METHOD PropertyType(const char *) override { return 0; }
  const char *SCI_METHOD DescribeProperty(const char *) override { return ""; }
  const char *SCI_METHOD DescribeWordListSets() override { return ""; }
  Sci_Position SCI_METHOD WordListSet(int, const char *) override { return -1; }

  int SCI_METHOD LineEndTypesSupported() noexcept override {
    return SC_LINE_END_TYPE_UNICODE;
  }

  int SCI_METHOD AllocateSubStyles(int styleBase, int numberStyles) override {
    return 0;
  }
  int SCI_METHOD SubStylesStart(int styleBase) override { return 0; }
  int SCI_METHOD SubStylesLength(int styleBase) override { return 0; }
  int SCI_METHOD StyleFromSubStyle(int subStyle) override { return 0; }
  int SCI_METHOD PrimaryStyleFromStyle(int style) noexcept override {
    return 0;
  }
  void SCI_METHOD FreeSubStyles() override {}
  void SCI_METHOD SetIdentifiers(int style, const char *identifiers) override {}
  int SCI_METHOD DistanceToSecondaryStyles() noexcept override { return 0; }
  const char *SCI_METHOD GetSubStyleBases() noexcept override { return ""; }
  int SCI_METHOD NamedStyles() override { return 0; }
  const char *SCI_METHOD NameOfStyle(int style) override { return ""; }
  const char *SCI_METHOD TagsOfStyle(int style) override { return ""; }
  const char *SCI_METHOD DescriptionOfStyle(int style) override { return ""; }

  const char *SCI_METHOD GetName() override { return ""; }

  int SCI_METHOD GetIdentifier() override { return 0; }

  const char *SCI_METHOD PropertyGet(const char *key) override { return ""; }

  /** Constructs a new instance of the lexer. */
  static ILexer5 *LexerFactoryLPeg() { return new LexerLPeg(); }
};

lua_State *LexerLPeg::L = NULL;
LexerModule lmLPeg(SCLEX_AUTOMATIC - 1, LexerLPeg::LexerFactoryLPeg, "lpeg");
