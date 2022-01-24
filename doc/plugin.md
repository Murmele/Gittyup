<html>
<head>
<style>
body {
  width: 800px;
  margin: 20px
}
h3 {
  text-decoration: underline
}
code {
  font-size: 85%;
  padding: 0.2em 0.4em;
  background-color: #F0F0F0;
  border-radius: 3px
}
pre {
  padding: 16px;
  margin: 16px;
  line-height: 1.45;
  background-color: #F0F0F0;
  border-radius: 3px
}
strong {
  font-size: 125%
}
</style>
</head>
<body>

# Gittyup Plugin API

---

## Overview

Plugins allow the application to be extended to flag errors in diffs.
Plugins are Lua scripts that detect one or more error kind. The name
of the script defines the name of the plugin category in the settings
interface. Plugins should be saved to one of the following locations:

* (Windows) - %APPDATA%\Gittyup\plugins
* (Linux) - ~/.config/Gittyup/plugins
* (macOS) - ~/Library/Application Support/Gittyup/plugins

---

## Conventions

Indexes are 1-based following the Lua convention.

Several APIs require `key` arguments to define and query values. Keys
should be strings that are unique for the given kind of object.

Line positions refer to the position of characters within a line
without regard to tab expansion. Column numbers take tab expansion
into account using the diff editor's current tab width.

---

## Plugin Format

Plugins should define the following global functions to be invoked
by the application:

**`options (opts)`** - Define options that the user can set in the
application. (optional)

* `opts` - the `Options` object to define user options

**`kinds (kinds, opts)`** - Define error kinds that can be added to diff
lines. (required)

* `kinds` - the `Kinds` object to define error kinds
* `opts` - the actual options set by the user

**`hunk (hunk, opts)`** - Invoked once for each hunk in the diff as it is
loaded. (required)

* `hunk` - the `Hunk` object that provides information about the hunk
* `opts` - the actual options set by the user

*Example:*

    function options (opts)
      -- define options
    end

    function kinds (kinds, opts)
      -- define kinds
    end

    function hunk (hunk, opts)
      -- handle one hunk
    end

---

## APIs

### Options

**`script_dir ()`** - Get the directory where the plugin script is located.

*Example:*

    local file = opts:script_dir() .. "/resource_file.txt"

**`define_boolean (key, text [, value])`** - Define a check box option.

* `text` - the text of the check box
* `value` - the default boolean state of the check box (false)

*Example:*

    opts:define_boolean("blank", "Allow blank lines", true)

**`define_integer (key, text [, value])`** - Define an integer spin box option.

* `text` - the text of the spin box label
* `value` - the default integer value of the spin box (0)

*Example:*

    opts:define_integer("column", "Line length limit", 80)

**`define_string (key, text [, value])`** - Define a text field option.

* `text` - the text of the field label
* `value` - the default string value of the text field ("")

*Example:*

    opts:define_string("keywords", "Keywords", "FIXME TODO TBD")

**`define_list (key, text, list [, index])`** - Define a popup list option.

* `text` - the text of the popup list label
* `list` - an ordered list of option strings
* `index` - the default index of the popup list (1)

*Example:*

    opts:define_list("tabs", "Disallow tabs", {"In indentation", "Everywhere"}, 2)

**`value (key)`** - Get the value of an option by key.

*Example:*

    local column = opts:value("column")

### Kinds

**`define_note (key, name, msg, desc [, enabled])`** - Define a note kind.

**`define_warning (key, name, msg, desc [, enabled])`** - Define a warning kind.

**`define_error (key, name, msg, desc [, enabled])`** - Define an error kind.

* `name` - the kind name to display in the settings interface
* `msg` - the message to display at each instance of the error
* `desc` - the kind description to display in the settings interface
* `enabled` - the default state of this kind (false)

*Example:*

    kinds:define_error(
      "trailing",
      "Trailing",
      "trailing whitespace added"
      "Trailing whitespace is not allowed.",
      true -- enabled by default
    )

