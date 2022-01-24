--
-- Many colors support 'active', 'inactive', and 'disabled' states.
-- They can all be set to the same color with the syntax:
--
--   key = '<color>'
--
-- Or set individually with syntax like:
--
--   key = { active = '<color>', inactive = '<color>', disabled = '<color>' }
--
-- Use the 'default' key set one state individually and the remainder
-- to a default value:
--
--   key = { default = '<color>', disabled = '<color>' }
--

-- generic colors used to render borders and separators
-- { default, active, inactive, disabled }
theme['palette']   = {
  -- These names correspond to a dark on light theme.
  -- The values should be inverted in light on dark themes.
  light            = '#FFFFFF', -- inverse of dark
  midlight         = '#F7F7F6', -- inverse of middark
  middark          = '#C4C9CD', -- inverse of midlight
  dark             = '#888E93', -- inverse of light

  -- This should always be a dark color.
  shadow           = '#474A4C'
}

-- the colors of text entry, list view, and other widgets
-- { default, active, inactive, disabled }
theme['widget']    = {
  text             = { default = '#232627', disabled = '#A0A2A2' },
  bright_text      = '#000000', --'#FFFFFF',
  background       = '#FCFCFC',
  alternate        = '#EFF0F1', -- an alternate background color for list rows
  highlight        = { active = '#3DAEE9', inactive = '#C2E0F5' },
  highlighted_text = { active = '#FCFCFC', inactive = '#232627' },
}

-- window colors
-- { default, active, inactive, disabled }
theme['window']    = {
  text             = '#232627',
  background       = '#EFF0F1'
}

-- button colors
-- { default, active, inactive, disabled, checked, pressed }
theme['button']    = {
  text             = { default = '#232627', inactive = '#232627', disabled = '#A0A2A2' },
  background       = { default = '#EFF0F1', checked = '#EFF0F1', pressed = '#E3E5E7' }
}

-- commit list colors
-- { default, active, inactive, disabled }
theme['commits']   = {
  text             = '#232627',
  bright_text      = '#000000',
  background       = '#FCFCFC',
  alternate        = '#EFF0F1', -- an alternate background color for list rows
  highlight        = { active = '#3DAEE9', inactive = '#C2E0F5' },
  highlighted_text = { active = '#FCFCFC', inactive = '#232627' },
  highlighted_bright_text = { active = '#FFFFFF', inactive = '#FFFFFF' }
}

-- status badge colors
-- { normal, selected, conflicted, head, notification }
theme['badge']     = {
  foreground       = {
    normal         = '#FFFFFF',
    selected       = '#6C6C6C'
  },
  background       = {
    normal         = '#A6ACB6', -- the default color
    selected       = '#FFFFFF', -- the color when a list item is selected
    conflicted     = '#D22222', -- the color of conflicted items
    head           = '#6F7379', -- a bolder color to indicate the HEAD
    notification   = '#FF0000'  -- the color of toolbar notifications badges
  }
}

-- blame margin heatmap background colors
theme['blame'] = {
  cold             = '#C0C0FF',
  hot              = '#FFC0C0'
}

-- graph edge colors
theme['graph']     = {
  edge1            = '#53AFEC',
  edge2            = '#82DA2A',
  edge3            = '#DA2ADA',
  edge4            = '#DA822A',
  edge5            = '#2ADADA',
  edge6            = '#DA2A82',
  edge7            = '#84A896',
  edge8            = '#2ADA82',
  edge9            = '#822ADA',
  edge10           = '#66D1E0',
  edge11           = '#D3C27E',
  edge12           = '#95CB80',
  edge13           = '#50D4BE',
  edge14           = '#2ADA82',
  edge15           = '#DA822A'
}

-- checkbox colors
-- { default, active, inactive, disabled }
theme['checkbox']  = {
  text             = '#232627',
  fill             = '#FCFCFC',
  outline          = '#EFF0F1'
}

-- commit editor colors
theme['commiteditor'] = {
  spellerror       = '#FF0000', -- spell check error
  spellignore      = '#A0A0A4', -- spell check ignored word(s)
  lengthwarning    = '#EFF0F1'  -- line length limit warning (background)
}

-- diff view colors
theme['diff']      = {
  addition         = '#DCFFDC', -- added lines
  deletion         = '#FFDCDC', -- deleted lines
  plus             = '#45CC45', -- plus icon
  minus            = '#F28080', -- minus icon
  ours             = '#DCFFFF', -- ours conflict lines
  theirs           = '#FFDCFF', -- theirs conflict lines
  word_addition    = '#B0F2B0', -- added words
  word_deletion    = '#F2B0B0', -- deleted words
  note             = '#000000', -- note squiggle
  warning          = '#FFFF00', -- warning background
  error            = '#FF0000'  -- error background
}

-- link colors
-- { default, active, inactive, disabled }
theme['link']      = {
  link             = '#2980B9',
  link_visited     = '#7F8C8D'
}

-- menubar background color
theme['menubar']   = {
  text             = '#232627',
  background       = '#FCFCFC'
}

-- tabbar background color (uncomment lines to customize)
theme['tabbar']   = {
  -- text             = theme['widget']['text'],
  -- base             = theme['palette']['dark'],
  -- selected         = theme['window']['background'],
}

-- remote comment colors
theme['comment']   = {
  background       = '#FCFCFC',
  body             = '#C2E0F5',
  author           = '#3DAEE9',
  timestamp        = '#232627'
}

-- star fill color
theme['star']      = {
  fill             = '#FFCE6D'
}

-- titlebar background color (currently only supported on macOS)
theme['titlebar']  = {
  background       = '#FCFCFC'
}

-- popup tooltip colors
-- { default, active, inactive, disabled }
theme['tooltip']   = {
  text             = '#FCFCFC',
  background       = '#232627'
}

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

-- colors
theme.property['color.red']          = '#800000'
theme.property['color.yellow']       = '#808000'
theme.property['color.green']        = '#008000'
theme.property['color.teal']         = '#008080'
theme.property['color.purple']       = '#800080'
theme.property['color.orange']       = '#B08000'
theme.property['color.blue']         = '#000080'
theme.property['color.black']        = '#000000'
theme.property['color.grey']         = '#808080'
theme.property['color.white']        = '#FFFFFF'

-- styles
theme.property['style.bracebad']     = '$(style.nothing)'
theme.property['style.bracelight']   = '$(style.nothing)'
theme.property['style.calltip']      = '$(style.nothing)'
theme.property['style.class']        = '$(style.nothing)'
theme.property['style.comment']      = 'fore:#0000FF'
theme.property['style.constant']     = '$(style.keyword)'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:$(color.black),back:$(color.white)'
theme.property['style.definition']   = '$(style.nothing)'
theme.property['style.embedded']     = '$(style.nothing)'
theme.property['style.error']        = 'fore:#FF0000'
theme.property['style.function']     = '$(style.label)'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = '$(style.nothing)'
theme.property['style.keyword']      = 'fore:$(color.blue),bold'
theme.property['style.label']        = 'fore:$(color.red)'
theme.property['style.linenumber']   = 'fore:$(color.black),back:#DCDCDC'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:$(color.teal)'
theme.property['style.operator']     = 'fore:$(color.black),bold'
theme.property['style.preprocessor'] = 'fore:$(color.green),bold'
theme.property['style.regex']        = '$(style.nothing)'
theme.property['style.string']       = 'fore:#CC0000'
theme.property['style.tag']          = '$(style.nothing)'
theme.property['style.type']         = 'fore:$(color.blue)'
theme.property['style.variable']     = '$(style.label)'
theme.property['style.whitespace']   = '$(style.nothing)'
