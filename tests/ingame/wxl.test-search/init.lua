-- The search path, per mod. In the chat box:
--   /run print(TestSearchRemove())   takes the folder out: the probe file is no longer found
--   /run print(TestSearchAdd())      puts it back

local FOLDER = wxl.mod.directory .. "\\files"
local PROBE = "Interface\\wxl-test\\probe.txt"
-- lua-services' folder: another mod's, which this one cannot remove.
local OTHER = wxl.mod.directory:gsub("[^\\/]+$", "") .. "wxl.lua-services\\assets"

local function show(v, why)
    if v == nil then return "nil " .. tostring(why) end
    return tostring(v)
end

local function probe()
    local text, why = wxl.storage.Read(PROBE)
    return text and ("read \"" .. text:gsub("\n", "") .. "\"") or ("not read: " .. tostring(why))
end

print("AddSearchPath: " .. show(wxl.storage.AddSearchPath(FOLDER, 1)))

-- An argument left out is an error naming the function and the argument.
local _, missing = pcall(wxl.storage.AddSearchPath, FOLDER)
print("left out: " .. tostring(missing))
local _, missingNil = pcall(wxl.storage.Read, nil)
print("nil: " .. tostring(missingNil))

local function remove()
    local text = "RemoveSearchPath: " .. show(wxl.storage.RemoveSearchPath(FOLDER)) ..
                 "; again: " .. show(wxl.storage.RemoveSearchPath(FOLDER)) ..
                 "; another mod's folder: " .. show(wxl.storage.RemoveSearchPath(OTHER)) ..
                 "; then the probe is " .. probe()
    print(text)
    return text
end

local function add()
    local text = "AddSearchPath: " .. show(wxl.storage.AddSearchPath(FOLDER, 1)) .. "; then the probe is " .. probe()
    print(text)
    return text
end

if wxl.script then
    wxl.script.RegisterFunction("TestSearchRemove", remove)
    wxl.script.RegisterFunction("TestSearchAdd", add)
end

registerForEvent("onInit", function()
    print("test-search ready: the probe is " .. probe())
end)
