# 🧭 Your first mod

This builds one real mod from nothing: **Scout**, which watches what happens around you, keeps
a journal of it, and hands that journal to the game's interface so an addon can show it.

It is not a toy. By the end it reads the world, serves the client's own Lua in both of its
contexts, and watches every message crossing to the server. The finished thing lives in
[`examples/scout/`](../examples/scout) and builds with one command.

You need to know C++. You do not need to know anything about World of Warcraft's internals,
about hooking, or about this project.

---

## 🤔 What a mod actually is

A DLL with four functions in it. The runtime looks them up by name, calls the first to ask who
you are, the second to start you, and the others as the game moves along.

You will never write those four functions. `WXL_MOD` writes them, and what you write instead is
a class:

```cpp
class MyMod final : public wxl::Mod
{
public:
    wxl_result OnLoad() override
    {
        wxl::Log(WXL_LOG_INFO, "mymod", "hello");
        return WXL_OK;
    }
};

WXL_MOD(MyMod, "com.example.mymod", "My Mod", 1)
```

Three things are worth knowing before you go further.

**Your C++ stays yours.** Nothing of C++ crosses between your mod and the runtime -- no
classes, no exceptions, no `std::string`. What crosses is plain C. That is why your mod can be
built with a different compiler than the runtime that loads it, and why an exception must never
escape one of your hooks.

**One source, two binaries.** The same code builds for the 32-bit stock client and for the
64-bit recompiled one. You write it once; the build produces both.

**Nothing is guaranteed to be there.** A runtime offers services -- reading world objects,
reaching the client's Lua -- and not every runtime offers all of them. Your manifest says which
ones you cannot live without, and a runtime that lacks one refuses your mod *at the door*, with
the reason in its log. That is much better than loading and quietly doing nothing.

---

## 📦 1. Get set up

You need **Visual Studio 2022 or newer with both the x86 and x64 build tools** (the installer
calls them "MSVC v143 x64/x86 build tools") and **CMake 3.21 or newer**.

Clone the SDK:

```sh
git clone https://github.com/WarcraftXL/sdk.git
```

Before writing anything of your own, build the finished example. If this works, your toolchain
is fine and anything that breaks later is your code:

```sh
cd sdk/examples/scout
cmake -S . -B build
cmake --build build --config Release
```

You should end up with this, and nothing red:

```
build/dist/wxl.scout/
    mod.json
    bin/x64/scout.dll
    bin/x86/scout.dll
```

That folder *is* the mod. Installing it is copying it into `Mods/` next to the client's
executable -- the whole folder, not its contents.

---

## 🧱 2. The smallest mod that runs

Make a folder of your own with three things in it:

```
scout/
    CMakeLists.txt
    mod.json
    src/Scout.cpp
```

### The build file

**`examples/scout/CMakeLists.txt`**

```cmake
wxl_add_mod(scout
    SOURCES
        src/Scout.cpp
        src/Journal.cpp
        src/WorldWatch.cpp
        src/ObjectWatch.cpp
        src/NetWatch.cpp
        src/ScriptApi.cpp
    MANIFEST mod.json
    TARGETS win-x64 win-x86)
```

`wxl_add_mod` is the entire build. It configures itself once per architecture, compiles your
sources against the SDK, and collects the result into `dist/<your id>/`. You do not write a
target, a link line or an install rule.

### The manifest

**`examples/scout/mod.json`**

```json
{
  "$schema": "../../schema/mod.json",
  "id": "wxl.scout",
  "name": "Scout",
  "version": "1.0.0",
  "abi": "0.3",
  "description": "Keeps a journal of what happens around you and hands it to the interface.",
  "binaries": {
    "win-x64": "bin/x64/scout.dll",
    "win-x86": "bin/x86/scout.dll"
  },
  "requires": [
    { "service": "wxl.object", "version": 1 }
  ],
  "optional": [
    { "service": "wxl.script", "version": 1 }
  ]
}
```

| Field | What it does |
|---|---|
| `id` | your identity, forever. It names your folder and appears in every log line |
| `abi` | the contract version you built against. A mismatch is refused, deliberately |
| `requires` | a service you cannot work without. A runtime without it refuses your mod |
| `optional` | a service you use if it is there, and carry on without if it is not |

Point `$schema` at the SDK's `schema/mod.json` and your editor will complete and check this
file as you type it.

