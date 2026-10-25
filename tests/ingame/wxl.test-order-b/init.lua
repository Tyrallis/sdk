-- Runs after wxl.test-order-a on the same event. For each "wxl order <n>" chat message
-- wxl.selftest sends, it keeps whether wxl.test-order-a had already answered it: true means a ran
-- first. TestOrderSeen(n) answers that for message n.

local MESSAGE = "^wxl order (%d+)$"
local seen = {}

Observe("ChatScript", "OnChatSend", function(text, type, language, target, prefix, addon, suppressed)
    local n = not addon and text:match(MESSAGE)
    if not n then return end
    seen[tonumber(n)] = suppressed == true
    print("order b: " .. text .. ", answered by wxl.test-order-a first " .. tostring(suppressed == true))
end)

Observe("ObjectScript", "OnTargetChanged", function() print("order b") end)

if wxl.script then
    -- true: wxl.test-order-a answered message n before this mod saw it; false: it did not;
    -- nil: message n never reached this mod.
    wxl.script.RegisterFunction("TestOrderSeen", function(n) return seen[tonumber(n)] end)
end
