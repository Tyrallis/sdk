-- lua-assets: a file served from memory, and what the engine loads.

-- A file the client opens as one of its own, on any of its threads, until the mod goes. Priority
-- 1 puts it above the client's archives; this name is in none of them anyway.
local NOTE = "Interface\\lua-assets\\note.txt"
local ok, why = wxl.storage.Serve(NOTE, "This file lives in a Lua mod's memory.", 1)
print("served " .. NOTE .. ": " .. tostring(ok or why))

-- Read back through the client's own file layer, as any of its files would be.
local text = wxl.storage.Read(NOTE)
print("read back: " .. tostring(text))

-- What the engine loads, counted; the first of each is described.
local counts = {}

local function count(name, describe)
    counts[name] = (counts[name] or 0) + 1
    if counts[name] == 1 then print("first " .. name .. ": " .. describe) end
end

-- The tile's bytes arrive as one string; # gives their count.
function AssetScript:OnTerrainTileLoad(path, tile_x, tile_y, data)
    count("terrain tile", string.format("%s (%d, %d), %d bytes", path, tile_x, tile_y, #data))
end

function AssetScript:OnTextureDecode(path, width, height, encoding, alpha_bits)
    count("texture", string.format("%s, %dx%d, %s, %d-bit alpha", path, width, height, tostring(encoding), alpha_bits))
end

function AssetScript:OnSoundFileLoad(path, streaming)
    count("sound", path .. (streaming and ", streamed" or ""))
end

-- /run LuaAssetsReport() in the chat box: how many of each so far.
wxl.script.RegisterFunction("LuaAssetsReport", function()
    for name, n in pairs(counts) do print(name .. ": " .. n) end
end)
