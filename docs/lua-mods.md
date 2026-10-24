# 🌙 Lua mods

This guide takes you from an empty folder to a Lua mod that reacts to the game, reads the world
and changes what the client does, and then covers each part in detail. It ends with a reference
you can come back to.

You need to know some Lua (tables, functions, `local`) and to have played World of Warcraft. You
do not need C++, a compiler for your own code, or any knowledge of how the client works inside.

A Lua mod is a folder of `.lua` files. Nothing is built: the same folder runs on the stock
`Wow.exe` 3.3.5a and on the recompiled client, without a change.

---

## 🤔 How it fits together

Three pieces are involved, and it helps to know their names before you start.

- **The runtime** is what loads mods into the client. There are two: one for the stock
  `Wow.exe`, one built into the recompiled `WowClient64.exe`. Both look for mods in a folder
  named `Mods` next to the client's executable, and both write a log, `Logs/WarcraftXL.log`,
  next to it too.
- **`wxl.lua`** is a mod like any other, written in C++. It carries a Lua engine (LuaJIT, which
  runs Lua 5.1 code) and runs every Lua mod it finds in `Mods/`. Without it, a Lua mod does
  nothing.
- **Your mod** is a folder in `Mods/` with an `init.lua` in it. `wxl.lua` gives it a Lua state of
  its own: your globals, your modules and your errors never touch another mod's.

What a mod does is react to **events**. An event is something the client is about to do or has
just done: a unit died, a file is being opened, a message arrived from the server. Every event
belongs to a **family**, and the families have the names the C++ side uses for them:
`WorldScript`, `RenderScript`, `ObjectScript`, `AssetScript`, `NetScript`, `LuaScript`,
`CombatScript`, `PlayerScript`, `GlueScript` and `ChatScript`. An event is named `On` followed by what happened: `OnUnitDeath` of
`ObjectScript`, `OnMessageReceived` of `NetScript`.

A function you give to an event is a **handler**. When the event happens, your handler is called
with what the event carries -- the unit that died, the opcode and bytes of the message -- as its
parameters.

---

## 📦 1. Put `wxl.lua` in place

`wxl.lua` is built once, from this SDK, and then serves every Lua mod you write. If you already
have a `Mods/wxl.lua/` folder from someone else, skip to step 2.

Building it needs the same tools as a C++ mod: CMake 3.21 or newer and Visual Studio 2022 or newer
with the *Desktop development with C++* workload, x86 and x64 build tools both ([Building a
mod](building-a-mod.md) says which components). From the SDK's checkout:

```sh
cmake -S lua -B build-lua
cmake --build build-lua --config Release
```

The first configure downloads LuaJIT at a fixed commit and builds it for each architecture. The
result is one folder:

```
build-lua/dist/wxl.lua/
    mod.json
    bin/x64/wxl-lua.dll
    bin/x86/wxl-lua.dll
    LICENSE-LuaJIT.txt
```

Copy that whole folder into `Mods/` next to the client's executable, keeping its name. Each
runtime takes the DLL for its own architecture and ignores the other, so the same folder serves
both clients.

---

## ⚡ 2. A mod in five minutes

Create a folder `Mods/hello/` and, in it, a file `init.lua`:

```lua
print("hello from " .. wxl.mod.id)

function ObjectScript:OnUnitDeath(unit)
    print((unit and unit:Name() or "something") .. " died")
end
```

Start the client, enter the world, and open `Logs/WarcraftXL.log`. Near the top you will find
`wxl.lua` finding and loading your folder, then your own line:

```
[wxl.lua] found hello in folder 'hello' (no manifest)
[wxl.lua] loaded hello 0.0.0 (hello) from init.lua
[hello] hello from hello
```

Every line your mod prints is tagged with its id, here `hello`. Now kill something, or watch
something die, and look again:

```
[hello] Young Wolf died
```

Leave the client running and change the text in `init.lua`. Within a second the log shows
`reloading hello` and the new version is the one that runs. That is the loop you will work in:
edit, save, read the log.

Three things happened that the rest of this guide explains:

- `init.lua` ran from top to bottom when `wxl.lua` loaded the mod. The `print` at the top ran
  then; the function did not.
- Defining `OnUnitDeath` on the `ObjectScript` table made it a handler: after `init.lua` had run,
  `wxl.lua` subscribed it to that event.
- `unit` is an **object**: a value standing for a thing in the world, with methods such as
  `Name()` that read it.

---

## 🧭 3. A complete example, file by file

[`examples/lua-hello/`](../examples/lua-hello/) is a small mod that uses every mechanism a Lua
mod has. Copy it into `Mods/` beside `wxl.lua` and it runs as it is. This section goes through it
in the order the files are read.

```
lua-hello/
  mod.json        who the mod is and what it needs
  init.lua        the entry file
  lib/tally.lua   a module
```

