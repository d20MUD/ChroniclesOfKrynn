-- A small profile-local helper for remembering the GUI offer decision.
KrynnPreference = KrynnPreference or {}
local K = KrynnPreference
if K.handler then killAnonymousEventHandler(K.handler) end
local path = getMudletHomeDir() .. "/krynn_gui_preference.txt"
local function stopped()
  local f = io.open(path, "r")
  if not f then return false end
  local value = f:read("*l")
  f:close()
  return value == "stop"
end
local function report()
  sendGMCP('Krynn.Preference ' .. (stopped() and '"stop"' or '"ask"'))
end
K.handler = registerAnonymousEventHandler("gmcp.Krynn.Preference", function()
  local message = gmcp.Krynn.Preference
  if type(message) == "table" and message.stop == true then
    local f = io.open(path, "w")
    if f then f:write("stop\n"); f:close() end
  else
    report()
  end
end)
-- The helper may have been installed after the server's initial request.
report()
