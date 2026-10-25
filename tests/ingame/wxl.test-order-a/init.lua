-- Handler order across a reload. Mods run in the order they loaded (folder order here: a before
-- b), and a mod that reloads keeps its place. This one answers the chat messages wxl.selftest
-- sends ("wxl order <n>") by keeping them from being sent; wxl.test-order-b, which runs after it,
-- sees that answer. TestOrderTouch() rewrites mark.lua, a file of this mod's own, which reloads it.
--
-- By hand: change the text in mark.lua and save, then target something: "order a <mark>" must
-- still come before "order b" in the log.

local mark = require("mark")
local MESSAGE = "^wxl order %d+$"

Override("ChatScript", "OnChatSend", function(text, type, language, target, prefix, addon, suppressed)
    if addon or not text:find(MESSAGE) then return nil end
    print("order a " .. mark .. ": " .. text .. ", answered so far " .. tostring(suppressed))
    return true
end)

Observe("ObjectScript", "OnTargetChanged", function() print("order a " .. mark) end)

if wxl.script then
    -- The mark this state was loaded with: a new one after a reload.
    wxl.script.RegisterFunction("TestOrderMark", function() return mark end)

    -- Writes mark.lua with a new mark; the runtime sees the change within a second and reloads
    -- this mod. Answers the mark written.
    wxl.script.RegisterFunction("TestOrderTouch", function(text)
        text = tostring(text or "a" .. os.time())
        local file, why = io.open("mark.lua", "w")
        if not file then error("mark.lua: " .. tostring(why)) end
        file:write("-- Rewritten by TestOrderTouch(): a change here reloads wxl.test-order-a.\n")
        file:write("return " .. string.format("%q", text) .. "\n")
        file:close()
        return text
    end)
end
