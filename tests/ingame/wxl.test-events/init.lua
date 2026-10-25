-- Logs the events merged on dev on 2026-10-25, one line each, and follows the identity of the
-- current target. Every line in Logs/WarcraftXL.log is tagged [wxl.test-events].

local part = require("lib.part")

local function show(v)
    if type(v) == "table" and v.x then
        return string.format("(%.1f, %.1f, %.1f)", v.x, v.y, v.z)
    elseif type(v) == "string" then
        return string.format("%q", v)
    end
    return tostring(v)
end

local function line(name, ...)
    local parts = {}
    for i = 1, select("#", ...) do
        parts[i] = show((select(i, ...)))
    end
    print(name .. " " .. table.concat(parts, ", "))
end

-- Events that fire often are logged at most once per second each.
local quiet = { SpellChannelUpdate = true, MouseoverChanged = true, PlayerMoveStart = true,
                PlayerMoveStop = true, CombatAttackStart = true, CombatAttackStop = true }
local last = {}
local clock = 0

local events = {
    { "Object", { "SpellChannelStart", "SpellChannelUpdate", "SpellChannelStop", "SpellCastResume",
                  "SpellCastDelayed", "SpellCooldown", "SpellCooldownChanged", "SpellCastCancel" } },
    { "Combat", { "CombatAttackStart", "CombatAttackStop", "CombatDamage", "CombatHeal", "CombatMiss",
                  "CombatEnergize" } },
    { "World",  { "ZoneChanged", "MouseoverChanged", "CVarChanged" } },
    { "Player", { "PlayerMoveStart", "PlayerMoveStop", "PlayerLevelUp", "PlayerXpGain",
                  "PlayerMoneyChanged" } },
    { "Glue",   { "GlueScreenChanged", "GlueCharacterList", "GlueCharacter", "GlueRealmList", "GlueRealm",
                  "GlueAuthResult", "GlueEnterWorld" } },
    { "Chat",   { "ChatMessage", "ChatSend", "ChatAddonMessage" } },
}

for _, family in ipairs(events) do
    for _, name in ipairs(family[2]) do
        Observe(family[1] .. "Script", "On" .. name, function(...)
            if quiet[name] then
                if last[name] and clock - last[name] < 1 then return end
                last[name] = clock
            end
            line(name, ...)
        end)
    end
end

-- Identity: the current target keeps its fields while in view; it is gone after it leaves.
local held, heldGuid
Observe("ObjectScript", "OnTargetChanged", function(previous, current)
    if not current then return end
    current.seen = (current.seen or 0) + 1
    local guid = current:Guid()
    print("identity target " .. tostring(current) .. " seen " .. current.seen ..
          " same at FromGuid " .. tostring(rawequal(wxl.object.FromGuid(guid), current)))
    held, heldGuid = current, guid
end)

Observe("ObjectScript", "OnObjectDestroy", function(object, guid)
    if guid == heldGuid then
        print("identity destroy " .. tostring(object) .. " seen " .. tostring(object.seen) ..
              " valid " .. tostring(object:Valid()))
    end
end)

Observe("ObjectScript", "OnObjectCreate", function(object, guid)
    if guid == heldGuid and held and not rawequal(object, held) then
        print("identity back " .. tostring(object) .. " new value true seen " .. tostring(object.seen) ..
              " old one valid " .. tostring(held:Valid()))
    end
end)

local reported
registerForEvent("onUpdate", function(delta)
    clock = clock + delta
    if held and not held:Valid() and reported ~= held then
        reported = held
        local value, why = held:Health()
        print("identity gone " .. tostring(held) .. " health " .. tostring(value) .. " " .. tostring(why) ..
              " seen " .. tostring(held.seen))
    end
end)

registerForEvent("onInit", function()
    -- A file the mod writes into its own folder never reloads it.
    local notes = io.open("notes.txt", "w")
    if notes then
        notes:write("written at init\n")
        notes:close()
    end
    print("test-events ready: " .. part .. ", abi " .. wxl.runtime.abi .. " on " .. wxl.runtime.name)
end)
