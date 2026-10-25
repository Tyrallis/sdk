# Scout

A mod that notes the units appearing, leaving and dying around you, keeps the last 64 of those
notes in a journal, hands the journal to the game's interface, and counts the messages between
the client and the server.

[docs/your-first-mod.md](../../docs/your-first-mod.md) builds it from an empty folder, one file
at a time, and explains each idea as it comes up. This page is for running it.

## What it does

- notes every creature or player that appears, leaves or dies within 60 yards, and each new
  target, at any distance; entering a world starts a new journal
- registers five functions and one event on the client's Lua, in both of its contexts (the
  login screens and the game), again each time a context is created
- counts the messages and bytes of each opcode, in each direction, and drops an incoming opcode
  when asked to

| File | What it holds |
|---|---|
| `src/Journal.h`, `.cpp` | the clock and the journal, plain C++ |
| `src/WorldWatch.h`, `.cpp` | a `WorldScript`: the client's tick, entering and leaving a world |
| `src/UnitWatch.h`, `.cpp` | an `ObjectScript`: the units around you, read through `wxl.object` |
| `src/Traffic.h`, `.cpp` | a `NetScript`: the counts, under a lock, and the drops |
| `src/ScriptApi.h`, `.cpp` | a `LuaScript`: the functions and the event on the client's Lua |
| `src/Scout.cpp` | the mod: owns the rest and registers the scripts |

## Build

```sh
cmake -S . -B build
cmake --build build --config Release
```

```
build/dist/wxl.scout/
    mod.json
    bin/x64/scout.dll
    bin/x86/scout.dll
```

Copy the `wxl.scout` folder, whole, into `Mods/` beside the game's executable.

## Use

In the game's chat box:

```
/run print(ScoutCount(), "lines in the journal")
/run print(ScoutEntry())
/run print(ScoutNearby(), "units in view")
/run print(ScoutTraffic(0xA9))
```

| Function | Answers |
|---|---|
| `ScoutCount()` | how many lines the journal holds |
| `ScoutEntry([index])` | what, who, level, whole seconds ago; 1, the default, is the newest line, and nothing past the oldest |
| `ScoutNearby()` | how many units the client holds, you included |
| `ScoutTraffic(opcode [, "sent"])` | the messages and bytes of that opcode received, or sent |
| `ScoutDrop(opcode, drop)` | whether that incoming opcode was dropped before; `true` starts dropping it, `false` stops |

An addon can listen instead of asking:

```lua
local frame = CreateFrame("Frame")
frame:RegisterEvent("SCOUT_UPDATED")
frame:SetScript("OnEvent", function(self, event, what, who, level)
    print(who, level, what)
end)
```

A dropped opcode never reaches the client's own code: the client and the server stop agreeing on
whatever it carried. Scout drops nothing unless asked by number, logs each change as a warning,
and never drops what the client sends.

## What it needs

`mod.json` requires `wxl.object`: without it there is nothing to watch, and a runtime that lacks
it refuses Scout with that reason. `wxl.script` is optional: without it Scout keeps its journal
and says once in the log that the interface cannot read it. It asks for ABI `0.1.5`, the lowest
version with everything it uses. Both runtimes publish both services and raise every event Scout
handles.