### The manifest

A folder without a manifest is a mod already, as step 2 showed: its id comes from the folder's
name and its version is `0.0.0`. A manifest, `mod.json`, gives it a stable identity and lets it
state what it needs.

**`examples/lua-hello/mod.json`**

```json
{
  "$schema": "../../schema/mod.json",
  "id": "wxl.lua-hello",
  "name": "Lua hello",
  "version": "1.0.0",
  "abi": "0.1.5",
  "description": "A Lua mod: it counts deaths, reports targets, answers two events and reloads when edited.",
  "lua": "init.lua",
  "optional": [
    { "service": "wxl.object", "version": 1 }
  ]
}
```

- `lua` names the entry file, and is what makes this a Lua mod. A C++ mod lists `binaries`
  instead; this manifest has none.
- `abi` is the lowest version of the contract the mod needs, `major.minor.feature`. The contract
  grows by features: each adds events, and a mod asks for the feature that brought the newest
  event it uses. Everything lua-hello uses was there at `0.1.5`, so it asks for `0.1.5` and runs
  on every runtime since. A runtime older than what a mod asks for, or of another major or minor,
  refuses it and says why, instead of loading it and failing at the first unknown event.
- `optional` lists **services** the mod uses when they are there. A service is a set of
  functions a runtime offers; `wxl.object` is the one that reads world objects. Listing a service
  under `requires` instead makes the runtime refuse the mod when the service is missing.

