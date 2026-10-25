# probe

A Lua test tool: it listens to every event the runtime raises and writes into
`Logs/WarcraftXL.log` what it saw, so that a test session can be read back from the log alone.
Copy it into a runtime's `Mods/` next to `wxl.lua` for a test, and take it out afterwards. It needs
a runtime of abi `0.1.13` or later, which can say which events it raises; its manifest asks for
`0.1.17`, so that it subscribes the gameplay events too.

```
probe/
  mod.json        "lua": "init.lua"; wxl.script and wxl.footprint optional
  init.lua        everything
```

What it writes, each line tagged `[wxl.probe]`:

- at load, how many events it listens to, and the events this runtime never raises;
- the first five occurrences of each event, with every argument in a compact form:
  `OnUnitDeath #1: Unit(0x000000000000002B) "Young Wolf"`; a string that is not UTF-8 text (a
  network message's payload, say) is shown as its bytes in hex, `<1 bytes 82>`, so the log stays a
  text file;
- every minute, how many times each event came in that minute, busiest first;
- `/mark <text>` in the chat box: `==== MARK 3: opened the bank ====`, what came since the previous
  mark, and the memory at that moment (from `wxl.footprint`); with a Tracy viewer connected, the
  mark is also a message on its timeline. `/run WxlProbeMark("text")` does the same;
- when the client closes, the totals, and the events that never came.

**An event the probe cannot see.** `OnObjectFieldChanged` reaches only the mods that watch a field
(`wxl.object.Watch("Unit", "Health")`), and only for the fields each one watches. The probe
watches none, so the event is always in its "never seen" line, with that reason next to it: its
absence says nothing about the runtime. A mod of yours that watches a field sees it.

**A test plan in the game.** Put a `steps.txt` next to `init.lua`, one step per line (blank lines
and lines starting with `#` are left out). On entering the world the chat box says how many steps
there are; then `/mark` with nothing after it moves to the next step and shows it in the chat box,
and the log's mark says which step began (`==== MARK 4: step 4/19: ... ====`). `/step` shows the
current step again. `/mark <text>` still writes a mark of your own without moving.

**What it costs.** The runtime skips the work of an event nobody listens to: some events are not
even read out of the client until a mod subscribes. Listening to everything turns all of that on,
for every event, for the whole session: the client runs slower, and a handler runs in Lua for each
texture, file and network message. `OnModelLoad` and `OnMapObjectLoad` hand the handler a copy of
the whole file, which takes memory on the original client. That is the price of seeing everything;
it is why the probe is a test tool. `OnModelBatchDrawn` is left out from the start: it comes once
for every batch of every model the client draws, thousands of times a frame, and a Lua call for each
would slow the whole session; the load line lists it under "not listened to" as skipped. Add an
event's name to `SKIP` at the top of `init.lua` to leave it alone too, or take this one out to see it.

`core/tools/read_logs.py` reads the log the probe writes into a summary of each launch: the marks,
the events seen and not seen, the warnings and the errors.
