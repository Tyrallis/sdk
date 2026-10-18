# 🧭 Scout

A mod that watches what happens around you, keeps a short journal of it, and hands that journal
to the game's interface.

It is the worked example behind [docs/your-first-mod.md](../../docs/your-first-mod.md), which
builds it up one file at a time. Read that if you are starting out; read this if you just want
to run it.

## What it does

- notes units arriving, leaving, dying, and what you target, within 60 yards
- clears the journal when you enter a world, so what you read is what happened *here*
- registers five functions and one event on the client's own Lua, in **both** of its contexts
  (the login screen and the game), re-registering every time a context is created
- counts every message crossing to and from the server, by opcode, and can drop an incoming
  opcode on request

## Build

```sh
cmake -S . -B build
cmake --build build --config Release
```

Out comes a mod folder, both architectures from the one command:

```
build/dist/wxl.scout/
    mod.json
    bin/x64/scout.dll
    bin/x86/scout.dll
```

## Install

Copy the whole `wxl.scout` folder into `Mods/` beside the client's executable. Do not flatten
it: the runtime looks for `Mods/<id>/mod.json` and `Mods/<id>/bin/<arch>/<name>.dll`.

## Use

From any script console or addon, once you are in the world:

```lua
print(ScoutCount(), "notes")

local what, who, level = ScoutLast()
if what then print(who, "("..level..")", what) end

print(ScoutNearby(), "units in view")
print(ScoutTraffic(0x0085), "SMSG_MONSTER_MOVE seen")
```

An addon can react instead of polling:

```lua
local f = CreateFrame("Frame")
f:RegisterEvent("SCOUT_UPDATED")
f:SetScript("OnEvent", function(self, event, what, who, level)
    print(who, what)
end)
```

## The one dangerous function

```lua
ScoutMute(0x0085, true)   -- the client never sees that opcode again
ScoutMute(0x0085, false)  -- back to normal
```

A muted opcode is dropped before the client's own handler runs. That is how a mod serves an
opcode the client does not know; it is also how a mod makes the client disagree with the server
about the state of the world. Only incoming messages can be muted, and nothing is muted unless
you ask for it by number.

## What depends on the runtime

| | stock `Wow.exe` 3.3.5a | the recompiled client |
|---|---|---|
| the journal, the Lua API, the traffic counters, `OnWorldEnter`/`OnWorldLeave` | yes | yes |
| `OnLoad`, `OnStorageReady`, `OnScriptReady`, `OnShutdown`, `OnUnload` | yes | yes |
| `OnUpdate` and `OnReady` | no | yes |
| `OnObjectCreate`, `OnObjectDestroy`, and every handle `OnUnitDeath` and `OnTargetChanged` carry | no | yes |
| `OnGraphicsReady` | not reported by either runtime yet | the same |

Two gaps, one cause. On the stock client four of the functions these events sit on take their
argument in a register rather than on the stack -- the compiler's own choice, because each has
internal linkage and one caller. A detour cannot hold a register across its own body and give
it back, so the runtime declares those seams and refuses to attach them rather than hand the
client a pointer it computed from nothing.

The tick is one of them, which is why the journal's clock never advances on `Wow.exe`. The
object manager is the other three, so nothing is ever tracked there: `wxl.object` answers for
an empty world, and the handle in a death or a target change reads as absent. The watchers
still run and the journal still fills from what does arrive -- the world, the traffic, the Lua
side -- but an object-shaped entry is a recompiled-client entry today.

`mod.json` requires `wxl.object` -- without it there is nothing to watch, and a runtime that
lacks it refuses the mod with a reason instead of loading it to do nothing.
