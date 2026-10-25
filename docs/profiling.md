# Profiling a mod with Tracy

A frame of the client lasts a few milliseconds. When it lasts longer, the game stutters, and the
first question is where those milliseconds went: the client's own work, the events the runtime
raised, or the handlers of one mod. The runtime carries a profiler that answers it, and this
guide shows you how to read it and how to add your own measurements to it.

The profiler is [Tracy](https://github.com/wolfpld/tracy). It has two halves: a small client
inside the game, which WarcraftXL.dll already contains, and a viewer, a program you run on any
computer that can reach the game over the network. The viewer connects to the game, and from
then on draws every frame on a timeline, with what ran inside it.

While no viewer is connected, the client records nothing. Every measurement point costs a single
check, a few nanoseconds, so the profiler stays in the builds players run, and your own
measurements can stay in the mod you ship.

## 1. Get the viewer

The viewer must be the same release as the client in the runtime: Tracy changes the way they
talk to each other between releases, and a viewer of another version refuses the connection,
saying the protocol is incompatible. This runtime carries **Tracy 0.14.1**.

1. Open [the release page of Tracy 0.14.1](https://github.com/wolfpld/tracy/releases/tag/v0.14.1).
2. Download `windows-0.14.1.zip` for Windows, `linux-0.14.1.zip` for Linux or
   `macos-0.14.1.zip` for a Mac. `tracy.pdf` on the same page is Tracy's own manual.
3. Unzip it anywhere and run the viewer inside, `tracy-profiler.exe` on Windows. Nothing needs
   installing.

## 2. Connect it to the game

Start the game as usual, x86 or x64. In `Logs/WarcraftXL.log` you should find:

```
[INFO ] [profile] profiler up (Tracy 0.14.1, on demand): a viewer connects on port 8086
```

The client listens on TCP port **8086**. In the viewer's window:

- **Same computer, or the same local network:** the game appears in the list under the address
  field within a second or two, named after its executable (`Wow.exe` or `WowClient64.exe`).
  Click it.
- **Anywhere else:** type the game computer's address in the field, `192.168.1.20` say, and press
  **Connect**. Add `:port` if you changed it (below).

The timeline starts filling at once. Press **Pause** in the viewer to stop scrolling and look at
a frame; disconnecting leaves the game as it was.

Some things to know:

- **Under Proton or Wine,** the game listens on the Linux machine's own network, so connect to
  that machine's address. When the game runs in one place and you watch it from another (a game
  streamed to your desktop, say), the viewer runs on your desktop and connects to the address of
  the machine that runs the game. The list under the address field only shows games on the same
  local network: type the address when it does not appear.
- **A firewall** on the game's machine must let TCP 8086 in.
- **Another port:** set the environment variable `TRACY_PORT` before the game starts, as
  `TRACY_PORT=8090 %command%` in Steam's launch options. The log line names the port in use.
- **No profiler at all:** `WXL_PROFILE=0` in the environment keeps the client from starting, so
  nothing listens. A runtime built with the CMake option `WXL_PROFILE` set to `OFF` carries no
  profiler; there, the calls of section 4 do nothing.

## 3. Read the timeline

Each frame of the client ends when it presents its image, and the viewer draws one bar per frame
at the top: its height is how long it took. Click a tall one to see what filled it. Below, each
thread has its own row of zones, nested as the calls were. The ones you will see on the main
thread:

| Zone | What it is |
|---|---|
| `OnWorldUpdate`, `OnUpdate`, `OnUnitDeath`... | the runtime raising an event: everything every mod did with it is inside |
| `scout: OnUpdate` | one mod's handler for that event, named after the mod's id |
| `wxl.lua: OnUpdate` | the Lua mod runtime's handler: the Lua mods' own zones are inside |
| `lua-hello: OnUpdate`, `lua-hello: onUpdate` | one Lua mod's handler, or its callback registered with `registerForEvent` |
| `World.Update`, `World.Render`, `WorldScene.Render` | the client's world, updated and drawn (the x64 client only) |
| `M2.AdvanceTime`, `M2.Animate`, `M2.Draw` | the client's models: animated, then drawn pass by pass (x64) |
| `ObjectMgr.UpdateObject`, `ObjectMgr.EnumVisibleObjects` | the server's object updates applied, and the walks over every visible object (x64) |
| `UI.Update`, `UI.Render`, `FrameScript.SignalEvent` | the interface's frames updated and drawn, and an event fired to its Lua (x64) |

On the stock client (x86), the client's own work has no zones: there, what is not inside a
runtime zone is the client's. On both, each mod's handlers have a colour of their own, the same
from one run to the next.

Two views answer the usual questions directly:

- **Statistics** (the sigma button) lists every zone by total time. Sort by it and look for your
  mod's id: that is what your handlers cost, all events together.
- **Find zone** shows how one zone's duration is spread. A handler that is usually fast but now
  and then takes ten times longer shows as a second hump: that is the stutter.

While a mod runs memory accounting (`wxl.footprint`, see
[Building a mod](building-a-mod.md#profiling-memory)), the viewer also draws three graphs under
the timeline: `storm bytes`, `storm blocks` and `accounting tables`.

## 4. Add your own zones

The zones the runtime opens tell you which handler is slow. To see which part of it is, open
zones inside it. A mod never links Tracy itself: it asks the runtime, through the service
`wxl.profile` (abi `0.1.12`), so that everything lands on the same timeline.

### From C++

Include `wxl/services/profile.hpp`. `WXL_PROFILE_ZONE` makes a zone from that line to the end of
the enclosing block:

```cpp
#include "wxl/services/profile.hpp"

void Journal::Flush()
{
    WXL_PROFILE_ZONE("Journal::Flush");          // until the end of Flush
    for (const Entry& entry : pending_)
    {
        WXL_PROFILE_ZONE_NAMED(zone, "Journal::Write");
        zone.Text(entry.name);                    // shown with the zone in the viewer
        Write(entry);
    }
    wxl::ProfileService::Plot("journal entries", static_cast<double>(pending_.size()));
}
```

The first time a zone's line runs, the runtime records its name, function, file and line once;
after that, opening the zone costs a check while no viewer is connected. Give each zone a name
that says what the code does, not which function it is in: the viewer shows the function and the
line already.

The rest of `wxl::ProfileService`:

| Call | What it does |
|---|---|
| `Connected()` | whether a viewer is connected: skip work done only to be measured |
| `Plot(name, value)` | a point on a graph of that name, drawn under the timeline: a count, a size |
| `Message(text)` | a line of text on the timeline, at this moment: "cache rebuilt" |
| `FrameMark(name)` | ends a frame of a frame set of your own, shown beside the client's frames: one per batch your mod processes, say |

`WXL_PROFILE_ZONE_COLOR(name, 0xRRGGBB)` gives a zone a colour. A zone must end on the thread
that opened it, which the block scope makes sure of.

### From Lua

Lua mods use the `wxl.profile` module. `Begin` opens a zone, `End` closes the innermost one:

```lua
function WorldScript:OnUpdate(delta)
    wxl.profile.Begin("count units")
    local units = 0
    for _ in pairs(tracked) do units = units + 1 end
    wxl.profile.Text(units .. " units")
    wxl.profile.End()

    wxl.profile.Plot("tracked units", units)
end
```

Your handler is already a zone of its own, `<your id>: OnUpdate`; these appear inside it. When a
handler returns, or stops on an error, whatever it left open is closed with it, so a forgotten
`End` cannot spill into the rest of the frame. `Plot`, `Message` and `FrameMark` are the C++
ones. A mod that uses the module asks for `"abi": "0.1.12"` in its `mod.json`.

## 5. What a zone costs

Measured on the runtime's own test harness, under Wine, with the profiler started and no viewer
connected (the harness's `--bench`):

| | x86 | x64 |
|---|---|---|
| a C++ mod's zone, `WXL_PROFILE_ZONE` | 3 ns | 2.5 ns |
| an event raised to two handlers, with the runtime's zones | 41 ns | 31 ns |
| the same in a runtime built without the profiler | 39 ns | 31 ns |

A Lua `Begin` and `End` add the cost of two calls from Lua to that. With a viewer connected, a
zone costs a few tens of nanoseconds and its data travels over the network: profile with the
zones you need, not one per line.
