-- lua-hello: the parts of a Lua mod, in one file and one module.
--
-- wxl.lua runs this file from the top when it loads the mod, and again in a fresh state each
-- time a file in this folder changes. Running it only sets things up: the functions it hands
-- over run later, when their event comes.

local Tally = require("lib.tally")

-- State lives in the file's locals: it lasts from one event to the next, and starts over when
-- the mod reloads.
local deaths = Tally.new()
local icons = 0

-- onInit runs once the client is fully up, and right after a reload.
registerForEvent("onInit", function()
    print("hello from " .. wxl.mod.name .. " on " .. wxl.runtime.name .. ", abi " .. wxl.runtime.abi)
end)

-- onShutdown runs when the client closes, and before a reload drops this state.
registerForEvent("onShutdown", function()
    print("deaths " .. deaths:summary() .. ", " .. icons .. " icon(s) opened")
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

-- A file is a different case. The client opens most of its files on its loading threads, and a
-- Lua handler only ever runs on the main thread: an event raised elsewhere reaches it as a copy,
-- at the next update, too late to answer. So swapping a file is data the runtime holds and
-- answers with on any thread: wxl.storage.Redirect. Every open of the first name, from here on,
-- opens the second; it lasts until the mod takes it back with Unredirect, or goes.
local redirected, why = wxl.storage.Redirect("Interface\\Icons\\INV_Misc_QuestionMark.blp",
                                             "Interface\\Icons\\INV_Misc_Coin_01.blp")
if not redirected then
    print("the question mark icon is not redirected: " .. why)
end

-- OnFileOpen still tells which files the client opens, from whatever thread: an Observe sees
-- each one, a moment later. Here it counts the icons, reported when the client closes.
Observe("AssetScript", "OnFileOpen", function(path)
    if path:lower():find("^interface\\icons\\") then
        icons = icons + 1
    end
end)
