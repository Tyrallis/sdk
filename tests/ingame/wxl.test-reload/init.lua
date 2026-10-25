-- Hot reload, driven from the client's Lua by wxl.selftest:
--   TestReloadPart()          what lib/part.lua returned when this state loaded
--   TestReloadWrite(text)     rewrites lib/part.lua to return text: the mod reloads within a second
--   TestReloadBreak()         keeps mod.json as mod.json.good and writes a broken one: the mod
--                             unloads and stays so; wxl.test-manifest's TestManifestRepair() puts
--                             the good one back, and the mod loads again
-- Every state loaded says so in the log, with its part.

local part = require("lib.part")

local function write(name, text)
    local file, why = io.open(name, "wb")
    if not file then error(name .. ": " .. tostring(why)) end
    file:write(text)
    file:close()
end

local function read(name)
    local file, why = io.open(name, "rb")
    if not file then error(name .. ": " .. tostring(why)) end
    local text = file:read("*a")
    file:close()
    return text
end

if wxl.script then
    wxl.script.RegisterFunction("TestReloadPart", function() return part end)

    wxl.script.RegisterFunction("TestReloadWrite", function(text)
        text = tostring(text)
        write("lib/part.lua", "-- Rewritten by TestReloadWrite(): a change here reloads wxl.test-reload.\n" ..
                              "return " .. string.format("%q", text) .. "\n")
        return text
    end)

    -- The broken manifest is a JSON syntax error: the runtime refuses it with its line.
    wxl.script.RegisterFunction("TestReloadBreak", function()
        write("mod.json.good", read("mod.json"))
        write("mod.json", '{\n  "id": "wxl.test-reload",\n  "broken on purpose by TestReloadBreak"\n}\n')
        return true
    end)
end

registerForEvent("onInit", function() print("test-reload loaded: " .. part) end)
