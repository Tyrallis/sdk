-- selftest: the in-game checks a person would otherwise type one by one. On entering the world it
-- runs, from the client's own Lua, everything below in order, without a keystroke:
--
--   1. calls every function the test mods register and checks each answer;
--   2. whispers the player, sends an addon message and changes a CVar, and checks the events;
--   3. hot reload: makes wxl.test-reload rewrite a file of its own and checks it reloaded; checks
--      that wxl.test-order-a still runs before wxl.test-order-b after it reloads; breaks
--      wxl.test-reload's mod.json (the mod unloads) and has wxl.test-manifest repair it (it loads);
--   4. provokes events a session rarely shows (PROVOKE): a cast refused, a cast cancelled, a
--      racial's cooldown, a hearthstone use refused, an item taken off and put back on;
--   5. reloads the interface and calls the functions again;
--   6. logs out and back in with the same character (RELOG) and calls them again.
--
-- Each check is one line in the log: PASS, FAIL, SKIP (what it needs is not there: a class
-- without a cast-time spell, say) or NOTE (an event the server may not send). The chat box shows
-- the totals at the end.

-- What the run does besides calling functions. Off, the part is skipped with a SKIP line.
local PROVOKE = true -- casts, a hearthstone use, an item taken off and put back on
local RELOG   = true -- logs out and back in at the end: do not move during the logout

local MARKER = "wxl selftest"

-- --- what each call must answer ----------------------------------------------------------
-- A check takes the call's pcall result and returns true, or false and what was expected.

local function equals(want) return function(ok, a) return ok and a == want, string.format("%q", want) end end
local function contains(want)
    return function(ok, a) return ok and type(a) == "string" and a:find(want, 1, true) ~= nil, "text with " .. want end
end
local function number(ok, a) return ok and tonumber(a) ~= nil, "a number" end
local function fails(want)
    return function(ok, a) return not ok and tostring(a):find(want, 1, true) ~= nil, "an error with " .. want end
end
local function answers(ok) return ok, "an answer, not an error" end

-- name, the client Lua that calls it, its check.
local CALLS = {
    { "ScoutCount", "return ScoutCount()", number },
    { "ScoutEntry", "return ScoutEntry()", answers },
    { "ScoutNearby", "return ScoutNearby()", number },
    { "ScoutTraffic received", "return ScoutTraffic(0xA9)", number },
    { "ScoutTraffic sent", "return ScoutTraffic(0x95, 'sent')", number },
    { "ScoutEntry(0) refused", "return ScoutEntry(0)", fails("") },
    { "ScoutDrop on", "return ScoutDrop(0x105, true)", answers },
    { "ScoutDrop off", "return ScoutDrop(0x105, false)", answers },
    { "LuaServicesGreet", "return LuaServicesGreet('you')", equals("hello you, from Lua services") },
    { "LuaServicesGreet()", "return LuaServicesGreet()", equals("hello nil, from Lua services") },
    { "TestThrow contained", "return TestThrow()", fails("thrown on purpose from TestThrow") },
    { "TestFields", "return TestFields()", contains("Health=") },
    { "TestSearchRemove", "return TestSearchRemove()", contains("RemoveSearchPath: true") },
    { "TestSearchAdd", "return TestSearchAdd()", contains("AddSearchPath: true") },
    { "MemoryReport", "return MemoryReport()", answers },
    { "WxlProbeStep", "return WxlProbeStep()", answers },
}

-- After a /reload, and after logging back in: the functions are registered again in the new
-- interface.
local AGAIN = {
    { "LuaServicesGreet", "return LuaServicesGreet('you')", equals("hello you, from Lua services") },
    { "TestThrow", "return TestThrow()", fails("thrown on purpose from TestThrow") },
    { "ScoutCount", "return ScoutCount()", number },
    { "TestFieldsHealth", "return TestFieldsHealth()", answers },
    { "TestReloadPart", "return TestReloadPart()", answers },
    { "TestOrderMark", "return TestOrderMark()", answers },
}

-- The first rank of a spell with a cast time that a class has from level 1, cast on the player:
-- Holy Light, Lesser Heal, Healing Wave, Healing Touch, Summon Imp.
local CAST_SPELLS = { 635, 2050, 331, 5185, 688 }

-- Racials with a cooldown that only touch the player: Every Man for Himself, Stoneform, Escape
-- Artist, Blood Fury (three), Will of the Forsaken, Berserking, Gift of the Naaru (one per class).
-- War Stomp, Arcane Torrent and Shadowmeld are left out: they reach the units around, or hide
-- the player.
local RACIALS = { 59752, 20594, 20589, 20572, 33697, 33702, 7744, 26297,
                  28880, 59542, 59543, 59544, 59545, 59547, 59548 }

