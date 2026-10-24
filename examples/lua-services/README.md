# lua-services

A Lua mod that uses the client's services: its files, its databases, its own script and the hook
points it names. Copy it into a runtime's `Mods/` next to `wxl.lua`.

```
lua-services/
  mod.json                                  "lua": "init.lua"
  init.lua                                  wxl.storage, wxl.data, wxl.hook, wxl.script
  assets/Interface/lua-services/hello.txt   a file the client finds through the search path
```

What it does, each line in `Logs/WarcraftXL.log` tagged `[wxl.lua-services]`:

- adds its `assets` folder to the files the client searches, and reads a file from it;
- reads `Interface\FrameXML\FrameXML.toc` whole, then its first bytes through a file it opens;
- counts the `.toc` files of `Interface\FrameXML`;
- says whether the client has read `Spell.dbc` yet, and how many hook points the runtime names;
- gives the client's script a function, `LuaServicesGreet`, and an event, `LUA_SERVICES_WORLD`,
  which it fires when you enter the world, along with a line in the chat frame.

In game, `/run print(LuaServicesGreet("you"))` in the chat box calls the function.

[docs/lua-mods.md](../../docs/lua-mods.md), "The client's services", explains everything it uses.