> 🚧 **The first thing everyone gets wrong.** The `id` in `mod.json` and the id in `WXL_MOD`
> must be the same string, to the letter. When they differ the mod is refused and the log says
> so -- read the log.

### The code

Write the class from the top of this page into `src/Scout.cpp`, build, copy
`build/dist/.../` into `Mods/`, start the client, and open `Logs/WarcraftXL.log`. You are
looking for your own line, and for these two:

```
[host] loaded wxl.scout 1.0.0 (Scout) from bin/x86/scout.dll
[core] mod wxl.scout 1.0.0 (Scout) is up
```

That is a mod. Everything below is making it do something.

---

## 🌍 3. React to something

Events are grouped into six **families**: the world, rendering, objects, assets, the network,
and the script VM. You derive from the family you care about and override the handful of hooks
you want. The ones you do not override cost nothing at all.

Scout's first watcher cares about entering and leaving a world, and about the clock:

**`examples/scout/src/Watchers.h`**

```cpp
/**
 * @brief The world coming and going, and the clock.
 *
 * The journal is about one place: entering a world clears it, so what you read is what
 * happened here.
 */
class WorldWatch final : public wxl::WorldScript
{
public:
    WorldWatch(Clock& clock, Journal& journal) : clock_(clock), journal_(journal) {}

    void OnUpdate(const wxl_update_args& args) override;
    void OnWorldEnter(const wxl_world_args& args) override;
    void OnWorldLeave(const wxl_world_args& args) override;

    /// The map we are on, or 0 while out of the world.
    uint32_t MapId() const { return mapId_; }

private:
    Clock&   clock_;
    Journal& journal_;
    uint32_t mapId_ = 0;
};
```

Note what it holds: **references**, not copies. There is one journal and one clock in the whole
mod, and everything that needs them borrows them. Who owns them is the mod class, in step 8.

The hooks themselves:

**`examples/scout/src/WorldWatch.cpp`**

```cpp
void WorldWatch::OnWorldEnter(const wxl_world_args& args)
{
    mapId_ = args.map_id;
    journal_.Clear();

    char line[96];
    std::snprintf(line, sizeof line, "watching map %u from %.0f, %.0f", args.map_id,
                  args.position.x, args.position.y);
    wxl::Log(WXL_LOG_INFO, kTag, line);
}
```

`OnWorldEnter` fires once the terrain and objects around you are loaded. Scout clears the
journal there, because the journal is about one place.

**`examples/scout/src/WorldWatch.cpp`**

```cpp
void WorldWatch::OnUpdate(const wxl_update_args& args)
{
    // Called once per tick of the client's scheduler, with the seconds since the last one.
    // This is the only clock Scout has, and the only work it does every frame.
    clock_.Advance(args.delta);
}
```

`OnUpdate` is the client's own tick, with the seconds since the last one. It is the only thing
Scout does every frame, and it does almost nothing -- which is the right instinct. Work in a
per-frame hook is work the player pays for sixty times a second.

One caveat, and it is an honest one: **on the stock `Wow.exe` the tick does not reach a mod
today.** The client's dispatch function takes its context in a register rather than on the
stack, so the runtime cannot stand in front of it without handing it back a context it no
longer has. `OnUpdate` and `OnReady` therefore run on the recompiled client only.

The same is true of the object manager, which costs `OnObjectCreate`, `OnObjectDestroy` and
every object handle on the stock client -- see *What depends on the runtime* in Scout's README
for the whole list. Write these handlers anyway: the contract is the same on both, and a mod
written against it is correct on the runtime that can serve it. Just do not make a mod *depend*
on one until you know which runtime it will meet, which is exactly what `requires` in the
manifest is for.

A watcher does nothing until you register it, which happens once, in `OnLoad`:

```cpp
wxl::ScriptMgr::Add(&world_);
```

---

## 🗒️ 4. Remember things

Here is a file with no WarcraftXL in it at all:

**`examples/scout/src/Journal.h`**

```cpp
/// The kinds of thing worth writing down.
enum class What
{
    Appeared,
    LeftView,
    Died,
    Targeted
};

/// One line of the journal.
struct Entry
{
    What        what  = What::Appeared;
    std::string who   = "?"; ///< the unit's name, or "?" when it could not be read
    uint32_t    level = 0;
    float       at    = 0.0f; ///< Clock::Now() when it happened
};
```

