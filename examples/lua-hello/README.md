# lua-hello

A Lua mod: one folder, no build. Copy it into a runtime's `Mods/` next to `wxl.lua` and it runs
on the stock client and on the recompiled one alike.

```
lua-hello/
  mod.json        "lua": "init.lua", no "binaries"
  init.lua        the entry: callbacks, family methods, Observe, Unobserve, Override
  lib/tally.lua   a module, reached with require("lib.tally")
```

What it does, each line in `Logs/WarcraftXL.log` tagged `[wxl.lua-hello]`:

- greets the runtime once the client is up, and prints a tally of the deaths it saw when it stops;
- counts every unit death it sees, by name;
- reports your new target's health, or why it has none;
- names the first map you enter, then stops listening;
- hides the server's message of the day;
- shows a coin wherever the interface asks for the question-mark icon.

Edit `init.lua` while the client runs: within a second the mod reloads, its `onShutdown` runs in
the old state and its `onInit` in the new one.

[docs/lua-mods.md](../../docs/lua-mods.md) explains everything it uses.
