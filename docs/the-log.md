# Reading the log

Everything the runtime does, and everything your mod prints, ends up in one text file. When a mod
does not load, when a handler never runs, when the game closes on its own, the answer is almost
always written there. This guide shows where the file is, what a launch looks like in it, how to
make it say more or less, and how to mark the moments of a test so you can find them again.

## 1. Where it is

Both runtimes write into a folder named `Logs` next to the client's executable:

```
Logs/
    WarcraftXL.log                        the launch running now, or the last one
    WarcraftXL-2026-10-25_14-03-22.log    an earlier launch, named after the time it started
    WarcraftXL-2026-10-25_09-41-07.log    ...
```

Each time the client starts, the previous `WarcraftXL.log` is renamed after the time that launch
started, and a new one begins. The 30 most recent earlier launches are kept; older ones are
deleted. So after a crash you can start the game again without losing the log that explains it.

The file is written as the game runs, and every line reaches the disk as soon as it is written:
open it in an editor while the game is running and you see what has happened so far. Lines that
report a problem are pushed all the way to the disk at once, so that a crash right after them
cannot lose them.

## 2. One launch, from top to bottom

A line starts with the time, the level and a tag that says who is speaking:

```
[14:03:22.101] [INFO ] [session] launch 2026-10-25 14:03:22 local (12:03:22 UTC), pid 4242
```

The level is one of `TRACE`, `DEBUG`, `INFO`, `WARN` and `ERROR`, from the most talkative to the
most serious. The tag is `session`, `core`, `host` or `crash` for the runtime itself, `wxl.lua` for
the Lua engine, and your mod's id for what your mod prints.

A launch reads in this order:

1. **The header**, tagged `session`: when the launch started, which runtime it is (`x86` for the
   original `Wow.exe`, `x64` for the recompiled client), the contract version it serves (`abi`),
   the commits it was built from, the executable with its size and, on the original client,
   whether its code is the unmodified 3.3.5a one, then Windows (and Wine or Proton, when the game
   runs on Linux), the processor, the memory and the address space, the command line, and the
   settings it read.
2. **The mods**: one line per folder of `Mods/`, loaded or not, and if not, why. Lua mods are
   reported a second time by `wxl.lua`, which is what runs them.
3. **The stages**: `stage early`, `stage storage`, `stage graphics`, `stage script`,
   `stage ready` as the client starts, `stage shutdown` as it closes.
4. **Whatever happens while you play**: your mod's lines, warnings, errors.
5. **The end**: `session end: clean exit, the client closed, after 1h 02m 03s`.

The last line matters most when something went wrong. A launch that ends with `session end` closed
as it should. A launch that ends without it did not: the process was killed, or it crashed in a way
nothing could report.

### When the game crashes

When the game crashes, the runtime writes what it can see before the client's own crash window
appears, under the tag `crash`:

```
[14:20:51.335] [ERROR] [crash] unhandled access violation (0xC0000005) reading 0x10 at Wow.exe+0x2134AB, thread 1234 (the main thread), 17m 29s after launch, stage ready
[14:20:51.336] [ERROR] [crash] #0 Wow.exe+0x2134AB
[14:20:51.336] [ERROR] [crash] #1 Wow.exe+0x1F00C2
[14:20:51.337] [ERROR] [crash] the last 16 events raised to mods, oldest first: OnUpdate (16 ms before), ...
```

An address is written as a module and an offset, `Wow.exe+0x2134AB`: the offset is the same at
every launch, so it can be looked up in a disassembler or a symbol file. The lines under it are
the chain of calls that led there, the most recent first, and the events that mods were handed
just before. The client's own report still goes into its `Errors/` folder as before.

A crash inside your mod's handler is different: the runtime catches it, writes one line saying
which mod, which handler and where, disables that mod, and the game goes on.

## 3. More detail, or less

The runtime writes `INFO` and above unless told otherwise. To change that, create a file named
`WarcraftXL.ini` next to the client's executable (beside `Wow.exe` or `WowClient64.exe`, not in
`Logs`) with these lines:

```ini
[log]
; trace, debug, info, warn, error or off
level = debug
```

Start the game again: the header's `settings:` line says the file was read and which level is in
force. `debug` adds what the runtime does at each step; `trace` adds every small detail and makes
a large file.

You can also give one tag a level of its own, to hear more from one mod without the rest, or less
from a mod that talks a lot:

```ini
[log]
level = info
; everything from this one mod
level.wxl.lua-hello = debug
; only the mod loader's problems
level.host = warn
```

Two more settings:

- `keep = 30` is how many earlier launches stay in `Logs/`; `keep = 0` keeps none.
- The environment variable `WXL_LOG_LEVEL` (`WXL_LOG_LEVEL=debug`), when it is set, wins over the
  file's `level`.

A line of the file the runtime does not understand is named in a warning near the top of the log,
with its line number, and otherwise ignored.

## 4. Marking the moments of a test

When you test something in the game, it helps to know which lines belong to which step. The
`wxl.probe` mod, in the SDK's [`examples/probe/`](../examples/probe/), is made for that. Copy it
into `Mods/` beside `wxl.lua` for the test. In the chat box, type:

```
/mark opened the bank
```

The log gets a line you cannot miss, with what happened since the previous mark and the state of
the memory at that moment:

```
[14:05:10.402] [INFO ] [wxl.probe] ==== MARK 1: opened the bank ====
[14:05:10.402] [INFO ] [wxl.probe] since the probe loaded (108.4 s): 31 events, 48210 raises: OnUpdate 6504, ...
```

The probe also writes the first few times each event happens, with what it carries, a line of
counts every minute, and, when the client closes, the events that never happened. That is a lot of
information, and it costs: listening to every event makes the client do work it normally skips,
so the game runs slower while the probe is installed. Take it out of `Mods/` when the test is over.

## 5. Reading many launches at once

The runtime's repository, `core`, has a script, `tools/read_logs.py`, that turns the logs of a
client folder into a summary of each launch: its header, its mods, how it ended, its warnings and errors grouped, the marks and
what happened between them, and the events seen and not seen. With Python 3:

```
python3 tools/read_logs.py path/to/the/client/folder -n 3
```

`-n` is how many of the most recent launches to show.
