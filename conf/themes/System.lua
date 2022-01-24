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

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling

if theme.dark then
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
  theme.property['style.calltip']      = 'fore:#AAB2BE,back:#333333'
  theme.property['style.class']        = 'fore:#F6E9D0'
  theme.property['style.comment']      = 'fore:#E2D9C9'
  theme.property['style.constant']     = 'fore:#E8C080'
  theme.property['style.controlchar']  = '$(style.nothing)'
  -- theme.property['style.default']      = 'fore:#AAB2BE,back:#212228'
  theme.property['style.definition']   = 'fore:#F6E9D0'
  theme.property['style.embedded']     = '$(style.tag),back:#333333'
  theme.property['style.error']        = 'fore:#994D4D'
  theme.property['style.function']     = 'fore:#4D99E6'
  theme.property['style.identifier']   = '$(style.nothing)'
  theme.property['style.indentguide']  = 'fore:#333333,back:#333333'
  theme.property['style.keyword']      = 'fore:#53AFEC,bold'
  theme.property['style.label']        = 'fore:#E8C080'
  -- theme.property['style.linenumber']   = 'fore:#5F6672,back:#2A2B30,bold'
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
else
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
  -- theme.property['style.default']      = 'fore:$(color.black),back:$(color.white)'
  theme.property['style.definition']   = '$(style.nothing)'
  theme.property['style.embedded']     = '$(style.nothing)'
  theme.property['style.error']        = 'fore:#FF0000'
  theme.property['style.function']     = '$(style.label)'
  theme.property['style.identifier']   = '$(style.nothing)'
  theme.property['style.indentguide']  = '$(style.nothing)'
  theme.property['style.keyword']      = 'fore:$(color.blue),bold'
  theme.property['style.label']        = 'fore:$(color.red)'
  -- theme.property['style.linenumber']   = 'fore:$(color.black),back:#DCDCDC'
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
end

