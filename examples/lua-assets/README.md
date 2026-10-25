# lua-assets

A Lua mod that serves a file from memory with `wxl.storage.Serve` and counts what the engine
loads through the asset events: terrain tiles, textures as they are decoded, sound files.

Copy the folder into the client's `Mods/` next to `wxl.lua`. The log shows the served file read
back, then the first tile, texture and sound of the session; `/run LuaAssetsReport()` prints the
counts. It needs a runtime of abi `0.1.19` or later. The walkthrough is in
`docs/lua-mods.md`, "Serving and converting assets".