Note that the severity of the kind (note, warning, error) is a default
value. It can be overridden by the user in plugin settings.

### Hunk

**`lines ()`** - Get an ordered list of `Line` objects.

*Example:*

    for _, line in ipairs(hunk:lines()) do
      -- handle each line
    end

**`lexer ()`** - Get the name of the lexer for the hunk editor.

*Example:*

    -- This plugin only applies to C/C++.
    if hunk:lexer() ~= "cpp" then
      return
    end

**`tab_width ()`** - Get the current tab width of the hunk editor.

### Line

**`text ()`** - Get line text. Includes any end-of-line characters.

**`origin ()`** - Get line origin character. Will be one of ' ' (context), '-'
(deletion), or '+' (addition).

*Example:*

    if line:origin() == "+" then
      -- handle added line
    end

**`lexemes ()`** - Get an ordered list of `Lexeme` objects in the line.

*Example:*

    for _, lexeme in ipairs(line:lexemes()) do
      -- handle each lexeme in line
    end

**`add_error (key, pos, len [, replacement])`** - Add an error to the line by key.

* `pos` - the start position of the diagnostic
* `len` - the length of the diagnostic
* `replacement` - replacement text to fix the violation

If `replacement` is not given (or nil), the fix button will be disabled. Pass
the empty string ("") to remove the given range.

*Example:*

    local len = last:text():len()
    line:add_error("trailing", line:text():len() - len, len, "")

**`column (pos)`** - Get the tab-expanded column for the given line position.

**`column_pos (column)`** - Get the position for the given tab-expanded column.

### Lexeme

**`pos ()`** - Get the position of the lexeme in the line.

**`text ()`** - Get the lexeme text.

**`kind ()`** - Get the lexeme kind string.

**`is_kind (kind)`** - Check if the lexeme is of the given kind.

* `kind` - the kind string to test

Lexeme kind will be one of:

* 'nothing'
* 'whitespace'
* 'comment'
* 'string'
* 'number'
* 'keyword'
* 'identifier'
* 'operator'
* 'error'
* 'preprocessor'
* 'constant'
* 'variable'
* 'function'
* 'class'
* 'type'
* 'label'
* 'regex'
* 'embedded'

Note that not all lexeme kinds are available for all languages. Documents for
which syntax highlighting is not available will have a single lexeme for each
line with the kind set to 'nothing'.

*Example:*

    if lexeme:is_kind("comment") then
      -- handle comment
    end

---

## Example Plugin

    function options (opts)
      opts:define_list("tabs", "Disallow tabs", {"In indentation", "Everywhere"}, 1)
      opts:define_boolean("blank", "Ignore blank lines", false)
    end

    function kinds (kinds, opts)
      kinds:define_warning(
        "tabs",
        "Tabs",
        "tab character added",
        "Tab characters are not allowed."
      )

      kinds:define_error(
        "trailing",
        "Trailing",
        "trailing whitespace added",
        "Trailing whitespace is not allowed.",
        true -- enabled by default
      )
    end

    function hunk (hunk, opts)
      if hunk:lexer() == "null" then
        return
      end

      local blank = opts:value("blank")
      local indent = opts:value("tabs") == 1
      for _, line in ipairs(hunk:lines()) do
        local lexemes = line:lexemes()
        local _, first = next(lexemes)
        if line:origin() == "+" and first then
          -- Check for tabs.
          local text = indent and first:text() or line:text()
          if text:find("\t") then
            local replacement = text:gsub("\t", string.rep(" ", hunk:tab_width()))
            line:add_error("tabs", 1, text:len(), replacement)
          end

          -- Check for trailing whitespace.
          local last = lexemes[#lexemes]
          if last:is_kind("whitespace") and (not blank or first ~= last) then
            local len = last:text():len()
            local line_len = line:text():gsub("[\r\n]+$", ""):len()
            line:add_error("trailing", line_len - len + 1, len, "")
          end
        end
      end
    end

</body>
</html>
