-- The entry file named by mod.json. It runs once when wxl.lua loads the mod, and again every
-- time a file in this folder changes.

local greet = require("lib.greet")

registerForEvent("onInit", function()
    print(greet.hello(wxl.mod.name) .. " on " .. wxl.runtime.name)
end)

registerForEvent("onShutdown", function()
    print("bye")
end)

-- A handler defined on a family table is subscribed once this file has run. It receives the
-- event's parameters, one per member of its args.
function ObjectScript:OnUnitDeath(unit)
    local name = unit:Name() or "something"
    local level = unit:Level() or 0
    print(name .. " died at level " .. level)
end

-- On an event that can be answered, what the handler returns is the answer. The parameter it
-- answers through holds what earlier handlers decided: here, a path another mod redirected to.
function AssetScript:OnFileOpen(path, redirect)
    if redirect == "" and path == "Interface\\Icons\\INV_Misc_QuestionMark.blp" then
        return "Interface\\Icons\\INV_Misc_Coin_01.blp"
    end
end

-- Observe and Override do the same from anywhere, at any time.
local ticks = 0
Observe("WorldScript", "OnUpdate", function(delta)
    ticks = ticks + 1
    if ticks % 600 == 0 then
        print(string.format("%d ticks, last delta %.3f", ticks, delta))
    end
end)

Override("NetScript", "OnMessageReceived", function(opcode, data)
    if opcode == 0x1234 then
        print("opcode 0x1234 with " .. #data .. " bytes, kept from the client")
        return true
    end
end)
