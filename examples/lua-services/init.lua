-- lua-services: the client's services from Lua -- its files, its databases, its own script and
-- the hook points it names. Every function of a wxl.<service> module answers its value, or nil
-- and the reason, the way an object's reads do.

-- A file is named as the client names it: backslashes, any case, relative to the data folder,
-- whether it sits in an archive or in a folder of a mod.
local TOC = "Interface\\FrameXML\\FrameXML.toc"

-- This mod's assets folder joins the files the client searches. Priority 1 puts it ahead of the
-- client's own data, which is 0, so a file in it is found first under its client name.
local added, why = wxl.storage.AddSearchPath(wxl.mod.directory .. "\\assets", 1)
if not added then
    print("the assets folder is not searched: " .. why)
end

registerForEvent("onInit", function()
    -- A whole file, as a string.
    local toc, reason = wxl.storage.Read(TOC)
    if toc then
        local _, lines = toc:gsub("\n", "")
        print(TOC .. ": " .. #toc .. " bytes, " .. lines .. " lines")
    else
        print(TOC .. " was not read: " .. reason)
    end

    -- The file this mod ships in assets\Interface\lua-services\, found through the search path.
    local hello = wxl.storage.Read("Interface\\lua-services\\hello.txt")
    print("from the assets folder: " .. (hello and hello:gsub("\n", "") or "nothing"))

    -- A file read a piece at a time: Open, then ReadAt as often as needed, then Close.
    local file = wxl.storage.Open(TOC)
    if file then
        print("it starts with " .. string.format("%q", file:ReadAt(0, 12)))
        file:Close()
    end

    -- The names that match a pattern, * standing for any run of characters.
    local tocs = wxl.storage.Find("Interface\\FrameXML\\*.toc")
    print(#tocs .. " table(s) of contents in Interface\\FrameXML")

    -- The client reads each database once, early: whether it has read this one already.
    print("Spell.dbc read already: " .. tostring(wxl.data.IsLoaded("Spell.dbc")))

    -- The points this runtime names for a native hook. The recompiled client has no detours, so
    -- there the service is absent and the call answers nil and why.
    local points, why = wxl.hook.Points()
    if points then
        print(#points .. " hook point(s) named")
    else
        print("no hook points on this runtime: " .. why)
    end
end)

-- A function the client's own scripts can call, from an addon or from the chat box:
--     /run print(LuaServicesGreet("you"))
-- It is registered in every script context the client makes, from now on.
wxl.script.RegisterFunction("LuaServicesGreet", function(name)
    return "hello " .. tostring(name) .. ", from " .. wxl.mod.name
end)

-- An event the client's frames can listen to: frame:RegisterEvent("LUA_SERVICES_WORLD").
wxl.script.RegisterEvent("LUA_SERVICES_WORLD")

function WorldScript:OnWorldEnter(map_id)
    wxl.script.FireEvent("LUA_SERVICES_WORLD", map_id)
    -- Script text, run in the client's context: it writes a line in the chat frame.
    wxl.script.Run("DEFAULT_CHAT_FRAME:AddMessage('lua-services: entered map " .. map_id .. "')",
                   "lua-services")
end
