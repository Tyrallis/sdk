# Your first mod

This guide builds one mod from an empty folder: **Scout**. Scout notes the creatures and players
that appear, leave and die near you, keeps the last few dozen of those notes in a journal, hands
the journal to the game's interface through functions an addon can call, and counts the messages
the client exchanges with the server.

Each step adds one file, builds, and shows you what changed in the game or in its log. Along the
way the guide explains every idea the code relies on, the first time it comes up. The finished mod
is [`examples/scout/`](../examples/scout); every block below that names a file of it is that
file's text, checked against it by the SDK's tests.

You need to know some C++: classes and virtual functions, references, `std::string` and the
standard containers. You do not need to know how World of Warcraft works inside, how a DLL is
made, or anything about this project.

- [What a mod is](#what-a-mod-is)
- [1. Get set up](#1-get-set-up)
- [2. The smallest mod](#2-the-smallest-mod)
- [3. A journal, in plain C++](#3-a-journal-in-plain-c)
- [4. React to the world](#4-react-to-the-world)
- [5. Look at the units around you](#5-look-at-the-units-around-you)
- [6. Count the traffic](#6-count-the-traffic)
- [7. Talk to the interface](#7-talk-to-the-interface)
- [8. Put it together](#8-put-it-together)
- [9. Build, install, try it](#9-build-install-try-it)
- [When something goes wrong](#when-something-goes-wrong)
- [Where to go next](#where-to-go-next)

---

## What a mod is

WarcraftXL runs mods inside the World of Warcraft 3.3.5a client, and that client comes in two
forms:

- **the stock client**, the original `Wow.exe`. It is a 32-bit program (the *x86* architecture).
  WarcraftXL gets into it as `WarcraftXL.dll`, which the executable is patched once to load.
- **the recompiled client**, `WowClient64.exe`, built from source as a 64-bit program (the *x64*
  architecture). It calls WarcraftXL itself; nothing is patched.

In both, the part of WarcraftXL that lives inside the game is called **the runtime**. It finds the
mods, loads them, and calls into them when something happens in the game.

A **mod** is a folder in `Mods/`, beside the game's executable:

```
Mods/
    wxl.scout/
        mod.json             the manifest: who the mod is and what it needs
        bin/x64/scout.dll    the code, for the recompiled client
        bin/x86/scout.dll    the same code, for the stock client
```

A **DLL** is a library that Windows loads into a program that is already running. A 32-bit program
can only load 32-bit DLLs and a 64-bit one only 64-bit DLLs, which is why a mod carries its code
twice: one source, built once per architecture. The build does both for you.

Between your DLL and the runtime, only plain C crosses: functions and structs. The SDK is a set of
headers that turns that C into C++ inside your DLL: you derive from classes, override virtual
functions, and receive ordinary C++ values. Nothing of your C++ reaches the runtime, so your mod
may be built with a different compiler than the runtime, and a C++ exception must never travel
out of your code (more on that in [step 8](#what-the-runtime-does-when-your-code-fails)).

---

## 1. Get set up

You need:

- **Visual Studio 2022 or newer**, with the *Desktop development with C++* workload and the
  component *MSVC v14x - C++ x64/x86 build tools*, which carries both compilers. The Build Tools
  edition is enough.
- **CMake 3.21 or newer.**
- **Git**, to fetch the SDK.

Fetch the SDK:

```sh
git clone https://github.com/WarcraftXL/sdk.git
```

Before you write anything, build the finished example. If it builds, your tools work, and anything
that fails later comes from your own code:

```sh
cd sdk/examples/scout
cmake -S . -B build
cmake --build build --config Release
```

The first command *configures*: CMake reads `CMakeLists.txt` and writes a Visual Studio solution
into `build/`. The second compiles, in the Release configuration. When it finishes you have:

```
build/dist/wxl.scout/
    mod.json
    bin/x64/scout.dll
    bin/x86/scout.dll
```

That folder is the whole mod. `build/` also holds one build tree per architecture,
`build/win-x64/` and `build/win-x86/`; you will not need to open them.

Two cases need something else:

- **CMake picked Ninja** (some editors set it for you): Ninja builds one architecture per tree, and
  the configure step says so. Pass `-G "Visual Studio 17 2022"` to the first command, or see
  [Building one architecture](building-a-mod.md#building-one-architecture).
- **You build from Linux**: set up clang-cl and the Windows SDK, and export the toolchain file, as
  the README says in [Run the tests from Linux](../README.md#run-the-tests-from-linux). Then
  configure one tree per architecture and install each into the mod folder:

  ```sh
  cmake -S . -B build-x64 -G Ninja -DCMAKE_BUILD_TYPE=Release -DWXL_MOD_ARCH=win-x64
  cmake --build build-x64
  cmake --install build-x64 --prefix dist/wxl.scout
  ```

  Then the same with the x86 toolchain file and `win-x86`, and copy `mod.json` into
  `dist/wxl.scout/`.

---

## 2. The smallest mod

Make a folder of your own, outside the SDK, with three files:

```
scout/
    CMakeLists.txt
    mod.json
    src/Scout.cpp
```

### The build file

```cmake
cmake_minimum_required(VERSION 3.21)

# No language here: wxl_add_mod enables C++ in each per-architecture build.
project(scout VERSION 1.0.0 LANGUAGES NONE)

# Where the SDK is. The default is this checkout; pass -DWXL_SDK_DIR=<path> for another one.
set(WXL_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Path to the WarcraftXL SDK")
include("${WXL_SDK_DIR}/cmake/wxl_add_mod.cmake")

wxl_add_mod(scout
    SOURCES
        src/Scout.cpp
    MANIFEST mod.json
    TARGETS win-x64 win-x86)
```

This is Scout's own `CMakeLists.txt` with one source so far; you will add a line under `SOURCES`
for each file the next steps create.

- `WXL_SDK_DIR` says where the SDK is. The default suits the example, which sits two folders
  inside the SDK. Your folder is elsewhere, so you will pass the path when you configure.
- `include(...)` brings in `wxl_add_mod`, the SDK's one CMake function.
- `wxl_add_mod(scout ...)` is the entire build. It checks `mod.json`, configures one build per
  architecture in `TARGETS`, compiles `SOURCES` in each against the SDK headers, and collects both
  DLLs and the manifest into `build/dist/<id>/`. The first argument, `scout`, names the DLL:
  `scout.dll`.
- `LANGUAGES NONE` is on purpose: this top level only drives the two architecture builds, which
  enable C++ themselves.

### The manifest

**`examples/scout/mod.json`**

```json
{
  "$schema": "../../schema/mod.json",
  "id": "wxl.scout",
  "name": "Scout",
  "version": "1.0.0",
  "abi": "0.1.5",
  "description": "Keeps a journal of the units around you and hands it to the interface's Lua.",
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

The runtime reads this file before it runs any of your code, and decides from it whether to load
the mod at all.

| Field | What it means |
|---|---|
| `$schema` | the SDK's description of this file, so an editor can complete and check it. In your folder, make it the path to `schema/mod.json` in your SDK checkout |
| `id` | the mod's identity: lowercase letters, digits, dots and dashes. The folder in `Mods/` is named after it and every log line about the mod uses it. Scout's is `wxl.scout`; for a mod of your own, pick a prefix that is yours, such as `yourname.scout` |
| `name`, `version`, `description` | for people: the log and the tools show them |
| `abi` | the version of the contract between mods and runtimes that the mod needs, `major.minor.feature` |
| `binaries` | where the DLL for each architecture is, inside the folder. The build checks that these are the paths it writes |
| `requires` | services the mod cannot work without |
| `optional` | services the mod uses when they are there |

**The `abi` version.** The SDK headers carry a version, the one in `include/wxl/abi.h`. A runtime
loads a mod when the major and minor parts match its own and the feature part is no higher; a
newer feature means something was added to the contract, an event or a service. Write the lowest
version that has everything your mod uses. Scout uses nothing added after `0.1.5`, so it asks for
`0.1.5` and loads on every runtime of the `0.1` line from that one on. When in doubt, write the
version of the headers you build against: the build accepts any version of the same major and
minor up to theirs, and refuses the rest with a message saying what to write.

**Services.** Beyond telling you what happens, a runtime offers *services*: sets of functions it
publishes under a name and a version. Scout uses two. `wxl.object` reads the world's objects,
their names, levels and positions, and Scout is pointless without it, so it is in `requires`: a
runtime that lacks it refuses Scout, and its log says why, instead of loading a mod that would do
nothing. `wxl.script` reaches the client's own Lua; Scout works without it, so it is in
`optional`. You will meet both in steps 5 and 7.

### The code

**`src/Scout.cpp`**, for now:

```cpp
#include "wxl/sdk.hpp"

namespace
{
    class Scout final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            wxl::Log(WXL_LOG_INFO, "scout", "hello from Scout");
            return WXL_OK;
        }
    };
} // namespace

WXL_MOD(Scout, "wxl.scout", "Scout", 1)
```

- `wxl/sdk.hpp` is the SDK's C++ layer. It is header-only: there is no library to link.
- **`wxl::Mod`** is the mod as the runtime sees it. You derive from it and override the moments
  you care about. `OnLoad` is the first: the runtime calls it once, right after loading your DLL.
  Returning anything but `WXL_OK` refuses the load: the runtime logs the result and keeps
  nothing of the mod.
- **`wxl::Log`** writes one line to the runtime's log: a level (`WXL_LOG_DEBUG`, `_INFO`, `_WARN`,
  `_ERROR`), a tag that tells your lines apart from everyone else's, and the text.
- **`WXL_MOD`** declares the mod. A runtime looks for four functions in your DLL by name, to ask
  who you are, to load you, to tell you how far the client has started, and to unload you; this
  macro writes all four around your class. It takes the class, the id, a display name and the
  binary's own version number, a plain integer. Put it once, in one source file. **The id must be
  the manifest's, letter for letter**: when they differ the runtime refuses the mod and logs both.
- The anonymous namespace keeps `Scout` private to this file; nothing else needs to see it.

### Build it

From your `scout/` folder, with the path to your SDK checkout:

```sh
cmake -S . -B build -DWXL_SDK_DIR=C:/path/to/sdk
cmake --build build --config Release
```

`build/dist/wxl.scout/` now holds `mod.json` and the two DLLs.

### Install it and run

Copy the whole `build/dist/wxl.scout/` folder into `Mods/` beside the game's executable. Keep the
folder as it is: the runtime looks for `Mods/<id>/mod.json` and takes the DLL the manifest names
for its own architecture.

For the stock client, the game folder also needs `WarcraftXL.dll` beside `Wow.exe`, and `Wow.exe`
patched once to load it; the runtime's README, *Into the stock client (32-bit)*, has the two
steps. The
recompiled client comes with its runtime and needs neither.

Start the game, wait for the login screen, and quit. Then open `Logs/WarcraftXL.log` beside the
executable. Among the runtime's own lines are these two (each line starts with the time, left out
here):

```
[INFO ] [scout] hello from Scout
[INFO ] [host] loaded wxl.scout 1.0.0 (Scout) from bin/x86/scout.dll
```

The first is yours. The second is the runtime's mod loader, which logs under the tag `host`; it
comes after yours because the loader reports a mod once its `OnLoad` has returned `WXL_OK`. On the
recompiled client the path reads `bin/x64/scout.dll`. If neither line is there, see
[When something goes wrong](#when-something-goes-wrong).

That is a mod. The steps that follow make it do something.

### When your code runs

`OnLoad` runs before the client has done anything: no files are open, there is no window, no
interface, no world. Code that needs one of those waits for the moment the client brings it up.
`wxl::Mod` has a virtual function for each of these **stages**, called in this order:

| Override | When |
|---|---|
| `OnLoad` | right after the DLL is loaded. Register what you will need; nothing else exists yet |
| `OnEarly` | the runtime is up, the client has not started |
| `OnStorageReady` | the game's archives are open and files can be read |
| `OnGraphicsReady` | there is a graphics device |
| `OnScriptReady` | the client's Lua exists |
| `OnReady` | the client is fully started, before its first frame |
| `OnShutdown` | the client is closing |
| `OnUnload` | last, right before the DLL is unloaded |

Both runtimes report every stage above. A runtime reports only the stages it can recognise, so a
mod should not rely on one that may never come: Scout registers everything in `OnLoad`, and uses
`OnReady` and `OnShutdown` only to write a line to the log.

---

## 3. A journal, in plain C++

Before Scout looks at the game, it needs somewhere to keep what it sees. That part has nothing to
do with WarcraftXL, and that is the lesson of this step: **a mod is an ordinary C++ program**. The
code that holds your own data does not need the SDK, so it stays easy to read, to test on its own,
and unchanged when the SDK changes.

Create `src/Journal.h`. It opens with a constant every file of Scout uses, and a clock:

**`examples/scout/src/Journal.h`**

```cpp
/// The tag on every line Scout writes to the log.
inline constexpr const char* kTag = "scout";

/// Seconds since the mod started, advanced by the client's tick.
class Clock
{
public:
    void  Advance(float seconds) { now_ += seconds; }
    float Now() const { return now_; }

private:
    float now_ = 0.0f;
};
```

The clock does not read the system time: in the next step the client's own tick advances it, so
Scout's idea of time is the game's.

Then what a line of the journal holds:

**`examples/scout/src/Journal.h`**

```cpp
/// The kinds of thing Scout writes down.
enum class What
{
    Appeared,
    Left,
    Died,
    Targeted
};

/// The word for a kind, as the log and the interface show it.
const char* Describe(What what);

/// One line of the journal.
struct Entry
{
    What        what  = What::Appeared;
    std::string who   = "?";  ///< the unit's name, or "?" when it could not be read
    uint32_t    level = 0;    ///< 0 when it could not be read
    float       at    = 0.0f; ///< Clock::Now() when it happened
};
```

And the journal itself:

**`examples/scout/src/Journal.h`**

```cpp
/// The last kLimit lines, oldest first. A line added at the limit drops the oldest.
class Journal
{
public:
    static constexpr std::size_t kLimit = 64;

    /// Appends a line, then calls the listener with it.
    void Add(Entry entry);
    void Clear() { entries_.clear(); }

    std::size_t Size() const { return entries_.size(); }

    /// The line @p back steps from the newest, 0 being the newest; null past the oldest.
    const Entry* FromNewest(std::size_t back) const;

    /// How many lines were ever added, the dropped ones included.
    uint32_t Written() const { return written_; }

    /// Sets the function Add calls after each new line.
    void OnAdd(std::function<void(const Entry&)> listener) { listener_ = std::move(listener); }

private:
    std::deque<Entry>                 entries_;
    uint32_t                          written_ = 0;
    std::function<void(const Entry&)> listener_;
};
```

Two choices worth copying into your own mods:

- **It is bounded.** A long evening in a crowded city would otherwise grow it without end, and
  nobody reads the thousandth line. A `std::deque` adds at one end and drops at the other cheaply.
- **It does not know who listens.** `OnAdd` takes any function to call after each new line. In
  step 7 the interface will listen; the journal does not need to know that.

The full header, with its include guard, its `#include`s and the `scout` namespace around all of
this, is [`examples/scout/src/Journal.h`](../examples/scout/src/Journal.h). The definitions go in
`src/Journal.cpp`:

**`examples/scout/src/Journal.cpp`**

```cpp
void Journal::Add(Entry entry)
{
    entries_.push_back(std::move(entry));
    if (entries_.size() > kLimit) entries_.pop_front();
    ++written_;
    if (listener_) listener_(entries_.back());
}

const Entry* Journal::FromNewest(std::size_t back) const
{
    if (back >= entries_.size()) return nullptr;
    return &entries_[entries_.size() - 1 - back];
}
```

`Describe` is a `switch` that turns each `What` into its word; it is in the same file. Add
`src/Journal.cpp` under `SOURCES` in `CMakeLists.txt` and build again: nothing in the game
changes yet, but the journal compiles.

---

## 4. React to the world

### Events and scripts

Things happen in the game: the client ticks, you enter a world, a creature comes into view, a
message arrives from the server. Each of these is an **event**, and the runtime raises it to every
mod that asked for it.

Events come in **families**, one per part of the game: `WorldScript` (the client's tick, entering
and leaving a world, input, sounds), `ObjectScript` (objects appearing, leaving, dying, being
targeted), `NetScript` (messages to and from the server), `LuaScript` (the client's Lua), and
`RenderScript`, `AssetScript`, `CombatScript`, `PlayerScript`, `GlueScript` and `ChatScript`.
Each family is a class with one empty virtual function per event, named `On` and the event:
`OnUpdate`, `OnWorldEnter`, `OnWorldLeave`, and so on.

You write a **script**: a class that derives from one family and overrides the events you want.
The ones you leave alone cost nothing; the runtime never calls into your mod for them.

### The world watcher

Scout's first script keeps the clock and starts a new journal on each world you enter. Create
`src/WorldWatch.h`:

**`examples/scout/src/WorldWatch.h`**

```cpp
/// Keeps the clock and starts a new journal on every world Scout enters.
class WorldWatch final : public wxl::WorldScript
{
public:
    WorldWatch(Clock& clock, Journal& journal) : clock_(clock), journal_(journal) {}

    void OnUpdate(float delta) override;
    void OnWorldEnter(uint32_t map_id, wxl_vec3 position) override;
    void OnWorldLeave(uint32_t map_id, wxl_vec3 position) override;

private:
    Clock&   clock_;
    Journal& journal_;
};
```

**Parameters.** An event carries data: entering a world carries the map's id and where the player
stands. A handler receives that data as its parameters. They are listed, event by event, in
[`include/wxl/fields.def`](../include/wxl/fields.def); entering and leaving a world carry
`wxl_world_args`:

**`include/wxl/fields.def`**

```cpp
WXL_ARGS(wxl_world_args)
WXL_ARGS_FIELD(wxl_world_args, map_id, u32)
WXL_ARGS_FIELD(wxl_world_args, position, vec3) ///< where the player is, or was
WXL_ARGS_END(wxl_world_args)
```

so `OnWorldEnter` takes a `uint32_t map_id` and a `wxl_vec3 position` (a struct of three floats,
`x`, `y`, `z`), in that order. The comment at the top of `fields.def` gives the C++ type of each
kind. `override` makes the compiler check the list: get a parameter wrong and the build fails,
instead of leaving a function that is never called.

**References.** The watcher holds a `Clock&` and a `Journal&`: it borrows them. There is one clock
and one journal in the whole mod, and the mod class owns them; every script that needs them gets a
reference. The mod class outlives its scripts, so the references stay good.

Now the handlers, in `src/WorldWatch.cpp`:

**`examples/scout/src/WorldWatch.cpp`**

```cpp
void WorldWatch::OnUpdate(float delta)
{
    clock_.Advance(delta);
}

void WorldWatch::OnWorldEnter(uint32_t map_id, wxl_vec3 position)
{
    journal_.Clear();
    wxl::Log(WXL_LOG_INFO, kTag,
             "entered map " + std::to_string(map_id) + " at " +
                 std::to_string(static_cast<int>(position.x)) + ", " +
                 std::to_string(static_cast<int>(position.y)));
}

void WorldWatch::OnWorldLeave(uint32_t map_id, wxl_vec3)
{
    wxl::Log(WXL_LOG_INFO, kTag,
             "leaving map " + std::to_string(map_id) + " with " +
                 std::to_string(journal_.Size()) + " line(s) in the journal");
}
```

- `OnUpdate` is the client's tick, many times a second, with the seconds since the last one. It is
  the only work Scout does on every tick, and it is one addition. Keep it that way in your own
  mods: whatever a per-tick handler does, the player pays for at every frame.
- `OnWorldEnter` comes once the world is loaded and the player is in it: after logging in, and
  after every loading screen. Scout empties the journal there, so what it holds is about where you
  are.
- `OnWorldLeave` does not use `position`, so the parameter has no name in the definition: C++
  allows that, and it says "not used" without a compiler warning.

### Register the script

A script does nothing until the mod registers it. `src/Scout.cpp` becomes:

```cpp
#include "wxl/sdk.hpp"

#include "Journal.h"
#include "WorldWatch.h"

namespace
{
    class Scout final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            wxl::ScriptMgr::Add(&world_);
            wxl::Log(WXL_LOG_INFO, scout::kTag, "watching");
            return WXL_OK;
        }

    private:
        scout::Clock      clock_;
        scout::Journal    journal_;
        scout::WorldWatch world_{clock_, journal_};
    };
} // namespace

WXL_MOD(Scout, "wxl.scout", "Scout", 1)
```

The mod now owns a clock, a journal and a watcher, and registers the watcher in `OnLoad`.

**`wxl::ScriptMgr::Add`** looks at which events the script overrides, at compile time, and asks
the runtime for those events only: `WorldWatch` gets `OnUpdate`, `OnWorldEnter` and
`OnWorldLeave`, and the other events of the family never reach it. Give it the script itself,
`&world_`; through a pointer to the family it could no longer see what is overridden, and would
take every event of the family.

**Member order.** C++ builds members in the order they are declared and destroys them in the
reverse order. The clock and the journal come first, so they exist when `world_` is built with
references to them, and they still exist while `world_` is destroyed.

Add `src/WorldWatch.cpp` under `SOURCES`, build, copy the folder into `Mods/` again (close the
game first: Windows keeps a loaded DLL locked), and log in with a character. The log now has:

```
[INFO ] [scout] entered map 0 at -8949, -132
```

with your own map and position: map 0 is the Eastern Kingdoms, 1 Kalimdor, 571 Northrend.

---

## 5. Look at the units around you

### Handles and objects

When an event is about an object of the game, a creature, a player, a chest, it gives you a
`wxl::Object`. Inside it is a **handle** (`wxl_handle`): a number that names the object. It is not a
pointer to the client's memory. It carries a generation, so a handle kept after its object is gone
does not reach whatever took that object's place; it answers that the object is gone. You can keep
objects across frames, store them in containers and compare them, and a stale one cannot crash
the client.

A handle names the object for as long as it stays in view. If a creature walks out of range and
comes back, the client announces it again, with a new handle: compare its `Guid()`, the server's
own id, to recognise it.

`wxl::Object` reads what every object has, such as its name and position. Below it are the
classes of `wxl/objects.hpp` for each kind: `wxl::Unit` (a creature or a player), `wxl::Player`,
`wxl::GameObject` and others. `wxl::Unit(object)` views an object you know to be a unit as a unit
and costs nothing; `object.As<wxl::Unit>()` checks first. Each read, such as `unit.Name()`, asks
the `wxl.object` service.

### A read can fail, in more than one way

Any read can find no answer, for different reasons: the object is gone (its handle is stale), the
object has no such thing (an item has no position in the world), or the runtime does not publish
`wxl.object` at all. A read therefore answers `wxl::Expected<T>`: either the value, or the reason
there is none.

```cpp
const wxl::Expected<uint32_t> health = unit.Health();
if (health)                         // a value, possibly 0: the unit is dead
    wxl::Log(WXL_LOG_INFO, "scout", *health == 0 ? "dead" : "alive");
else if (health.IsNotFound())       // the unit is gone
    wxl::Log(WXL_LOG_INFO, "scout", "gone");
else if (health.IsUnsupported())    // this runtime cannot read units
    wxl::Log(WXL_LOG_INFO, "scout", "no wxl.object here");
```

`health.Error()` gives the reason as a `wxl_result` code. `health.ValueOr(0)` gives the value, or
`0` when there is none, for when you do not care why.

### The unit watcher

Scout notes the units that appear, leave and die within 60 yards, and every new target. Create
`src/UnitWatch.h`:

**`examples/scout/src/UnitWatch.h`**

```cpp
/// Writes down the units that appear, leave and die near the player, and each new target.
class UnitWatch final : public wxl::ObjectScript
{
public:
    /// How close, in yards on the ground, a unit has to be for Scout to note it.
    static constexpr float kRange = 60.0f;

    UnitWatch(const Clock& clock, Journal& journal) : clock_(clock), journal_(journal) {}

    void OnObjectCreate(wxl::Object object, wxl_guid guid, uint32_t type) override;
    void OnObjectDestroy(wxl::Object object, wxl_guid guid, uint32_t type) override;
    void OnUnitDeath(wxl::Object unit) override;
    void OnTargetChanged(wxl::Object previous, wxl::Object current) override;

private:
    /// Adds a line about @p unit to the journal.
    void Note(What what, const wxl::Unit& unit);

    const Clock& clock_;
    Journal&     journal_;
};
```

Its header includes `wxl/objects.hpp` beside `wxl/sdk.hpp`. Besides the object, an object event
carries its `wxl_guid`, the server's own id for it, and its type, one of the `WXL_OBJECT_...`
values.

`src/UnitWatch.cpp` starts with the question every handler below asks, how far a unit is from the
player:

**`examples/scout/src/UnitWatch.cpp`**

```cpp
/// How far @p unit is from the player, or why that cannot be read.
wxl::Expected<float> DistanceToPlayer(const wxl::Unit& unit)
{
    const wxl::Expected<wxl_vec3> mine = wxl::Player::Local().Position();
    if (!mine) return wxl::Unexpected{mine.Error()};

    const wxl::Expected<wxl_vec3> theirs = unit.Position();
    if (!theirs) return wxl::Unexpected{theirs.Error()};

    return GroundDistance(*mine, *theirs);
}

/// Whether @p unit is known to be within range. A distance that cannot be read is not near.
bool IsNear(const wxl::Unit& unit)
{
    const wxl::Expected<float> distance = DistanceToPlayer(unit);
    return distance && *distance <= scout::UnitWatch::kRange;
}
```

`wxl::Player::Local()` is you. `DistanceToPlayer` passes a failed read on with
`wxl::Unexpected{...}`, so the caller still learns why there is no distance. A function that
answered `0.0f` when it could not tell would say "right beside you" for a unit that is gone. `IsNear`
makes the decision explicit: only a unit known to be within range counts. `GroundDistance`, above
them in the file, is the distance on the ground, height left out.

The handlers:

**`examples/scout/src/UnitWatch.cpp`**

```cpp
void UnitWatch::OnObjectCreate(wxl::Object object, wxl_guid, uint32_t type)
{
    if (!wxl::Unit::Accepts(type)) return;

    const wxl::Unit unit(object);
    if (unit == wxl::Player::Local()) return;
    if (IsNear(unit)) Note(What::Appeared, unit);
}

void UnitWatch::OnObjectDestroy(wxl::Object object, wxl_guid, uint32_t type)
{
    if (!wxl::Unit::Accepts(type)) return;

    const wxl::Unit unit(object);
    if (IsNear(unit)) Note(What::Left, unit);
}

void UnitWatch::OnUnitDeath(wxl::Object unit)
{
    if (IsNear(wxl::Unit(unit))) Note(What::Died, wxl::Unit(unit));
}

void UnitWatch::OnTargetChanged(wxl::Object, wxl::Object current)
{
    if (!current.HasHandle()) return;
    Note(What::Targeted, wxl::Unit(current));
}
```

- **`ObjectCreate` comes for everything the client tracks**: the items in your bags, every chest
  and door of the zone, every creature in sight. Scout tests the type first, which costs nothing:
  `wxl::Unit::Accepts(type)` is true for the types a unit can be, a creature or a player. Then it
  tests the distance, and reads the name only for what it keeps.
- `unit == wxl::Player::Local()` compares two objects by handle: you appear too when you log in,
  and Scout does not note you.
- An object being destroyed is still there during `OnObjectDestroy`, so its position can be read.
- `OnTargetChanged` gives the previous target and the new one; an object with no handle
  (`HasHandle()` is false) means none, which is what you get when you clear your target. A target is noted at any distance: you chose it.

And the line itself:

**`examples/scout/src/UnitWatch.cpp`**

```cpp
void UnitWatch::Note(What what, const wxl::Unit& unit)
{
    Entry entry;
    entry.what  = what;
    entry.who   = unit.Name().ValueOr("?");
    entry.level = unit.Level().ValueOr(0u);
    entry.at    = clock_.Now();
    journal_.Add(std::move(entry));
}
```

Here `ValueOr` is the right tool: a name that cannot be read is honestly `"?"`, and the journal
line is still worth keeping. For the distance it was not, because a missing distance is not zero.
The same tool, two decisions, made by what the value means.

Add `src/UnitWatch.cpp` under `SOURCES`. In `Scout.cpp`, include `"UnitWatch.h"`, add the
member `scout::UnitWatch units_{clock_, journal_};` after `world_`, and register it in `OnLoad`
with `wxl::ScriptMgr::Add(&units_);`.

The journal now fills as you walk around. The log does not show it; step 7 lets you read it in
the game.

---

## 6. Count the traffic

Every message between the client and the server crosses two events of the `NetScript` family:
`MessageReceived` for what the server sends, `MessageSent` for what the client sends. Scout counts
them, by opcode (the number at the head of each message that says what it is) and by direction,
and can be asked to drop an incoming opcode.

### Threads

The client does most of its work on one thread, its main thread, and events are raised there.
A few are not: [`include/wxl/events.def`](../include/wxl/events.def) says so on their row, and
`MessageSent` is one of them:

**`include/wxl/events.def`**

```cpp
/** A message is about to go to the server. May be raised on the connection's own thread. */
WXL_EVENT(Net, MessageSent,     24, wxl_message_args)
```

So `OnMessageSent` may run at the same moment as code of yours on the main thread. Data that both
touch needs a lock, or the two can corrupt it. Scout's tallies are written by both handlers and
read from the interface on the main thread, so they live under a `std::mutex`.

Create `src/Traffic.h`:

**`examples/scout/src/Traffic.h`**

```cpp
/// Which way a message went.
enum class Direction
{
    Received,
    Sent
};

/// How much of one opcode went one way.
struct Tally
{
    uint32_t messages = 0;
    uint64_t bytes    = 0;
};
```

**`examples/scout/src/Traffic.h`**

```cpp
class Traffic final : public wxl::NetScript
{
public:
    void OnMessageReceived(uint32_t opcode, wxl::Bytes data, wxl::Answer suppressed) override;
    void OnMessageSent(uint32_t opcode, wxl::Bytes data, wxl::Answer suppressed) override;

    /// What has crossed so far for that opcode, that way.
    Tally Count(uint32_t opcode, Direction direction) const;

    /// Starts or stops dropping an incoming opcode. Answers whether it was dropped before.
    bool Drop(uint32_t opcode, bool drop);

private:
    void Record(Direction direction, uint32_t opcode, uint32_t size);

    mutable std::mutex        lock_;
    std::map<uint32_t, Tally> received_; ///< under lock_
    std::map<uint32_t, Tally> sent_;     ///< under lock_
    std::set<uint32_t>        dropped_;  ///< main thread only
};
```

`lock_` is `mutable` so that `Count`, a `const` function, can still take it. The set of dropped
opcodes needs no lock: only `OnMessageReceived` and `Drop` touch it, and both run on the main
thread.

### Two new parameter types

A message event carries `wxl_message_args`, whose members are the opcode, the body (`data` and its
`size`) and a flag named `suppressed`. They reach a handler as three parameters:

- **`wxl::Bytes data`** is the message's body as read-only bytes, `data.size()` long. It is valid
  during the call only: copy what you want to keep. Its length is not a parameter of its own,
  since `data` carries it.
- **`wxl::Answer suppressed`** is a question the runtime asks the handlers. `suppressed.Set()`
  answers yes: the message goes no further. `suppressed.Value()` tells whether a handler before
  yours already said so.

`src/Traffic.cpp`:

**`examples/scout/src/Traffic.cpp`**

```cpp
void Traffic::OnMessageReceived(uint32_t opcode, wxl::Bytes data, wxl::Answer suppressed)
{
    Record(Direction::Received, opcode, data.size());

    // Setting the answer keeps the message from the client: its own handler never runs.
    if (dropped_.count(opcode) != 0) suppressed.Set();
}

void Traffic::OnMessageSent(uint32_t opcode, wxl::Bytes data, wxl::Answer)
{
    Record(Direction::Sent, opcode, data.size());
}

void Traffic::Record(Direction direction, uint32_t opcode, uint32_t size)
{
    const std::lock_guard<std::mutex> hold(lock_);
    Tally& tally = (direction == Direction::Received ? received_ : sent_)[opcode];
    ++tally.messages;
    tally.bytes += size;
}
```

`std::lock_guard` takes the lock when it is built and lets it go when it is destroyed, at the end
of the function, whichever way the function ends.

**Dropping a message is the most dangerous thing in this guide.** A dropped incoming message never
reaches the client's own code: that is how a mod handles an opcode the client does not know, and
it is also how a mod leaves the client with a different picture of the world than the server has,
with nothing to say so. Scout drops nothing until it is asked for an opcode by number, logs each
change as a warning, and never drops what the client sends. `Drop`, in the same file, does that
bookkeeping.

One rule comes with threads: a family whose events can come from another thread reads its list
of scripts without a lock, so a script of that family is added in `OnLoad` and left there. Scout
does exactly that. Add `src/Traffic.cpp` under `SOURCES`, the member `scout::Traffic traffic_;`
after `units_`, and `wxl::ScriptMgr::Add(&traffic_);` in `OnLoad`.

---

## 7. Talk to the interface

The game's interface, its windows, bars and chat, is written in Lua and run by the client. Addons
are Lua too. Through the `wxl.script` service a mod can add functions to that Lua, and events that
an addon's frames can register for.

### Using a service

Each service has a C++ class in the SDK, its *facade*: `wxl::ScriptService` for `wxl.script`,
from `wxl/services/script.hpp`. A facade has no state of its own, so you build one where you need
it. It tests false when the runtime does not publish the service, and then every call through it
answers `WXL_E_UNSUPPORTED`. Since Scout lists `wxl.script` as optional, it tests the facade first,
and without it keeps its journal to itself.

### The client's two Lua contexts

The client does not keep one Lua for the whole session. It runs one *context* for the login and
character screens (the *glue*), throws it away when you enter the world, and builds another for
the game; logging out, or reloading the interface, throws that one away in turn. **Nothing
registered in a context survives it.** A mod that registered its functions once, at load, would
lose them at the first change of screen.

Two events of the `LuaScript` family mark these moments: `ScriptVmReady` when a context is ready
to be registered on, with `WXL_SCRIPT_CONTEXT_GLUE` or `WXL_SCRIPT_CONTEXT_GAME`, and
`ScriptVmClosing` before it goes. So Scout registers from scratch in every `OnScriptVmReady`, in
both contexts.

### The script API

Create `src/ScriptApi.h`. A function registered on the client's Lua is called through the script
service with one argument, a `wxl::ScriptCall&`. Here each is a `std::function` member whose
lambda forwards to a member function, so it reaches Scout's state:

**`examples/scout/src/ScriptApi.h`**

```cpp
/// A function as the script service calls it: (*function)(call).
using Function = std::function<void(wxl::ScriptCall&)>;

Function scoutCount_   = [this](wxl::ScriptCall& call) { Count(call); };
Function scoutEntry_   = [this](wxl::ScriptCall& call) { EntryAt(call); };
Function scoutNearby_  = [this](wxl::ScriptCall& call) { Nearby(call); };
Function scoutTraffic_ = [this](wxl::ScriptCall& call) { TrafficOf(call); };
Function scoutDrop_    = [this](wxl::ScriptCall& call) { Drop(call); };
```

The lambdas capture `this`, so the class deletes its copy constructor: a copy would call into the
original. The rest of the header is the class around these members, `ScriptApi`, a
`wxl::LuaScript` that holds the clock, the journal and the traffic counter by reference.

`src/ScriptApi.cpp` registers them each time a context is ready:

**`examples/scout/src/ScriptApi.cpp`**

```cpp
void ScriptApi::OnScriptVmReady(uint32_t context, void*)
{
    const wxl::ScriptService script;
    if (!script) return; // wxl.script is optional: without it, Scout keeps its journal to itself

    struct Registration
    {
        const char* name;
        Function*   function;
    };
    const Registration functions[] = {
        {"ScoutCount", &scoutCount_},
        {"ScoutEntry", &scoutEntry_},
        {"ScoutNearby", &scoutNearby_},
        {"ScoutTraffic", &scoutTraffic_},
        {"ScoutDrop", &scoutDrop_},
    };

    for (const Registration& registration : functions)
    {
        const wxl_result result = script.RegisterFunction(registration.name, registration.function);
        if (result != WXL_OK)
            wxl::Log(WXL_LOG_WARN, kTag,
                     std::string("could not register ") + registration.name + ": result " +
                         std::to_string(result));
    }
    if (script.RegisterEvent(kUpdatedEvent) != WXL_OK)
        wxl::Log(WXL_LOG_WARN, kTag, std::string("could not declare ") + kUpdatedEvent);

    open_ = true;
    wxl::Log(WXL_LOG_INFO, kTag,
             context == WXL_SCRIPT_CONTEXT_GLUE ? "functions registered on the login screens"
                                                : "functions registered in the game");
}

void ScriptApi::OnScriptVmClosing(uint32_t, void*)
{
    open_ = false;
}
```

- `RegisterFunction(name, &function)` makes a global Lua function of that name. The object you
  pass must live as long as the context; a member of a script lives as long as the mod. A name the
  client already uses is refused rather than replaced, and the result code says so: Scout logs it.
- `RegisterEvent` declares an event name, so an addon's frame can listen for it before anything
  fires it. `kUpdatedEvent` is `"SCOUT_UPDATED"`, declared in the header.
- The second parameter, the Lua state's own pointer, is for a mod that talks to the Lua engine
  directly. Scout does not, so it goes unnamed.
- `open_` remembers whether a context holds what Scout registered.

### A function's arguments and results

A `wxl::ScriptCall` is one call from Lua. You read the arguments by position, from 0, and push the
results, as many as you like, since a Lua function can return several values:

**`examples/scout/src/ScriptApi.cpp`**

```cpp
// ScoutEntry([index]) -> what, who, level, whole seconds ago; index 1, the default, is the
// newest. Nothing past the oldest line.
void ScriptApi::EntryAt(wxl::ScriptCall& call) const
{
    double index = 1.0;
    if (call.ArgType(0) != WXL_VALUE_NIL)
    {
        const std::optional<double> given = call.Number(0);
        if (!given || *given < 1.0 || std::floor(*given) != *given)
        {
            call.Fail("ScoutEntry([index]): index is a whole number from 1, the newest line");
            return;
        }
        index = *given;
    }

    const Entry* entry = journal_.FromNewest(static_cast<std::size_t>(index) - 1);
    if (!entry) return;

    call.PushString(Describe(entry->what));
    call.PushString(entry->who);
    call.PushNumber(entry->level);
    call.PushNumber(std::floor(clock_.Now() - entry->at));
}
```

- `call.Number(0)` answers a `std::optional<double>`: empty when the argument is missing or is not
  a number. Lua numbers are all `double`.
- `call.ArgType(0)` tells which kind of value is there; `WXL_VALUE_NIL` when there is none, so the
  index can be left out.
- **`call.Fail(message)`** raises a Lua error, as the game's own functions do when they are called
  wrongly. It does not throw: your function returns normally, and the error is raised on the Lua
  side once it has.
- A function that pushes nothing returns nothing to Lua, which reads as `nil`.

`Count`, `Nearby`, `TrafficOf` and `Drop` follow the same pattern; `Nearby` counts the units the
client holds with `wxl::ObjectService().InView<wxl::Unit>()`, a snapshot of what is in view.

### Firing an event

**`examples/scout/src/ScriptApi.cpp`**

```cpp
void ScriptApi::Announce(const Entry& entry) const
{
    if (!open_) return;
    wxl::ScriptService().FireEvent(kUpdatedEvent, {wxl::Value::String(Describe(entry.what)),
                                                   wxl::Value::String(entry.who),
                                                   wxl::Value::Number(entry.level)});
}
```

`FireEvent` hands the event to every frame registered for it, with the values listed. An addon
receives it the way it receives the game's own events:

```lua
local frame = CreateFrame("Frame")
frame:RegisterEvent("SCOUT_UPDATED")
frame:SetScript("OnEvent", function(self, event, what, who, level)
    print(who, level, what)
end)
```

Add `src/ScriptApi.cpp` under `SOURCES`. The next step registers the script and connects
`Announce` to the journal.

---

## 8. Put it together

`src/Scout.cpp` in full, minus its `#include`s and the anonymous namespace around the class:

**`examples/scout/src/Scout.cpp`**

```cpp
class Scout final : public wxl::Mod
{
public:
    wxl_result OnLoad() override
    {
        journal_.OnAdd([this](const scout::Entry& entry) { script_.Announce(entry); });

        wxl::ScriptMgr::Add(&world_);
        wxl::ScriptMgr::Add(&units_);
        wxl::ScriptMgr::Add(&traffic_);
        wxl::ScriptMgr::Add(&script_);

        const std::string_view runtime = wxl::View(wxl::Host()->runtime()->name);
        wxl::Log(WXL_LOG_INFO, scout::kTag, "watching, on " + std::string(runtime));
        return WXL_OK;
    }

    void OnReady() override
    {
        if (!wxl::ScriptService::Available())
            wxl::Log(WXL_LOG_WARN, scout::kTag,
                     "this runtime has no wxl.script: the journal stays out of the interface");
    }

    void OnShutdown() override
    {
        wxl::Log(WXL_LOG_INFO, scout::kTag,
                 std::to_string(journal_.Written()) + " line(s) written this session");
    }

private:
    // Members are built in this order and destroyed in the reverse one: the state first,
    // then the scripts that hold references to it.
    scout::Clock   clock_;
    scout::Journal journal_;

    scout::WorldWatch world_{clock_, journal_};
    scout::UnitWatch  units_{clock_, journal_};
    scout::Traffic    traffic_;
    scout::ScriptApi  script_{clock_, journal_, traffic_};
};
```

- `journal_.OnAdd(...)` connects the journal to the interface: each new line is announced.
- `wxl::Host()->runtime()` describes the runtime that loaded the mod; its `name` is a `wxl_str`,
  the boundary's string, and `wxl::View` turns it into a `std::string_view`.
- `OnReady` says once, in the log, why the interface hears nothing when `wxl.script` is missing,
  so a quiet mod explains itself.

And `CMakeLists.txt` now lists every source:

**`examples/scout/CMakeLists.txt`**

```cmake
wxl_add_mod(scout
    SOURCES
        src/Scout.cpp
        src/Journal.cpp
        src/WorldWatch.cpp
        src/UnitWatch.cpp
        src/Traffic.cpp
        src/ScriptApi.cpp
    MANIFEST mod.json
    TARGETS win-x64 win-x86)
```

### How long things live

`WXL_MOD` creates your `Scout` object when the runtime loads the DLL, and destroys it at unload.
Before destroying it, it calls `OnUnload` and then drops every registration `ScriptMgr` holds, so
no event can reach a script that is being destroyed. That is why scripts can be plain members of
the mod: they live exactly as long as it does.

### What the runtime does when your code fails

Every call from the runtime into your mod, `OnLoad`, each stage, each event handler, each Lua
function, runs under a **crash guard**. If your code faults, for instance by reading through a null
pointer, the client does not crash. The runtime logs your mod's id, what it was doing and the
fault, and **disables** the mod: its handlers are dropped, its Lua functions are taken back, its
remaining stages and its unload are skipped. A mod that faults in `OnLoad` is refused instead. The
game goes on without it.

The guard does not undo anything: what your code changed before the fault stays changed, and
destructors of the frames it interrupted do not run. It keeps the game alive; it does not make the
fault harmless.

C++ exceptions are another matter. One that leaves an event handler or a Lua function is caught by
the runtime and logged as an error, and the rest of that handler does not run; the mod stays
enabled. One that leaves `OnLoad`, a stage or `OnUnload` is not caught and ends the game. Catch
what you throw, inside your own code.

---

## 9. Build, install, try it

Build as before:

```sh
cmake -S . -B build -DWXL_SDK_DIR=C:/path/to/sdk
cmake --build build --config Release
```

Close the game, replace `Mods/wxl.scout/` with the new `build/dist/wxl.scout/`, start the game and
log in. The log shows Scout at each stage of the session:

```
[INFO ] [scout] watching, on WarcraftXL core
[INFO ] [host] loaded wxl.scout 1.0.0 (Scout) from bin/x86/scout.dll
[INFO ] [scout] functions registered on the login screens
[INFO ] [scout] functions registered in the game
[INFO ] [scout] entered map 571 at 5800, 640
```

Each new Lua context gets Scout's functions again: the login screens first, then the game. Lines
from the runtime itself come between these.

In the game, type in the chat box. `/run` runs a line of Lua:

```
/run print(ScoutCount(), "lines in the journal")
/run print(ScoutEntry())
/run print(ScoutEntry(2))
/run print(ScoutNearby(), "units in view")
/run print(ScoutTraffic(0xA9))
/run print(ScoutTraffic(0x95, "sent"))
```

`ScoutEntry()` prints the newest line, such as `appeared Grizzled Bear 72 3`: what happened, to
whom, its level, and how many seconds ago. Target something and print it again. `0xA9` is
`SMSG_UPDATE_OBJECT`, the server's message about the objects in view, so its count climbs as they
come and change; `0x95`
is `CMSG_MESSAGECHAT`, which the client sends each time you say something in chat. Each call
prints the number of messages and their bytes.

Call one wrongly, `/run ScoutEntry(0)`, and the game shows Scout's error message, as it does for
its own functions.

Then the dangerous one:

```
/run print(ScoutDrop(0x105, true))
/run print(ScoutDrop(0x105, false))
```

`0x105` is `SMSG_TEXT_EMOTE`, which carries the text of an emote such as `/wave`. While it is
dropped, the client receives no such text, and the log has a warning saying so. The second line
puts it back.

---

## When something goes wrong

| What you see | What it means |
|---|---|
| nothing about your mod in the log | the folder is not where the runtime looks. It must be `Mods/<id>/mod.json` beside the executable, with the DLL at the path `binaries` gives |
| `refused wxl.scout: the binary reports id '...' but the manifest says '...'` | the id in `WXL_MOD` and the one in `mod.json` differ |
| `refused ...: requires service wxl.object ...` | this runtime does not publish a service in `requires`. Move it to `optional` if the mod can work without it |
| `refused ...` naming the abi | the manifest asks for a contract this runtime does not have: a later feature, or another major or minor. The message names both versions; rebuild against the runtime's SDK, or ask for less |
| the build stops on `"abi"` | `mod.json` asks for a version the SDK headers do not carry. Write the one the message gives |
| the build stops on `binaries` | the manifest's paths are not where the build writes the DLLs. The message gives the right path |
| `LNK1104: cannot open file '...dll'` | the game is running and holds the DLL. Close it, build again |
| a handler of yours never runs | check that its script is passed to `ScriptMgr::Add` in `OnLoad`, and that the log has no warning about the event (next row) |
| `subscribed to ..., which this runtime does not raise` | a handler for an event this runtime does not raise yet. The runtime's README lists what it raises |
| Scout's Lua functions are `nil` in the game | they were registered in one context only: register in every `OnScriptVmReady` |
| `mod wxl.scout faulted ...; it is disabled` | your code crashed; the line says in which handler. See [step 8](#what-the-runtime-does-when-your-code-fails) |

[Building a mod](building-a-mod.md#when-it-fails) lists the build's own messages in full.

---

## Where to go next

- **Change something.** Make the range smaller. Note what a player wears with
  `wxl::Player::VisibleItem(wxl::EquipSlot::Chest)` when a player appears. Fire `SCOUT_UPDATED`
  for deaths only. Write a small addon that shows the journal in a frame.
- **Read the rest of the families** in [`include/wxl/events.def`](../include/wxl/events.def), one
  documented line per event, and the parameters of each in
  [`include/wxl/fields.def`](../include/wxl/fields.def).
- **[Building a mod](building-a-mod.md)** is the reference for the build and the C++ layer: one
  architecture at a time, extra sources and include paths, one event outside a script with
  `wxl::Subscribe`, the other services.
- **[`template/`](../template)** is a small mod to copy when you start your next one.
- **The runtime's README** lists the events it raises and the services it publishes today.
