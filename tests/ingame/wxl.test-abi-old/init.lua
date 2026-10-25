-- Asks for abi 0.1.5: an event of 0.1.6 or 0.1.7 is refused, with the version to ask for.
local ok, err = pcall(Observe, "ChatScript", "OnChatMessage", function() end)
print("ChatMessage " .. tostring(ok) .. ": " .. tostring(err))
ok, err = pcall(Observe, "ChatScript", "OnChatAddonMessage", function() end)
print("ChatAddonMessage " .. tostring(ok) .. ": " .. tostring(err))
ok = pcall(Observe, "CombatScript", "OnCombatDamage", function() end)
print("CombatDamage (0.1.5) " .. tostring(ok))
