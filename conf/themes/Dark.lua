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
  light            = '#11111b', -- inverse of dark
  midlight         = '#181825', -- inverse of middark
  middark          = '#1e1e2e', -- inverse of midlight
  dark             = '#313244', -- inverse of light

  -- This should always be a dark color.
  shadow           = '#181825'
}

-- the colors of text entry, list view, and other widgets
-- { default, active, inactive, disabled }
theme['widget']    = {
  text             = { default = '#cdd6f4', disabled = '#a6adc8' },
  bright_text      = '#bac2de',
  background       = '#181825',
  alternate        = '#1e1e2e', -- an alternate background color for list rows
  highlight        = { active = '#2A82DA', inactive = '#1B5B9B' },
  highlighted_text = { active = '#cdd6f4', inactive = '#cdd6f4' },
}

-- window colors
-- { default, active, inactive, disabled }
theme['window']    = {
  text             = '#cdd6f4',
  background       = '#1e1e2e'
}

-- button colors
-- { default, active, inactive, disabled, checked, pressed }
theme['button']    = {
  text             = { default = '#cdd6f4', inactive = '#a6adc8', disabled = '#a6adc8' },
  background       = { default = '#1e1e2e', checked = '#2A82DA', pressed = '#2A82DA' }
}

-- commit list colors
-- { default, active, inactive, disabled }
theme['commits']   = {
  text             = '#bac2de',
  bright_text      = '#cdd6f4',
  background       = '#1e1e2e',
  alternate        = '#1e1e2e', -- an alternate background color for list rows
  highlight        = { active = '#2A82DA', inactive = '#1B5B9B' },
  highlighted_text = { active = '#A6CBF0', inactive = '#cdd6f4' },
  highlighted_bright_text = { active = '#cdd6f4', inactive = '#9090A5' }
}

-- status badge colors
-- { normal, selected, conflicted, head, notification }
theme['badge']     = {
  foreground       = {
    normal         = '#cdd6f4',
    selected       = '#2A82DA'
  },
  background       = {
    normal         = '#2A82DA', -- the default color
    selected       = '#cdd6f4', -- the color when a list item is selected
    conflicted     = '#DA2ADA', -- the color of conflicted items
    head           = '#52A500', -- a bolder color to indicate the HEAD
    notification   = '#8C2026',  -- the color of toolbar notifications badges
    modified       = '#91973A', -- (yellow) the color of the badge when the file is modified
    added          = '#394734', -- (green) the color of the badge when the file was newly added
    deleted        = '#5E3638', -- (red) the color of the badge when the file was deleted
	untracked	   = '#2A4944', -- (green blue) the color of the badge when the file is untracked
	renamed		   = '#23455E'  -- (blue) the color of the badge when the file is renamed
  }
}

-- blame margin heatmap background colors
theme['blame'] = {
  cold             = '#282940',
  hot              = '#5E3638'
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
  text             = '#cdd6f4',
  fill             = '#535359',
  outline          = '#3C3C42'
}

-- commit editor colors
theme['commiteditor'] = {
  spellerror       = '#BC0009', -- spell check error
  spellignore      = '#cdd6f4', -- spell check ignored word(s)
  lengthwarning    = '#464614'  -- line length limit warning (background)
}

-- diff view colors
theme['diff']      = {
  addition         = '#394734', -- added lines
  deletion         = '#5E3638', -- deleted lines
  plus             = '#207A00', -- plus icon
  minus            = '#BC0009', -- minus icon
  ours             = '#000060', -- ours conflict lines
  theirs           = '#600060', -- theirs conflict lines
  word_addition    = '#296812', -- added words
  word_deletion    = '#781B20', -- deleted words
  note             = '#cdd6f4', -- note squiggle
  warning          = '#E8C080', -- warning background
  error            = '#7E494B'  -- error background
}

-- link colors
-- { default, active, inactive, disabled }
theme['link']      = {
  link             = '#2A82DA',
  link_visited     = '#FF00FF'
}

-- menubar background color
theme['menubar']   = {
  text             = '#cdd6f4',
  background       = '#181825'
}

-- tabbar background color (uncomment lines to customize)
theme['tabbar']   = {
  -- text             = theme['widget']['text'],
  -- base             = theme['palette']['dark'],
  -- selected         = theme['window']['background'],
}

-- remote comment colors
theme['comment']   = {
  background       = '#212228',
  body             = '#bac2de',
  author           = '#378BDD',
  timestamp        = '#cdd6f4'
}

-- star fill color
theme['star']      = {
  fill             = '#FFFFFF'
}

-- titlebar background color (currently only supported on macOS)
theme['titlebar']  = {
  background       = '#36383D'
}

-- popup tooltip colors
-- { default, active, inactive, disabled }
theme['tooltip']   = {
  text             = '#cdd6f4',
  background       = '#2A82DA'
}

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

-- colors
theme.property['color.red']          = '#994D4D'
theme.property['color.yellow']       = '#99994D'
theme.property['color.green']        = '#4D994D'
theme.property['color.teal']         = '#4D9999'
theme.property['color.purple']       = '#994D99'
theme.property['color.orange']       = '#E6994D'
theme.property['color.blue']         = '#4D99E6'
theme.property['color.black']        = '#1A1A1A'
theme.property['color.grey']         = '#808080'
theme.property['color.white']        = '#E6E6E6'

-- styles
theme.property['style.bracebad']     = 'fore:#CC8080'
theme.property['style.bracelight']   = 'fore:#80CCFF'
theme.property['style.calltip']      = 'fore:#bac2de,back:#333333'
theme.property['style.class']        = 'fore:#F6E9D0'
theme.property['style.comment']      = 'fore:#E2D9C9'
theme.property['style.constant']     = 'fore:#E8C080'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:#bac2de,back:#212228'
theme.property['style.definition']   = 'fore:#F6E9D0'
theme.property['style.embedded']     = '$(style.tag),back:#333333'
theme.property['style.error']        = 'fore:#994D4D'
theme.property['style.function']     = 'fore:#4D99E6'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#333333,back:#333333'
theme.property['style.keyword']      = 'fore:#53AFEC,bold'
theme.property['style.label']        = 'fore:#E8C080'
theme.property['style.linenumber']   = 'fore:#5F6672,back:#2A2B30,bold'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:#4D99E6'
theme.property['style.operator']     = 'fore:#CCCCCC,bold'
theme.property['style.preprocessor'] = 'fore:#CC77DA,bold'
theme.property['style.regex']        = 'fore:#80CC80'
theme.property['style.string']       = 'fore:#93C37E'
theme.property['style.tag']          = 'fore:#CCCCCC'
theme.property['style.type']         = 'fore:#CC77DA'
theme.property['style.variable']     = 'fore:#80CCFF'
theme.property['style.whitespace']   = '$(style.nothing)'
