-- lua-hello: the parts of a Lua mod, in one file and one module.
--
-- wxl.lua runs this file from the top when it loads the mod, and again in a fresh state each
-- time a file in this folder changes. Running it only sets things up: the functions it hands
-- over run later, when their event comes.

local Tally = require("lib.tally")

-- State lives in the file's locals: it lasts from one event to the next, and starts over when
-- the mod reloads.
local deaths = Tally.new()

-- onInit runs once the client is fully up, and right after a reload.
registerForEvent("onInit", function()
    print("hello from " .. wxl.mod.name .. " on " .. wxl.runtime.name .. ", abi " .. wxl.runtime.abi)
end)

-- onShutdown runs when the client closes, and before a reload drops this state.
registerForEvent("onShutdown", function()
    print("deaths " .. deaths:summary())
end)

-- A function named after an event, on its family's table, handles that event. It is called as
-- a method, so `self` is ObjectScript; the event's members follow as parameters.
function ObjectScript:OnUnitDeath(unit)
    if not unit then
        return
    end
    -- A read answers its value, or nil and the reason: "gone" once the unit has left.
    local name = unit:Name() or "something"
    local level = unit:Level() or "?"
    print(name .. " died at level " .. level .. " (" .. deaths:add(name) .. " so far)")
end

-- Observe does the same for a function kept anywhere. Here it reports a target's health, or why
-- there is none to report.
Observe("ObjectScript", "OnTargetChanged", function(previous, current)
    if not current then
        print("target cleared")
        return
    end
    local health, why = current:Health()
    if health then
        print("target " .. (current:Name() or "?") .. ": " .. health .. "/" ..
              (current:MaxHealth() or "?") .. " health")
    else
        print("target " .. tostring(current) .. " has no health: " .. why)
    end
end)

-- Observe returns an id, and Unobserve takes it: this handler runs on the first world entered
-- and then removes itself.
local firstWorld
firstWorld = Observe("WorldScript", "OnWorldEnter", function(map_id, position)
    print(string.format("first world: map %d at %.1f, %.1f, %.1f", map_id, position.x,
                        position.y, position.z))
    Unobserve(firstWorld)
end)

-- Override is for an event a handler can answer, and its return value is the answer.
-- OnMessageReceived answers through `suppressed`: true keeps the message from the client.
local SMSG_MOTD = 0x33D
Override("NetScript", "OnMessageReceived", function(opcode, data, suppressed)
    if opcode == SMSG_MOTD and not suppressed then
        print("message of the day hidden (" .. #data .. " bytes)")
        return true
    end
end)

-- A family method on such an event answers the same way. OnFileOpen answers through
-- `redirect`, the path to open instead: a string fills it, nothing leaves the file as asked.
-- `redirect` holds what an earlier handler wrote, "" when none did.
function AssetScript:OnFileOpen(path, redirect)
    if redirect == "" and path:lower() == "interface\\icons\\inv_misc_questionmark.blp" then
        return "Interface\\Icons\\INV_Misc_Coin_01.blp"
    end
end