**`examples/scout/src/Journal.h`**

```cpp
class Journal
{
public:
    static constexpr size_t kLimit = 64;

    void Add(Entry entry);
    void Clear() { entries_.clear(); }

    size_t Size() const { return entries_.size(); }
    bool   Empty() const { return entries_.empty(); }

    /// The most recent line, or nullptr while nothing has happened.
    const Entry* Last() const { return entries_.empty() ? nullptr : &entries_.back(); }

    const std::deque<Entry>& Entries() const { return entries_; }

    /// How many lines have ever been written, including the ones that fell off.
    uint32_t Written() const { return written_; }

    /// A word for a kind, for the log and for script.
    static const char* Describe(What what);

    /// Called after every Add. The journal does not know who listens, and does not care.
    void OnChange(std::function<void()> listener) { listener_ = std::move(listener); }

private:
    std::deque<Entry>     entries_;
    uint32_t              written_ = 0;
    std::function<void()> listener_;
};
```

This is the point of the step. **A mod is ordinary C++.** The parts of it that hold your own
data should not know the SDK exists -- they are easier to read, easier to test, and they do not
change when the platform does.

Two decisions worth copying:

- **It is bounded.** An hour in a capital city would otherwise grow this without end, and
  nobody reads the thousandth entry.
- **It notifies without knowing who listens.** `OnChange` takes a callback. The journal has no
  idea that the thing listening is the script API; the mod wires them together in step 8.

---

## 👁️ 5. Look at the world

Now the interesting part. Two ideas carry everything here.

### A handle is not a pointer

Every hook that concerns an object hands you a **handle**: a number carrying a generation. If
you keep it and the creature dies, the handle does not point at whatever memory took its place
-- it answers "gone". So you can keep handles across frames, put them in a `std::map`, compare
them, and nothing you do with a stale one can crash the client.

The price is that every read can fail.

### There are three different kinds of "no"

A read answers one of: here is the value, the object is gone, or this runtime cannot answer at
all. Those are three different situations and `Expected<T>` keeps them apart:

**`examples/scout/src/ObjectWatch.cpp`**

```cpp
/**
 * @brief How far a unit is from the player, or nothing when that cannot be answered.
 *
 * Three things can go wrong and they are not the same thing: there is no object service on
 * this runtime, the player is not in the world, or the unit has gone. Expected<T> keeps
 * them apart; a function returning 0.0f for all three would be a lie that reads as "right
 * on top of you".
 */
wxl::Expected<float> DistanceToPlayer(const wxl::Unit& unit)
{
    const wxl::Player me = wxl::ObjectService().LocalPlayer();

    const wxl::Expected<wxl_vec3> mine = me.Position();
    if (!mine) return wxl::Unexpected{mine.Error()};

    const wxl::Expected<wxl_vec3> theirs = unit.Position();
    if (!theirs) return wxl::Unexpected{theirs.Error()};

    return GroundDistance(*mine, *theirs);
}
```

A function that returned `0.0f` for all three would be a lie that reads as *right on top of
you*. This is the single most important habit to pick up: ask what kind of "no" you got.

When you genuinely do not care, say so in one word:

**`examples/scout/src/ObjectWatch.cpp`**

```cpp
void ObjectWatch::Note(What what, wxl::Unit unit)
{
    // Out of range is the common case in a city, so it is checked first and nothing else is
    // read. A distance that cannot be answered is not a reason to drop the line: being out
    // of the world yourself still makes a death near you worth noting.
    const wxl::Expected<float> distance = DistanceToPlayer(unit);
    if (distance && *distance > kRange) return;

    Entry entry;
    entry.what = what;
    entry.at   = clock_.Now();

    // ValueOr is the right tool here and the wrong one above: a name we could not read is
    // honestly "?", while a distance we could not read is not zero.
    entry.who   = unit.Name().ValueOr(std::string("?"));
    entry.level = unit.Level().ValueOr(0u);

    journal_.Add(std::move(entry));
}
```

`ValueOr` is right for the name -- a name you could not read honestly is `"?"`. It would be
wrong for the distance, which is why the distance is checked properly above. Same tool, two
answers, decided by what the value means.

### Pick what you actually want