local HEARTHSTONE, HEARTHSTONE_SPELL = 6948, 8690

-- The equipment slots tried, in order: shirt, feet, wrists, waist, legs, chest.
local SLOTS = { 4, 8, 9, 6, 7, 5 }

-- --- results ---------------------------------------------------------------------------------

local passed, failed, skipped = 0, 0, 0

local function result(name, ok, detail)
    if ok then passed = passed + 1 else failed = failed + 1 end
    print(string.format("%s %s%s", ok and "PASS" or "FAIL", name, detail and (": " .. detail) or ""))
end

local function skip(name, why)
    skipped = skipped + 1
    print("SKIP " .. name .. ": " .. why)
end

-- An event the server may or may not send: seen is a PASS, unseen a NOTE, never a FAIL.
local function note(name, seen, why)
    if seen then
        result(name, true, "raised")
    else
        print("NOTE " .. name .. ": not raised; " .. why)
    end
end

-- --- the client's Lua ------------------------------------------------------------------------

local answered = {}

-- The client's Lua answers each call here: a name, whether pcall succeeded, and two values.
wxl.script.RegisterFunction("WxlSelfTestAnswer", function(name, ok, a, b)
    answered[name] = { ok = ok, a = a, b = b }
end, "game")

local function run(code) wxl.script.Run(code, "wxl.selftest") end

-- Runs `code` in the client's Lua and answers what it returned: true and its first two values,
-- or false and the error. In the world only: the login screens have no WxlSelfTestAnswer.
local function ask(code)
    answered.ask = nil
    run(string.format("WxlSelfTestAnswer('ask', pcall(loadstring(%q)))", code))
    local got = answered.ask
    if not got then return false, "the client never answered" end
    return got.ok, got.a, got.b
end

local function chat(text)
    run("DEFAULT_CHAT_FRAME:AddMessage(" .. string.format("%q", "|cff40ff40wxl.selftest|r " .. text) .. ")")
end

local function call(list, suffix)
    for _, entry in ipairs(list) do
        local ok, a, b = ask(entry[2])
        local good, expected = entry[3](ok, a, b)
        local got = (ok and "answered " or "failed with ") .. tostring(a) .. (b ~= nil and (", " .. tostring(b)) or "")
        result(entry[1] .. (suffix or ""), good, good and got or (got .. "; expected " .. expected))
    end
end

-- --- the events, as the runtime reports them ---------------------------------------------------

-- Event name -> the argument lists seen since `watch(name)`; an event not watched keeps nothing.
local seen = {}

local function watch(...)
    for i = 1, select("#", ...) do seen[(select(i, ...))] = {} end
end

-- Whether `name` came since it was watched, with arguments `test` accepts when given.
local function saw(name, test)
    for _, args in ipairs(seen[name] or {}) do
        if not test or test(args) then return true end
    end
    return false
end

