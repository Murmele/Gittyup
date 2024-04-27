local family = "Courier New"
local size = 10
if platform == "mac" then
  family = "Menlo"
  size = 12
elseif platform == "win" then
  family = "Consolas"
elseif platform == "x11" then
  family = "Monospace"
end

return {
  font = {
    family = family,
    size = size
  },
  view = {
    showWhitespace = false
  },
  indent = {
    tabs = true,
    width = 4,
    tabwidth = 4
  },
  blame = {
    heatmap = true
  }
}