**`examples/scout/src/ObjectWatch.cpp`**

```cpp
void ObjectWatch::OnObjectCreate(const wxl_object_args& args)
{
    // Everything the client tracks comes through here: items in your bags, corpses, the
    // doodads of the zone. Scout only wants things that are alive.
    if (args.type != WXL_OBJECT_UNIT && args.type != WXL_OBJECT_PLAYER) return;
    Note(What::Appeared, wxl::Unit(args.object));
}
```

> 🚧 **`ObjectCreate` fires for everything.** Items appearing in your bags, corpses, the
> doodads of the zone, every creature on the far side of the map. Filter early, read late.
> Scout checks the type first, then the distance, and only then reads the name.

And the one hook that needs its empty case thought about:

**`examples/scout/src/ObjectWatch.cpp`**

```cpp
void ObjectWatch::OnTargetChanged(const wxl_target_args& args)
{
    // Clearing the target is a change too, and there is nothing to read then.
    if (args.current == WXL_HANDLE_NONE) return;
    Note(What::Targeted, wxl::Unit(args.current));
}
```

---

## 🔌 6. Talk to the interface

The client has its own Lua. `wxl.script` lets you put functions and events on it, so an addon
can call you and you can tell it something happened.

> 🚧 **The trap that catches everyone.** The client does not have *one* script context. It
> builds one for the login screen, throws it away and builds another for the game interface,
> and does it again every time you go back out to the character select. **Nothing you register
> survives a context.** A mod that registers once, at load, is gone by the time you are in the
> world.

The answer is to register from scratch every time a context appears, which is exactly what the
`ScriptVmReady` hook is for:

**`examples/scout/src/ScriptApi.cpp`**

```cpp
void ScriptApi::OnScriptVmReady(const wxl_script_vm_args& args)
{
    const wxl::ScriptService script;
    if (!script)
    {
        // No scripting on this runtime. Everything else Scout does still works, which is
        // why wxl.script is "optional" in mod.json and not "requires".
        return;
    }

    // Both contexts are served. The login screen is a script context too, and a mod that
    // only registers on the game one is simply absent from half the session.
    const bool glue = args.context == WXL_SCRIPT_CONTEXT_GLUE;

    script.RegisterEvent(kUpdatedEvent);
    script.RegisterFunction<&ScoutCount>("ScoutCount");
    script.RegisterFunction<&ScoutLast>("ScoutLast");
    script.RegisterFunction<&ScoutNearby>("ScoutNearby");
    script.RegisterFunction<&ScoutTraffic>("ScoutTraffic");
    script.RegisterFunction<&ScoutMute>("ScoutMute");
    registered_ = true;

    wxl::Log(WXL_LOG_INFO, kTag,
             glue ? "script API is up on the login screen" : "script API is up in the game");
}
```

Two things to notice. It serves **both** contexts -- the login screen is a script context too,
and a mod that only handles the game one is simply absent from half the session. And it returns
quietly when there is no `wxl.script` at all, because the manifest lists it as `optional`:
everything else Scout does still works.

A registered function looks like this:

**`examples/scout/src/ScriptApi.cpp`**

```cpp
/// ScoutLast() -> what, who, level, seconds. Returns nil when nothing has happened yet.
void ScoutLast(wxl::ScriptCall& call)
{
    const scout::Entry* last = g_journal ? g_journal->Last() : nullptr;
    if (!last)
    {
        call.PushNil();
        return;
    }
    call.PushString(scout::Journal::Describe(last->what));
    call.PushString(last->who);
    call.PushNumber(last->level);
    call.PushNumber(last->at);
}
```

`ScriptCall` is the call: ask it for your arguments, push your results. You can return several
values, which is what Lua expects. When the arguments are wrong, say so:

**`examples/scout/src/ScriptApi.cpp`**

```cpp
/// ScoutTraffic(opcode [, "sent"]) -> how many messages of that opcode crossed.
void ScoutTraffic(wxl::ScriptCall& call)
{
    const std::optional<double> opcode = call.Number(0);
    if (!opcode || *opcode < 0.0)
    {
        // The call still returns normally here; the script side fails once it unwinds,
        // with this message, which is how a Lua error is raised from C++ safely.
        call.Fail("ScoutTraffic(opcode [, \"sent\"]): opcode must be a number");
        return;
    }

    const std::optional<std::string_view> side     = call.String(1);
    const bool                            incoming = !side || *side != "sent";
    const uint32_t                        counted =
        g_net ? g_net->Count(static_cast<uint32_t>(*opcode), incoming) : 0u;
    call.PushNumber(counted);
}
```