local function keep(family, name)
    Observe(family, "On" .. name, function(...)
        local list = seen[name]
        if list then list[#list + 1] = { ... } end
    end)
end

for family, names in pairs({
    ChatScript   = { "ChatAddonMessage", "ChatSend", "ChatMessage" },
    WorldScript  = { "CVarChanged", "WorldEnter", "WorldLeave" },
    ObjectScript = { "EquipmentChanged", "SpellCastStart", "SpellCastGo", "SpellCastFailed",
                     "SpellCastInterrupted", "SpellCastCancel", "SpellCooldown", "ItemUseRequest",
                     "SpellCastRequest" },
    GlueScript   = { "GlueScreenChanged", "GlueCharacterList", "GlueEnterWorld" },
}) do
    for _, name in ipairs(names) do keep(family, name) end
end

-- Any argument equal to `text`.
local function carries(text)
    return function(args)
        for i = 1, table.maxn(args) do
            if tostring(args[i]) == text then return true end
        end
        return false
    end
end

-- The spell id at argument `index` is `id`.
local function spell(index, id) return function(args) return args[index] == id end end

-- What the run refuses while a check is armed: a spell id, and any item use.
local refuseSpell, refuseItems = nil, false

Override("ObjectScript", "OnSpellCastRequest", function(caster, spell_id)
    if refuseSpell and spell_id == refuseSpell then return true end
    return nil
end)

Override("ObjectScript", "OnItemUseRequest", function()
    if refuseItems then return true end
    return nil
end)

-- --- the run: a queue of steps, one per tick at most -----------------------------------------

local steps, cursor = {}, 1
local delay = 0
local condition, deadline, poll, onTimeout

local function step(fn) steps[#steps + 1] = fn end

-- The next step waits `seconds`.
local function wait(seconds) delay = seconds end

-- The next step waits until `test()` is true, asked every quarter second, or `seconds` have
-- passed; then `timeout()` runs, when given.
local function waitFor(test, seconds, timeout)
    condition, deadline, poll, onTimeout = test, seconds, 0, timeout
end

local entered, started = 0, false

Observe("LuaScript", "OnUiEvent", function(_, name)
    if name ~= "PLAYER_ENTERING_WORLD" then return end
    entered = entered + 1
    if not started then
        started = true
        wait(3)
    end
end)

registerForEvent("onUpdate", function(delta)
    if not started then return end
    if condition then
        deadline = deadline - delta
        poll = poll - delta
        if poll > 0 and deadline > 0 then return end
        poll = 0.25
        if condition() then
            condition = nil
        elseif deadline <= 0 then
            condition = nil
            if onTimeout then onTimeout() end
        else
            return
        end
    end
    if delay > 0 then
        delay = delay - delta
        if delay > 0 then return end
    end
    local fn = steps[cursor]
    if not fn then return end
    cursor = cursor + 1
    fn()
end)

-- 1. The functions -----------------------------------------------------------------------------

step(function()
    chat("running: about two minutes; the interface reloads once and the character logs out " ..
         "and back in by itself, do not move")
    print("phase 1: the functions the test mods register")
    call(CALLS)
end)

-- 2. Chat, addon message, CVar ----------------------------------------------------------------

step(function()
    print("phase 2: chat, an addon message and a CVar")
    watch("ChatAddonMessage", "ChatSend", "ChatMessage", "CVarChanged")
    run([[
local me = UnitName("player")
SendAddonMessage("WXL", "ping wxl selftest", "WHISPER", me)
SendChatMessage("wxl selftest", "WHISPER", nil, me)
local farclip = GetCVar("farclip")
SetCVar("farclip", farclip == "500" and "600" or "500")
SetCVar("farclip", farclip)
]])
    wait(5)
end)

step(function()
    result("event ChatAddonMessage", saw("ChatAddonMessage", carries("ping " .. MARKER)))
    result("event ChatSend", saw("ChatSend", carries(MARKER)))
    result("event ChatMessage", saw("ChatMessage", carries(MARKER)))
    result("event CVarChanged", saw("CVarChanged", carries("farclip")))
end)

-- 3. Hot reload -----------------------------------------------------------------------------------

local function part() return ask("return TestReloadPart()") end

local originalPart, newPart

step(function()
    print("phase 3: hot reload")
    local ok, current = part()
    if not ok then
        result("TestReloadPart", false, "failed with " .. tostring(current) .. "; is wxl.test-reload in Mods/?")
        return
    end
    originalPart, newPart = current, "part " .. os.time()
    local written, text = ask(string.format("return TestReloadWrite(%q)", newPart))
    result("TestReloadWrite", written and text == newPart, tostring(text))
    waitFor(function()
        local loaded, now = part()
        return loaded and now == newPart
    end, 5)
end)

step(function()
    if not newPart then return end
    local ok, now = part()
    result("reload after a file of its own changed", ok and now == newPart,
           "lib/part.lua now gives " .. tostring(now) .. ((ok and now == newPart) and "" or (", expected " .. newPart)))
end)

-- Order: wxl.test-order-a answers "wxl order <n>" before wxl.test-order-b sees it.
local function order(n)
    run(string.format("SendChatMessage(%q, 'WHISPER', nil, UnitName('player'))", "wxl order " .. n))
end

local markBefore, markAfter

step(function()
    order(1)
    local ok, first = ask("return TestOrderSeen(1)")
    result("handler order: wxl.test-order-a before wxl.test-order-b", ok and first == true,
           ok and ("b saw a's answer: " .. tostring(first)) or tostring(first))
    local known, touched
    known, markBefore = ask("return TestOrderMark()")
    touched, markAfter = ask(string.format("return TestOrderTouch(%q)", "a" .. os.time()))
    if not (known and touched) then
        result("TestOrderTouch", false, tostring(markAfter))
        markAfter = nil
        return
    end
    waitFor(function()
        local loaded, now = ask("return TestOrderMark()")
        return loaded and now == markAfter
    end, 5)
end)

step(function()
    if not markAfter then return end
    local ok, now = ask("return TestOrderMark()")
    result("wxl.test-order-a reloaded after mark.lua changed", ok and now == markAfter,
           tostring(markBefore) .. " -> " .. tostring(now))
    order(2)
    local seenOk, second = ask("return TestOrderSeen(2)")
    result("handler order kept after wxl.test-order-a reloaded", seenOk and second == true,
           seenOk and ("b saw a's answer: " .. tostring(second)) or tostring(second))
    -- Leaves mark.lua as it was before the run.
    if markBefore then ask(string.format("return TestOrderTouch(%q)", markBefore)) end
end)

-- A broken mod.json unloads the mod; repaired, it loads again.
local broken

local function unloaded()
    local running, failure = part()
    return not running and tostring(failure):find("not running", 1, true) ~= nil, running, failure
end

step(function()
    local ok, done = ask("return TestReloadBreak()")
    broken = ok and done == true
    result("TestReloadBreak", broken, tostring(done))
    if broken then waitFor(unloaded, 5) end
end)

step(function()
    if not broken then return end
    local gone, running, failure = unloaded()
    result("a broken mod.json unloads the mod", gone,
           running and ("still answers " .. tostring(failure)) or tostring(failure))
    -- Two more seconds: it stays unloaded while the file stays broken.
    wait(2)
end)

step(function()
    if not broken then return end
    result("it stays unloaded while mod.json is broken", (unloaded()))
    local ok, repaired, what = ask("return TestManifestRepair()")
    result("TestManifestRepair", ok and repaired == true,
           ok and tostring(what) or (tostring(repaired) .. "; is wxl.test-manifest in Mods/?"))
    waitFor(function() return (part()) end, 5)
end)

step(function()
    if not broken then return end
    local ok, now = part()
    result("the repaired mod.json loads the mod again", ok, ok and ("answers " .. tostring(now)) or tostring(now))
    -- Leaves lib/part.lua as it was before the run.
    if ok and originalPart and now ~= originalPart then
        ask(string.format("return TestReloadWrite(%q)", originalPart))
    end
end)

-- 4. Events a session rarely shows ----------------------------------------------------------------

local inCombat, castSpell, castName

-- The first spell of `list` the player knows: its id and name.
local function known(list)
    local ids = {}
    for i, id in ipairs(list) do ids[i] = tostring(id) end
    return ask("for _, id in ipairs({" .. table.concat(ids, ", ") .. "}) do " ..
               "if IsSpellKnown(id) then return id, (GetSpellInfo(id)) end end return nil")
end

-- The name of the spell the player is casting, or nil.
local function casting()
    local _, name = ask("return UnitCastingInfo('player')")
    return name
end

step(function()
    print("phase 4: events a session rarely shows")
    if not PROVOKE then return end
    local _, combat = ask("return (UnitAffectingCombat('player') or InCombatLockdown()) and true or false")
    inCombat = combat == true
    local _, id, name = known(CAST_SPELLS)
    castSpell, castName = id, name
    if not castSpell then return end
    -- A cast refused by a handler never starts.
    watch("SpellCastRequest")
    refuseSpell = castSpell
    run(string.format("CastSpellByID(%d, 'player')", castSpell))
    wait(1)
end)

step(function()
    if not PROVOKE then
        skip("events", "PROVOKE is off at the top of wxl.selftest/init.lua")
        return
    end
    if not castSpell then
        skip("casts", "this character knows none of the cast-time spells tried (a paladin's, priest's, " ..
                      "shaman's, druid's or warlock's first)")
        return
    end
    refuseSpell = nil
    result("event SpellCastRequest (" .. tostring(castName) .. ")", saw("SpellCastRequest", spell(2, castSpell)))
    local name = casting()
    result("a cast a handler refuses does not start", name == nil, name and ("casting " .. name) or "no cast")
    if name then run("SpellStopCasting()") end
    wait(2)
end)

local castStarted

step(function()
    if not (PROVOKE and castSpell) then return end
    -- A cast the player cancels: the client's own SpellCastCancel.
    watch("SpellCastStart", "SpellCastCancel", "SpellCastInterrupted", "SpellCastFailed")
    run(string.format("CastSpellByID(%d, 'player')", castSpell))
    waitFor(function() return casting() ~= nil end, 3)
end)

step(function()
    if not (PROVOKE and castSpell) then return end
    castStarted = casting() ~= nil
    run("SpellStopCasting()")
    waitFor(function() return saw("SpellCastCancel", spell(3, castSpell)) end, 2)
end)

step(function()
    if not (PROVOKE and castSpell) then return end
    if not castStarted then
        skip("cast cancelled", castName .. " did not start (out of mana, moving, or refused by the server)")
    else
        result("event SpellCastStart (" .. castName .. ")", saw("SpellCastStart", spell(6, castSpell)))
        result("event SpellCastCancel (" .. castName .. ")", saw("SpellCastCancel", spell(3, castSpell)))
        note("event SpellCastInterrupted (" .. castName .. ")", saw("SpellCastInterrupted", spell(3, castSpell)),
             "the server answers a cancel with SMSG_SPELL_FAILURE only when the cast reached it")
    end
    if casting() then run("SpellStopCasting()") end
    -- The global cooldown of the cast passes.
    wait(2)
end)

-- A racial's cooldown, and the client refusing the same cast while it runs.
local racial, racialName, racialWent

step(function()
    if not PROVOKE then return end
    local _, id, name = known(RACIALS)
    racial, racialName = id, name
    if not racial then
        skip("cooldown", "this character knows none of the racials tried (War Stomp, Arcane Torrent " ..
                         "and Shadowmeld are left out on purpose)")
        return
    end
    local _, start = ask(string.format("return GetSpellCooldown(%d)", racial))
    if (tonumber(start) or 0) > 0 then
        skip("cooldown", racialName .. " is still cooling down from an earlier run")
        racial = nil
        return
    end
    watch("SpellCastGo", "SpellCooldown", "SpellCastFailed")
    run(string.format("CastSpellByID(%d, 'player')", racial))
    waitFor(function() return saw("SpellCastGo", spell(6, racial)) end, 3)
end)

step(function()
    if not (PROVOKE and racial) then return end
    racialWent = saw("SpellCastGo", spell(6, racial))
    result("event SpellCastGo (" .. racialName .. ")", racialWent)
    if not racialWent then return end
    local _, start, duration = ask(string.format("return GetSpellCooldown(%d)", racial))
    result("the client holds " .. racialName .. "'s cooldown",
           (tonumber(start) or 0) > 0 and (tonumber(duration) or 0) > 1.5,
           "start " .. tostring(start) .. ", duration " .. tostring(duration))
    run(string.format("CastSpellByID(%d, 'player')", racial))
    waitFor(function() return saw("SpellCastFailed", spell(3, racial)) end, 2)
end)

step(function()
    if not (PROVOKE and racial and racialWent) then return end
    result("event SpellCastFailed: a cast during its cooldown", saw("SpellCastFailed", spell(3, racial)))
    note("event SpellCooldown (" .. racialName .. ")", saw("SpellCooldown", spell(5, racial)),
         "the server sends SMSG_SPELL_COOLDOWN for some casts of a player only")
    wait(1)
end)

-- An item use a handler refuses: the hearthstone, which never starts its cast. Its spell is
-- refused too, and any cast is stopped, should the refusal not hold.
local hearthBag, hearthCast

step(function()
    if not PROVOKE then return end
    local _, bag, slot = ask(string.format(
        "for bag = 0, 4 do for slot = 1, GetContainerNumSlots(bag) do " ..
        "if GetContainerItemID(bag, slot) == %d then return bag, slot end end end return nil", HEARTHSTONE))
    hearthBag = bag
    if not hearthBag then
        skip("item use", "no hearthstone in the bags")
        return
    end
    watch("ItemUseRequest")
    refuseItems, refuseSpell = true, HEARTHSTONE_SPELL
    local _, started = ask(string.format(
        "UseContainerItem(%d, %d) if UnitCastingInfo('player') then SpellStopCasting() return true end return false",
        bag, slot))
    hearthCast = started == true
    wait(1)
end)

step(function()
    if not (PROVOKE and hearthBag) then return end
    local name = casting()
    if name then run("SpellStopCasting()") end
    refuseItems, refuseSpell = false, nil
    result("event ItemUseRequest (hearthstone)", saw("ItemUseRequest"))
    result("an item use a handler refuses does not cast", not hearthCast and name == nil,
           (hearthCast or name) and "a cast started and was stopped" or "no cast")
end)

-- An item taken off and put back on: EquipmentChanged twice. Out of combat only, where the
-- client lets armour change.
local wornSlot, wornLink, wornId, takenOff

local function emptied(args) return args[3] == 0 end
local function filled(args) return args[3] == wornId end

step(function()
    if not PROVOKE then return end
    if inCombat then
        skip("equipment", "in combat, where armour cannot change")
        return
    end
    local slots = {}
    for i, slot in ipairs(SLOTS) do slots[i] = tostring(slot) end
    local _, slot, link = ask("for _, slot in ipairs({" .. table.concat(slots, ", ") .. "}) do " ..
                              "local link = GetInventoryItemLink('player', slot) " ..
                              "if link then return slot, link end end return nil")
    if not slot then
        skip("equipment", "nothing worn in the shirt, feet, wrist, waist, legs or chest slot")
        return
    end
    wornSlot, wornLink, wornId = slot, link, tonumber(tostring(link):match("item:(%d+)"))
    watch("EquipmentChanged")
    local _, what = ask(string.format(
        "PickupInventoryItem(%d) PutItemInBackpack() " ..
        "if CursorHasItem() then PickupInventoryItem(%d) return 'kept' end return 'off'", slot, slot))
    takenOff = what == "off"
    if not takenOff then
        skip("equipment", "the backpack has no room: the item stays on")
        return
    end
    waitFor(function() return saw("EquipmentChanged", emptied) end, 3)
end)

step(function()
    if not (PROVOKE and takenOff) then return end
    result("event EquipmentChanged: slot " .. wornSlot .. " emptied", saw("EquipmentChanged", emptied))
    run(string.format("EquipItemByName(%q)", wornLink))
    waitFor(function() return saw("EquipmentChanged", filled) end, 3)
end)

local function wornAgain()
    local _, back = ask(string.format("return GetInventoryItemLink('player', %d) == %q", wornSlot, wornLink))
    return back == true
end

step(function()
    if not (PROVOKE and takenOff) then return end
    result("event EquipmentChanged: slot " .. wornSlot .. " filled", saw("EquipmentChanged", filled))
    waitFor(wornAgain, 3)
end)

step(function()
    if not (PROVOKE and takenOff) then return end
    -- The chat box reads | as an escape; the log shows the link as the client wrote it.
    local back = wornAgain()
    result("the item is worn again", back, back and wornLink or ("put " .. wornLink .. " back on by hand"))
end)

-- 5. /reload ------------------------------------------------------------------------------------

local enteredBefore

step(function()
    print("phase 5: /reload")
    enteredBefore = entered
    run("ReloadUI()")
    waitFor(function() return entered > enteredBefore end, 60,
            function() result("/reload", false, "the world was not entered again within 60 s") end)
end)

step(function() wait(3) end)

step(function()
    if entered > enteredBefore then call(AGAIN, " after /reload") end
end)

-- 6. Log out and back in ---------------------------------------------------------------------------

local relogging

step(function()
    print("phase 6: log out and back in")
    if not RELOG then
        skip("log out and back in", "RELOG is off at the top of wxl.selftest/init.lua")
        return
    end
    relogging = true
    chat("logging out and back in by itself: do not move")
    watch("WorldLeave", "GlueScreenChanged", "GlueCharacterList", "GlueEnterWorld", "WorldEnter")
    run("Logout()")
    waitFor(function() return saw("GlueCharacterList") end, 45, function()
        relogging = false
        result("log out", false, "the character list did not come within 45 s: in combat, moved, or the " ..
                                 "logout was refused; log in again by hand if needed")
    end)
end)

step(function()
    if not relogging then return end
    result("event WorldLeave", saw("WorldLeave"))
    result("event GlueScreenChanged to charselect", saw("GlueScreenChanged", carries("charselect")))
    result("event GlueCharacterList", saw("GlueCharacterList"))
    wait(3)
end)

step(function()
    if not relogging then return end
    enteredBefore = entered
    -- The login screens' Lua: the character selected is the one that just left.
    run("EnterWorld()")
    waitFor(function() return entered > enteredBefore end, 90, function()
        relogging = false
        result("back in the world", false, "not in the world 90 s after EnterWorld(); enter by hand")
    end)
end)

step(function()
    if not relogging then return end
    result("event GlueEnterWorld", saw("GlueEnterWorld"))
    result("event WorldEnter", saw("WorldEnter"))
    wait(3)
end)

step(function()
    if relogging then call(AGAIN, " after logging back in") end
end)

-- The end ----------------------------------------------------------------------------------------

step(function()
    local summary = string.format("done: %d passed, %d failed, %d skipped", passed, failed, skipped)
    print(summary)
    chat(summary .. (failed == 0 and "" or " (the log says which)"))
end)

print("ready: the checks start a few seconds after entering the world")