The `$schema` line lets an editor check the file as you type. The [reference](#the-manifest-1)
lists every field.

### Modules and state

**`examples/lua-hello/init.lua`**

```lua
local Tally = require("lib.tally")

-- State lives in the file's locals: it lasts from one event to the next, and starts over when
-- the mod reloads.
local deaths = Tally.new()
```

`require("lib.tally")` loads `lib/tally.lua` from the mod's own folder and returns what that
file returns. Dots in the name are folders. `require` never looks outside the mod's folder, so two
mods can each have a `lib/tally.lua` without meeting.

**`examples/lua-hello/lib/tally.lua`**

```lua
local Tally = {}
Tally.__index = Tally

function Tally.new()
    return setmetatable({ counts = {}, total = 0 }, Tally)
end

-- Counts one more for a name, and returns how many that name has now.
function Tally:add(name)
    self.counts[name] = (self.counts[name] or 0) + 1
    self.total = self.total + 1
    return self.counts[name]
end
```

`deaths` is a local of `init.lua`. Every handler defined in the file sees it, and it keeps its
value from one event to the next for as long as the mod is loaded. That is how a mod remembers
anything. When the mod reloads, the file runs again in a new Lua state and `deaths` starts empty.

### Callbacks: the mod's own life

**`examples/lua-hello/init.lua`**

```lua
-- onInit runs once the client is fully up, and right after a reload.
registerForEvent("onInit", function()
    print("hello from " .. wxl.mod.name .. " on " .. wxl.runtime.name .. ", abi " .. wxl.runtime.abi)
end)

-- onShutdown runs when the client closes, and before a reload drops this state.
registerForEvent("onShutdown", function()
    print("deaths " .. deaths:summary())
end)
```

`registerForEvent` attaches a function to a moment in the mod's own life rather than to a game
event. There are four: `onInit`, `onUpdate`, `onDraw` and `onShutdown` (the
[reference](#registerforeventname-fn) says when each runs).

`init.lua` runs very early, before the client has finished starting. Handlers work from then on,
but anything that needs the client up -- the player, the interface -- belongs in `onInit`.
`onShutdown` is the last chance to report or save; here it prints the tally:

```
[wxl.lua-hello] deaths 3 seen: Thorgrim x2, Boar x1
```

`wxl` is a table every mod has: `wxl.mod` describes this mod (from its manifest), `wxl.runtime`
the runtime that is running it, including `wxl.runtime.abi`, the contract version it serves.

### A handler on a family table

**`examples/lua-hello/init.lua`**

```lua
function ObjectScript:OnUnitDeath(unit)
    if not unit then
        return
    end
    -- A read answers its value, or nil and the reason: "gone" once the unit has left.
    local name = unit:Name() or "something"
    local level = unit:Level() or "?"
    print(name .. " died at level " .. level .. " (" .. deaths:add(name) .. " so far)")
end
```

Each family is a global table. A function defined on it under an event's name is a handler for
that event, subscribed once the entry file has finished running. Because it is defined with `:`,
it is a method: its first parameter, `self`, is the family table, and the event's parameters come
after it. `OnUnitDeath` carries one, the unit.

The table is checked when it is subscribed: anything on `ObjectScript` that is not a function
named after one of its events stops the mod from loading, with a log line listing the events it
does have. A misspelt `OnUnitDaeth` is caught the moment you save, not after an evening of
wondering why nothing happens.

### Objects, and reads that can fail

`unit` is an object. Its methods read the thing it stands for: `Name()`, `Level()`, `Health()`,
`Position()` and dozens more. Every read can fail -- the unit may have left your view since
the event, or the method may not apply to this kind of object -- so every read answers either
the value, or `nil` and a short reason. `unit:Name() or "something"` is the usual way to say
"this, or a fallback". When you want to know why, take the second value:

**`examples/lua-hello/init.lua`**

```lua
Observe("ObjectScript", "OnTargetChanged", function(previous, current)
    if not current then
        print("target cleared")
        return
    end
    local health, why = current:Health()
    if health then
        print("target " .. (current:Name() or "?") .. ": " .. health .. "/" ..
              (current:MaxHealth() or "?") .. " health")
    else
        print("target " .. tostring(current) .. " has no health: " .. why)
    end
end)
```

Target a chest and the mod writes:

```
[wxl.lua-hello] target GameObject(0x000000000000002C) has no health: a GameObject has no Health
```

The reasons are `"gone"` (the object left), `"unsupported"` (this runtime cannot make that read)
and a sentence naming what the object is not, as above. An object parameter is `nil` when the
event has no object to give: `current` is `nil` when you clear your target.

### `Observe`, and taking a handler back

The handler above was given with `Observe(family, event, function)`, the second way to subscribe.
It does the same as a family method, with two differences: the function is not a method (no
`self`), and `Observe` can be called at any time, from anywhere -- inside another handler, inside
`onInit` -- while family methods are only collected once, after the entry file.

`Observe` returns an id, and `Unobserve(id)` removes that handler:

**`examples/lua-hello/init.lua`**

```lua
local firstWorld
firstWorld = Observe("WorldScript", "OnWorldEnter", function(map_id, position)
    print(string.format("first world: map %d at %.1f, %.1f, %.1f", map_id, position.x,
                        position.y, position.z))
    Unobserve(firstWorld)
end)
```

`firstWorld` is declared before the call so that the function can refer to it. The handler runs
for the first world you enter and then removes itself. `position` shows another kind of
parameter: a table with `x`, `y` and `z`.

### `Override`: changing what the client does

Some events can be **answered**: the handler decides something, and the client acts on it.
`OnMessageReceived` lets a handler hide a message from the client; `OnFileOpen` lets it name
another file to open instead. For these, a handler's return value is its answer, and
`Override(family, event, function)` subscribes it:

**`examples/lua-hello/init.lua`**

```lua
local SMSG_MOTD = 0x33D
Override("NetScript", "OnMessageReceived", function(opcode, data, suppressed)
    if opcode == SMSG_MOTD and not suppressed then
        print("message of the day hidden (" .. #data .. " bytes)")
        return true
    end
end)
```

`0x33D` is the opcode of the server's message of the day. `data` is the message's body as a Lua
string, so `#data` is its length in bytes. `suppressed` is the member the event is answered
through, and arrives as a parameter like the others, holding what was decided before this
handler ran: `true` if another handler already hid the message. Returning `true` hides it;
returning nothing leaves it alone.

A family method on an event that can be answered is an override already, and answers the same
way:

**`examples/lua-hello/init.lua`**

```lua
function AssetScript:OnFileOpen(path, redirect)
    if redirect == "" and path:lower() == "interface\\icons\\inv_misc_questionmark.blp" then
        return "Interface\\Icons\\INV_Misc_Coin_01.blp"
    end
end
```

`redirect` is the path to open instead, `""` while no handler has written one. Returning a
string writes it. The comparison is made in lowercase because the client's paths do not keep one
case. Doubled backslashes are how a Lua string holds one backslash.

That is the whole example. The sections below take each mechanism further.

---

## 🧩 4. Handlers in depth

### Three ways to subscribe

| Way | When it is subscribed | Called with | Answers |
|---|---|---|---|
| `function Family:OnEvent(...)` | once, after the entry file has run | `self`, then the event's parameters | on an event that can be answered |
| `Observe("Family", "OnEvent", fn)` | at the call | the event's parameters | never |
| `Override("Family", "OnEvent", fn)` | at the call | the event's parameters | always; refused on an event that cannot be answered |

`Observe` and `Override` return an id for `Unobserve`. A misspelt family or event is an error at
the call, naming what exists: the families, or the family's events. A function added to a family
table after the entry file has run is never subscribed; use `Observe` from then on.

A family table holds handlers and nothing else. Keep your state in locals, or in a table of your
own: `ObjectScript.count = 0` stops the mod from loading.

### Parameters

A handler receives what the event carries, one parameter per member, in a fixed order. The
order, names and types of every event's parameters are in the editor definitions,
[`lua/types/wxl.d.lua`](../lua/types/wxl.d.lua) (see [Your editor](#-6-your-editor)), generated
from the SDK's tables so that they always say what the runtime does. For example:

```lua
function ObjectScript:OnUnitAuraChanged(unit, spell_id, removed) end
function CombatScript:OnCombatDamage(source, source_guid, target, target_guid, spell_id, kind,
                                     environment, amount, overkill, school, absorbed, resisted,
                                     blocked, crit, hit_info) end
```

You may take fewer parameters than an event has (`function(source, source_guid, target)`): the
rest are dropped, as Lua always does. You cannot skip one in the middle; name it `_` if you do
not use it.

Each member arrives as a Lua value according to its kind. The kind is what the SDK's field table,
[`include/wxl/fields.def`](../include/wxl/fields.def), says the member is:

| Kind | In Lua |
|---|---|
| `u32`, `i32`, `u64`, `i64`, `f32` | a number. A 64-bit value above 2^53 loses its lowest digits |
| `bool` | `true` or `false` |
| `handle` | an object, or `nil` when there is none |
| `guid` | a string, `"0x"` and sixteen hex digits, such as `"0x0000000000000100"`: the server's id for an object, kept whole |
| `vec3` | a table `{ x = ..., y = ..., z = ... }` |
| `str` | a string |
| `bytes` | a string holding the bytes |
| `answer` | a boolean: what was decided so far; `nil` when the runtime gave nothing to answer through |
| `buffer` | a string: what was written so far; `nil` when the runtime gave nothing to write to |
| `ptr` | an opaque value (a light userdata) or `nil`; nothing in the Lua API uses it |

A member that only gives the length of another is not a parameter: the string carries its own
length. `OnMessageReceived` has no `size` parameter, `OnFileOpen` no `redirect_capacity`.

Every value is a copy. A handler may keep any of them in a local and read it later. An object
kept that way reads as `"gone"` once its subject has left.

### Answers

An event can be answered when its members include an `answer` or a `buffer`. An `Override` (or a
family method on such an event) answers with its return values, one per such member, in member
order. Every event of the contract has one such member today, so one return value:

- **an `answer`** (`suppressed`, `handled`, `refused`): `true` answers. `false` and `nil` leave it
  as it is: an answer is a request, and a later handler cannot take it back.
- **a `buffer`** (`redirect`): a string is written into it. A string longer than the buffer holds
  is not written, and the log says what the limit is.

A return value of the wrong kind (a number where `true` was expected) or more values than the
event takes are not used, and the log says so, once per mod and event. An `Observe` handler sees
the answer parameter but cannot change it.

The events that can be answered, as the SDK stands:

| Event | Answers through | What answering does |
|---|---|---|
| `WorldScript:OnInput` | `handled` | the client never sees the input |
| `WorldScript:OnSoundPlay` | `suppressed` | the sound does not play |
| `AssetScript:OnFileOpen` | `redirect` | the client opens the path you return instead |
| `NetScript:OnMessageReceived` | `suppressed` | the client never sees the server's message |
| `NetScript:OnMessageSent` | `suppressed` | the message does not go to the server |
| `ObjectScript:OnSpellCastRequest` | `refused` | the player's cast does not go ahead |
| `ObjectScript:OnItemUseRequest` | `refused` | the item is not used |
| `ChatScript:OnChatSend` | `suppressed` | the chat or addon message is not sent (abi `0.1.6`) |

### The order handlers run in

For one event, handlers run in the order they were subscribed. Mods are loaded in an order the
manifests decide (`loadAfter`, then folder names in alphabetical order), and within a mod, the
`Observe` and `Override` calls the entry file makes come first, in order, then its family
methods. A mod that reloads is loaded last, so its handlers move after every other mod's until the
client restarts.

Lua mods all run inside `wxl.lua`, so next to C++ mods they run where `wxl.lua` runs.

### Events from other threads

Every handler runs on the client's main thread, the one that runs the game. A few events can
happen elsewhere: [`events.def`](../include/wxl/events.def) says so on their row, and today they
are `OnFileOpen` (any thread that opens files), `OnMessageSent` (the connection's thread), and
`OnGlueRealmList` and `OnGlueRealm` (the logon connection's thread).
When one of them happens on another thread, `wxl.lua` copies it and delivers the copy at the
start of the next `OnUpdate`. Your handler sees the same values, a moment later. It can no longer
answer: the client has already gone on. The log says so the first time, once per mod and event:

```
[wxl.lua] mod wxl.lua-hello: OnFileOpen was raised off the main thread; its Override handler ran on a copy at the next Update and its answer is not taken
```

---

## 🌍 5. Objects in depth

An object stands for something in the client's world: a unit (a creature or an NPC), a player,
an item, a container, a game object (a chest, a door, a mailbox), a dynamic object (a spell's
area on the ground), a corpse. You get one from an event's parameter or from `wxl.object`:

```lua
local me = wxl.object.LocalPlayer()               -- nil while out of the world
local units = wxl.object.InView("Unit")           -- every unit in view, as a list
local someone = wxl.object.FromGuid("0x0000000000000100")  -- nil when it is not in view
```

`InView` takes a type name (`"Item"`, `"Container"`, `"Unit"`, `"Player"`, `"GameObject"`,
`"DynamicObject"`, `"Corpse"`) or nothing, for every object. A list is a snapshot: an object in
it may have left by the time you read it, and then answers `"gone"`.

The methods:

- `Name()`, `Guid()`, `Type()` (the kind's name, `"Unit"`, `"GameObject"`...), `TypeId()` (the
  same as a number) and `Target()` (what a unit is targeting, an object) work on every kind.
- `Valid()` answers `true` while the object is still there, `false` once it has left.
- `Handle()` answers the object's handle as a string, for logs.
- The others come from the runtime's table of reads and are named after it: `Health()`,
  `MaxHealth()`, `Level()`, `Power(index)`, `Position()`, `Facing()`, `DisplayId()`,
  `StackCount()`, `Durability()`... The editor definitions list all of them, with the kinds each
  applies to. A method that does not apply to an object answers `nil` and a sentence such as
  `"a GameObject has no Health"`. A read whose name would be taken twice says its kind:
  `GameObjectType()` is a game object's own type (a chest, a door), `PlayerFlags()` a player's
  flags beside the `Flags()` every unit has.

### One object, one value

While an object stays in view, it is one Lua value: the object an event hands you, the one
`FromGuid` finds and the one in an `InView` list are the same value (abi `0.1.7`: a mod that
counts on it asks for `"0.1.7"`). You can compare objects with `==` and use them as table keys:

```lua
local hits = {}
Observe("CombatScript", "OnCombatDamage", function(source, source_guid, target)
    if target then hits[target] = (hits[target] or 0) + 1 end
end)
```

An object also keeps fields of your own. Set one as on a table, read it back at any later event:

```lua
Observe("ObjectScript", "OnObjectCreate", function(object, guid, type)
    object.noted = true
end)
```

A field cannot take the name of a method (`object.Health = 1` is an error that says so), and
stays with the object until it leaves. That is the difference with a table of your own keyed by
objects, like `hits` above: the table keeps an object that left until you remove it, while a
field goes with the object.

### When an object leaves

When the object leaves view, your `OnObjectDestroy` handlers run first, with its fields still
there. Then the object you kept is **gone**: `Valid()` answers `false`, every read answers `nil,
"gone"`, its fields are dropped and storing a new one is an error. `tostring` shows it as `gone
Unit(0x...)`.

If the same creature comes back into view, it is a new object: a new value, with no fields, and
`OnObjectCreate` announces it again. What it shares with the old one is its guid, so key a table
by guid for what must outlast leaving and coming back:

```lua
local seen = {}
Observe("ObjectScript", "OnObjectCreate", function(object, guid, type)
    seen[guid] = (seen[guid] or 0) + 1
end)
```

A guid is a string because a Lua number cannot hold every 64-bit value; as a string it compares
and keys tables exactly. `FromGuid` takes the string, `"0x"` and up to sixteen hex digits, or a
whole number for a small one; anything else is an error.

When no runtime offers the `wxl.object` service, every read answers `nil, "unsupported"`,
`Valid()` included, and `InView` an empty list. List the service under `requires` in your
manifest if your mod is no use without it.

---

## 🔁 6. Your mod's life

### Loading

When the client starts, the runtime loads `wxl.lua`, which reads `Mods/` and for each folder:

1. takes its `mod.json` if it has one and names a `lua` entry, or, with no manifest at all, an
   `init.lua`, naming the mod after the folder: lowercase, letters and digits, runs of anything
   else as one `-` (`My Mod_One` is `my-mod-one`);
2. refuses it, with the reason in the log, if its `abi` cannot be served or a service it
   `requires` is missing;
3. orders the mods by `loadAfter` and folder name;
4. for each, creates a Lua state, runs the entry file, and subscribes its family methods.

The runtime's own log says, for every Lua mod, that it was skipped because `wxl.lua` serves it.
That is expected: the runtime loads C++ mods, `wxl.lua` the Lua ones.

```
[host] skipped wxl.lua-hello: script mod (lua: init.lua): wxl.lua serves it
[wxl.lua] loaded wxl.lua-hello 1.0.0 (Lua hello) from init.lua
```

The `abi` a Lua mod is checked against is the lower of the runtime's and the one `wxl.lua` was
built with: an event newer than your `wxl.lua` is not available, whatever the runtime offers.
`wxl.runtime.abi` says what your mod is served.

### When each part runs

| What | When |
|---|---|
| the entry file | once, when `wxl.lua` loads, before the client has finished starting; again at each reload |
| family methods, `Observe`, `Override` handlers | every time their event happens, from the moment they are subscribed |
| `onInit` | once the client is fully up (`WXL_STAGE_READY`), and right after each reload |
| `onUpdate(delta)` | every game tick, after the `OnUpdate` handlers; `delta` is the time since the previous one, in seconds |
| `onDraw` | every frame, once the interface is drawn |
| `onShutdown` | when the client closes, and before each reload, in the state that is going away |

### Reloading

Once a second, `wxl.lua` compares each Lua mod's folder with what it saw last (how many files,
their sizes, the latest change time). When it differs, the mod reloads: `onShutdown` runs, its
handlers are removed, its Lua state is closed, then the entry file runs in a new state and
`onInit` runs. Other mods are not touched.

Nothing in Lua memory survives a reload. A file the mod writes into its own folder does (see
the sandbox below), but writing one is itself a change: a mod that writes a file while it runs
reloads a second later. Write at `onShutdown`, which runs before the folder is looked at again.

### Errors

An error in a handler or a callback is written to the log with the mod's id, what it was doing,
and a traceback (the chain of calls that led to it). The other handlers and the other mods carry
on:

```
[wxl.lua] mod hello errored in its Observe handler for OnWorldEnter of WorldScript (1/25): hello/init.lua:4: attempt to index local 'position' (a nil value)
stack traceback: ...
```

The count is there because a mod that keeps failing is **disabled** at the 25th error since it
was loaded: its handlers are removed and its callbacks stop. The log says so, and it stays off
until one of its files changes, which reloads it with the count back at zero.

An error while the entry file runs, a syntax error included, means the mod is not loaded. Fix the
file and save: the change reloads it.

`print` writes a line to the log at the `info` level, tagged with the mod's id, its arguments
separated by tabs. `wxl.log(level, ...)` picks the level: `"trace"`, `"debug"`, `"info"`,
`"warn"` or `"error"`.

### The sandbox

A mod runs with the standard Lua libraries, without what would let it reach outside the game or
outside its folder:

- **removed:** `os.execute`, `os.exit`, `os.getenv`, `os.remove`, `os.rename`, `os.setlocale`,
  `os.tmpname`, `io.popen`, `io.tmpfile`, `io.input`, `io.output`, `io.read`, `io.stdin`,
  `io.stdout`, `io.stderr`, `package.loadlib`, the `ffi` module, `jit.util`, `jit.profile`, and
  every function of `debug` but `debug.traceback` and `debug.getinfo`;
- **limited to the mod's folder:** `io.open`, `io.lines`, `loadfile`, `dofile` and `require` take
  paths relative to the folder, and refuse one that is absolute or contains `..`;
- **text only:** `load`, `loadstring` and every file loaded refuse precompiled Lua bytecode;
- **redirected:** `io.write` writes to the log, one line per call.

`require("a.b")` looks for `a/b.lua`, then `a/b/init.lua`, in the mod's folder. `package.preload`
works as usual. `os.time`, `os.clock`, `os.date`, `string`, `table`, `math` and `bit` are all
there.

---

## ✏️ 7. Your editor

[`lua/types/wxl.d.lua`](../lua/types/wxl.d.lua) describes the whole API for
[lua-language-server](https://github.com/LuaLS/lua-language-server), which the *Lua* extension
of VS Code and most other editors use: every family's events with their parameters named and
typed, what an `Override` returns, `Observe` and `Override` with the families and events to
complete, every object method and the kinds it applies to, and the `wxl` table. Pointed at it,
the editor completes `ObjectScript:On...`, shows a handler's parameters as you type them, and
underlines a misspelt event before you save.

In VS Code, add to the workspace's `.vscode/settings.json`:

```json
{
  "Lua.runtime.version": "LuaJIT",
  "Lua.workspace.library": [ "path/to/sdk/lua/types" ]
}
```

A copy of `wxl.d.lua` anywhere in the folder the editor has open works too. The file is
generated from the same tables `wxl.lua` is built from; take it from the SDK your `wxl.lua` was
built with, so it describes the events your `wxl.lua` knows.

---

## 📚 8. Reference

### The manifest

`mod.json`, beside the entry file. The schema is [`schema/mod.json`](../schema/mod.json).

| Field | Required | Meaning |
|---|---|---|
| `id` | yes | the mod's identity: lowercase letters and digits, separated by single `.` or `-`, 3 to 64 characters. It tags its log lines and is what `loadAfter` names |
| `name` | yes | the name shown in the log |
| `version` | yes | the mod's own version, `1.2.3` |
| `abi` | yes | the lowest contract version the mod needs, `major.minor.feature`, such as `"0.1.5"`: the feature that brought the newest event it uses. A runtime of another major, or another minor while the major is 0, refuses it; so does one whose feature is lower. A two-part version (`"0.5"`) is refused as the old numbering |
| `lua` | yes, for a Lua mod | the entry file, relative to the folder; it may not leave it |
| `requires` | no | services the mod cannot run without, `{ "service": "wxl.object", "version": 1 }`. A runtime without one refuses the mod |
| `optional` | no | services the mod uses when they are there; informational |
| `loadAfter` | no | ids of mods to load before this one when they are present; a missing one is ignored |
| `description`, `authors`, `homepage` | no | for people reading the folder |

A folder with an `init.lua` and no `mod.json` is a mod named after the folder, version `0.0.0`,
served the current contract, requiring nothing.

### Globals

#### `registerForEvent(name, fn)`

Adds `fn` to one of the mod's callbacks. `name` is `"onInit"`, `"onUpdate"` (`fn` receives
`delta`, in seconds), `"onDraw"` or `"onShutdown"`; [When each part runs](#when-each-part-runs)
says when. Several functions may share a name; they run in the order they were added. Another
name is an error listing these four.

#### `Observe(family, event, fn)`

Subscribes `fn` to an event and returns its id, a number. `family` is a family's name as a string
(`"WorldScript"`), `event` the event's (`"OnUpdate"`). `fn` receives the event's parameters. A
handler subscribed while the event is being delivered runs from the next time.

#### `Override(family, event, fn)`

As `Observe`, and `fn`'s return values answer the event ([Answers](#answers)). An error on an
event that cannot be answered.

#### `Unobserve(id)`

Removes the handler with that id. Answers `true`, or `false` when the mod has no handler with that
id (it was already removed). Safe from inside the handler itself.

#### The family tables

`WorldScript`, `RenderScript`, `ObjectScript`, `AssetScript`, `NetScript`, `LuaScript`,
`CombatScript`, `PlayerScript`, `GlueScript`, `ChatScript`. A function defined on one under an event's name is subscribed after the entry file
has run, as an `Override` on an event that can be answered and as an `Observe` otherwise, and is
called with `self` first. Anything else on these tables, or replacing one with something that is
not a table, stops the mod from loading.

#### `print(...)`

Writes its arguments to the log, through `tostring`, separated by tabs, at the `info` level.

### The `wxl` table

| Field | What it holds |
|---|---|
| `wxl.mod.id`, `.name`, `.version` | this mod, from its manifest or its folder |
| `wxl.mod.directory` | the full path of the mod's folder, for messages (file functions take paths relative to the folder) |
| `wxl.runtime.id`, `.name`, `.version` | the runtime that loaded `wxl.lua` |
| `wxl.runtime.abi` | the contract this mod is served, as text, such as `"0.1.6"` |
| `wxl.runtime.abi_major`, `.abi_minor`, `.abi_feature` | the same, as numbers |
| `wxl.object.LocalPlayer()` | the player you control, or `nil` |
| `wxl.object.FromGuid(guid)` | the object with that guid (`"0x"` and up to sixteen hex digits, or a whole number), or `nil` when it is not in view |
| `wxl.object.InView(type)` | a list of the objects of a type in view; every object without a type |
| `wxl.events` | each family's events by name, `wxl.events.NetScript[1] == "OnMessageReceived"` |
| `wxl.log(level, ...)` | a log line at `"trace"`, `"debug"`, `"info"`, `"warn"` or `"error"` |

### Object methods

| Method | Answers |
|---|---|
| `Valid()` | whether the object is still there; `nil, "unsupported"` without the object service |
| `Guid()` | its guid, `"0x..."` |
| `Type()`, `TypeId()` | its kind, as a name or a number |
| `Name()` | its name |
| `Target()` | for a unit, the object it targets, or `nil` |
| `Handle()` | its handle as text, `"0x..."`, for logs |
| every other method | a read from the runtime's table: the value, or `nil` and why. The list, with the kinds each applies to, is in [`wxl.d.lua`](../lua/types/wxl.d.lua) |

`tostring(object)` gives its kind and handle, `Unit(0x000000000000002B)`, `gone Unit(...)` once it
has left. An object is one value while it is in view: `==` and table keys work on it, and any
other name is a field of your own (`object.note = "rare"`), dropped when it leaves.

### Events

The families and their events are listed, with every parameter, in
[`wxl.d.lua`](../lua/types/wxl.d.lua), and described one line each in
[`include/wxl/events.def`](../include/wxl/events.def). At run time, `wxl.events` lists them.
A few to start from:

| Family | Event | Parameters | Happens when |
|---|---|---|---|
| `WorldScript` | `OnUpdate` | `delta` | every game tick |
| | `OnWorldEnter` | `map_id`, `position` | the world finished loading and you are in it |
| | `OnWorldClick` | `point`, `object`, `button` | a click landed in the world (`button`: 0 left, 1 right, 2 middle) |
| `ObjectScript` | `OnObjectCreate` | `object`, `guid`, `type` | an object came into view |
| | `OnTargetChanged` | `previous`, `current` | your target changed |
| | `OnUnitDeath` | `unit` | a unit died |
| | `OnUnitAuraChanged` | `unit`, `spell_id`, `removed` | an aura was applied to a unit, or left it |
| | `OnSpellCastRequest` | `caster`, `spell_id`, `item`, `target_guid`, `confirmed`, `refused` | you ask to cast a spell; answer to refuse it |
| `CombatScript` | `OnCombatDamage` | `source`, `source_guid`, `target`, `target_guid`, `spell_id`, ... `amount`, ... | damage landed, as the server reports it |
| `NetScript` | `OnMessageReceived` | `opcode`, `data`, `suppressed` | a message arrived from the server; answer to hide it |
| `AssetScript` | `OnFileOpen` | `path`, `redirect` | a file is about to be opened; answer with another path |
| `LuaScript` | `OnUiEvent` | `index`, `name`, `format`, `values`, `value_count` | the client fires an event to its interface (`PLAYER_TARGET_CHANGED`...) |
| `PlayerScript` | `OnPlayerMoneyChanged` | `player`, `previous`, `current` | your money changed, in copper (abi `0.1.6`) |
| `ChatScript` | `OnChatMessage` | `type`, `language`, `sender`, `sender_guid`, `sender_name`, ... `text`, ... | a chat message of the server became a line the chat shows (abi `0.1.6`; `line_id`, `shown_text` `0.1.7`) |
| | `OnChatAddonMessage` | `type`, `sender`, `sender_guid`, `sender_name`, `prefix`, `text`, `channel`, `gm` | an addon message reached the interface as `CHAT_MSG_ADDON` (abi `0.1.7`) |
| `GlueScript` | `OnGlueScreenChanged` | `screen`, `previous` | the login screens moved on: `"login"`, `"charselect"`... (abi `0.1.6`) |

The families `PlayerScript`, `GlueScript` and `ChatScript`, and `WorldScript`'s `OnZoneChanged`,
`OnMouseoverChanged` and `OnCVarChanged`, came with `0.1.6`: a mod that uses one asks for
`"abi": "0.1.6"`, so that an older runtime refuses it with a reason rather than failing at the
unknown name. `ChatScript`'s `OnChatAddonMessage`, and `OnChatMessage`'s last two parameters, came
with `0.1.7`: a mod that uses them asks for `"abi": "0.1.7"`.

Not every runtime raises every event. Subscribing to one that yours does not raise logs a warning
naming it, and the handler never runs.

### The `wxl.lua` service

`wxl.lua` publishes a service, `wxl.lua` version 1
([`lua/include/wxl/lua.h`](../lua/include/wxl/lua.h)), for a runtime, a console or a C++ mod: how
many Lua mods there are, `poll()` to look for changed folders now, `reload(id)` to reload one mod.

---

## 🧰 When it does not work

Every one of these leaves a line in `Logs/WarcraftXL.log`, tagged `[wxl.lua]`. Search the log for
your mod's id first.

| The log says | What it means |
|---|---|
| nothing about your mod at all | `wxl.lua` is not in `Mods/`, or your folder has neither a `mod.json` with `lua` nor an `init.lua`. Look for `[wxl.lua] scanning` |
| `mod <id> failed to load init.lua: ...` | the entry file has a syntax error, or raised one while running; the rest of the line is Lua's message, with the line number |
| `<Family>.<name> is not an event of <Family>; its events are ...` | a misspelt family method, or something other than a handler on a family table |
| `refused <id>: needs abi ...` | the manifest's `abi` is newer than your `wxl.lua` or the runtime, or of another minor. The line gives both versions |
| `refused <id>: requires service ...` | a service under `requires` is missing from this runtime |
| `folder '<name>' holds an init.lua but its name makes no mod id` | rename the folder, or give it a `mod.json` with an `id` |
| `this runtime does not raise On<Event> of <Family>Script` | the handler is valid but will never run on this runtime |
| `mod <id> errored in its ... (n/25)` | an error in a handler; the traceback after it gives the line |
| `mod <id> is disabled: it errored 25 times` | fix the error and save a file of the mod: it reloads and runs again |
| `the Override of On<Event> returned ...` | a return value of the wrong kind, too many of them, or a string too long for the buffer |
| `On<Event> was raised off the main thread` | that event happened on another thread, and its answer could not be taken ([Events from other threads](#events-from-other-threads)) |
| your mod reloads every second | it writes a file into its own folder while it runs; write at `onShutdown` |

---

## 🧭 Where to go next

- [`examples/lua-hello/`](../examples/lua-hello/), the example of section 3, to copy and change.
- [`lua/types/wxl.d.lua`](../lua/types/wxl.d.lua), for every event and object method.
- [Your first mod](your-first-mod.md), the same platform from C++, for the services the Lua API
  does not offer yet: the client's interface Lua (`wxl.script`), files and archives, hooks and
  patches.