`Fail` does not throw and does not return early on its own -- the call finishes normally in
C++, and the script side raises the error once it unwinds. That is how a Lua error crosses the
boundary without an exception crossing it.

Finally, telling the interface that something happened:

**`examples/scout/src/ScriptApi.cpp`**

```cpp
void ScriptApi::Announce() const
{
    if (!registered_) return;

    const Entry* last = g_journal ? g_journal->Last() : nullptr;
    if (!last) return;

    const wxl::ScriptService script;
    script.FireEvent(kUpdatedEvent, {wxl::Value::String(Journal::Describe(last->what)),
                                     wxl::Value::String(last->who),
                                     wxl::Value::Number(last->level)});
}
```

An addon picks that up the way it picks up any other event:

```lua
local f = CreateFrame("Frame")
f:RegisterEvent("SCOUT_UPDATED")
f:SetScript("OnEvent", function(self, event, what, who, level)
    print(who, what)
end)
```

---

## 📡 7. The wire

Every message between the client and the server passes through two hooks. Counting them is
harmless:

**`examples/scout/src/NetWatch.cpp`**

```cpp
void NetWatch::OnMessageReceived(const wxl_message_args& args)
{
    ++incoming_[args.opcode];

    // suppressed is a pointer, not a value: the runtime is asking, not telling. Writing
    // WXL_TRUE through it means the client never sees this message at all -- its own
    // handler does not run. That is how a mod serves an opcode the client does not know;
    // it is also how a mod breaks the client, so it only happens on request.
    if (!IsMuted(args.opcode) || !args.suppressed) return;
    *args.suppressed = WXL_TRUE;
}
```

**`examples/scout/src/NetWatch.cpp`**

```cpp
void NetWatch::OnMessageSent(const wxl_message_args& args)
{
    // Counted, never muted. Dropping something the client chose to say leaves the server
    // with a different idea of the world than the client has, and nothing says which.
    ++outgoing_[args.opcode];
}
```

`suppressed` is a **pointer**, not a value. The runtime is asking, not telling: write
`WXL_TRUE` through it and the client never sees that message -- its own handler does not run.

That is how a mod serves an opcode the client does not know, and it is the most dangerous thing
in this tutorial. An opcode the client needs, dropped, leaves it out of step with the server
and nothing will tell you why. Scout mutes nothing unless something asks for it by number, and
it refuses to mute outgoing messages at all.

---

## 🧩 8. Putting it together

The mod class owns everything and wires it:

**`examples/scout/src/Scout.cpp`**

```cpp
// Declaration order is construction order: the state first, then the watchers that
// borrow it. A watcher holds a reference, never a copy, so there is one journal.
scout::Clock   clock_;
scout::Journal journal_;

scout::WorldWatch  world_{clock_, journal_};
scout::ObjectWatch objects_{clock_, journal_};
scout::NetWatch    net_;
scout::ScriptApi   script_{journal_, net_};
```

**`examples/scout/src/Scout.cpp`**

```cpp
/**
 * @brief Set-up. Nothing the client brings up exists yet -- not the world, not the
 *        interface, not a single file.
 *
 * So the only thing done here is registering: saying what to call later. Anything that
 * needs the client to be further along waits for the stage that brings it up, or for
 * the event that announces it.
 */
wxl_result OnLoad() override
{
    journal_.OnChange([this] { script_.Announce(); });

    wxl::ScriptMgr::Add(&world_);
    wxl::ScriptMgr::Add(&objects_);
    wxl::ScriptMgr::Add(&net_);
    wxl::ScriptMgr::Add(&script_);

    wxl::Log(WXL_LOG_INFO, scout::kTag, "watching");
    return WXL_OK;
}
```

`OnLoad` runs before the client has done *anything*. There is no world, no interface, not a
file open. So the only thing that belongs here is registering -- saying what to call later.

When you need more than that, wait for the stage that brings it up:

| Override | When |
|---|---|
| `OnLoad` | you are being started. Nothing exists yet |
| `OnStorageReady` | archives are mounted, files can be read |
| `OnGraphicsReady` | there is a graphics device |
| `OnScriptReady` | a script context exists |
| `OnReady` | fully booted, before the first frame |
| `OnShutdown` | coming down |
| `OnUnload` | you are being removed |

> ⚠️ A runtime reports the stages it can recognise and no others. Today both of them report
> every stage above except `OnGraphicsReady`, which neither can honestly answer for. An
> override that is never called is not an error -- it is that runtime saying it cannot tell
> you. Write your mod so that nothing it does depends on a stage it has not been given, and
> it will work everywhere.

And the declaration, once, in one file:

**`examples/scout/src/Scout.cpp`**

```cpp
// The four entry points a runtime looks for, written for you. The id must be the one in
// mod.json, to the letter.
WXL_MOD(Scout, "wxl.scout", "Scout", 1)
```

---

## 🔨 9. Build it

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

For one architecture only, name it: `TARGETS win-x86`.

## 🚀 10. Install and run it

Both clients read a `Mods/` folder beside the executable, and both want the same shape. Copy
the whole `dist/<id>/` directory in; do not flatten it.

**The stock client (32-bit)**

```
<your WoW folder>/
    Wow.exe
    WarcraftXL.dll            from core: build/dist/bin/x86/
    Mods/
        wxl.scout/
            mod.json
            bin/x86/scout.dll
```

The executable carries the runtime's import once:

```sh
wxl-core-patcher.exe Wow.exe
```

It keeps the original as `Wow.exe.orig` and is safe to run twice.

**The recompiled client (64-bit)**

```
<client folder>/
    WowClient64.exe
    WarcraftXL.dll            built with the client
    Mods/
        wxl.scout/
            mod.json
            bin/x64/scout.dll
```

Nothing to patch: the client calls the runtime itself.

**Then look at it.** `Logs/WarcraftXL.log` is written one line at a time, so a crash keeps
everything up to it. In the game, with any script console:

```lua
print(ScoutCount(), "notes")

local what, who, level = ScoutLast()
if what then print(who, "("..level..")", what) end

print(ScoutNearby(), "units in view")
```

---

## 🧰 When it does not work

| What you see | What it means |
|---|---|
| nothing at all in the log | the folder shape is wrong. It must be `Mods/<id>/mod.json` and `Mods/<id>/bin/<arch>/<name>.dll` |
| `refused ...: requires service ... which this runtime does not have` | your `requires` cannot be met here. Move it to `optional` if you can work without it |
| `refused ...: abi 0.3, this runtime serves 0.2` | you built against a newer SDK than the runtime. Rebuild, or update the runtime |
| the build fails on the `abi` field | `mod.json` and the SDK headers disagree. Set `abi` to what the error names |
| the client does not start at all | the DLL beside the executable is the wrong architecture |
| the mod loads but nothing happens | an event that runtime does not raise. Its README lists what it raises today |
| your Lua functions are gone after logging in | you registered once instead of on every `ScriptVmReady`. See step 6 |
| a read always answers "unsupported" | that service is not published here. `requires` it if you need it |

---

## 🧭 Where to go next

- **Read the whole example.** [`examples/scout/`](../examples/scout) is six short files and
  every one of them is quoted above.
- **Change one thing.** Make the range smaller. Note what a unit is wearing with
  `Player::VisibleItemId`. Fire the event only for deaths.
- **[`docs/building-a-mod.md`](building-a-mod.md)** is the reference for the build: one
  architecture, extra sources, installing the SDK.
- **The runtime's own README** lists every event it raises and every service it publishes, for
  that runtime. There is no single answer: that is what `requires` and `optional` are for.

### Four rules worth keeping

**Nothing of C++ crosses the boundary.** Your classes, exceptions and standard library stay
inside your DLL. Never let an exception escape a hook.

**A handle is not a pointer.** It carries a generation, so one kept past its subject's death
answers "invalid" rather than reading whatever took its place. Keep handles, not addresses.

**Ids are forever.** Event ids and stage numbers are appended, never renumbered. A mod built
today keeps working against a later runtime of the same major version.

**Ask, do not assume.** Require what you truly cannot work without, ask for the rest, and say
something useful in the log when it is not there. "This mod does not run here, and here is
why" is a good error. "It loaded and did nothing" is a bug report nobody can answer.

