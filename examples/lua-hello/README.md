# lua-hello

A Lua mod: one folder, no build. Copy it into a runtime's `Mods/` next to `wxl.lua` and it runs
on the stock client and on the recompiled one alike.

```
lua-hello/
  mod.json        "lua": "init.lua", no "binaries"
  init.lua        the entry: callbacks, Observe, Override
  lib/greet.lua   reached with require("lib.greet")
```

Edit `init.lua` while the client runs: the mod reloads and its `onInit` runs again.

[docs/lua-mods.md](../../docs/lua-mods.md) explains everything it uses.
