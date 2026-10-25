# probe

A Lua test tool: it listens to every event the runtime raises and writes into
`Logs/WarcraftXL.log` what it saw, so that a test session can be read back from the log alone.
Copy it into a runtime's `Mods/` next to `wxl.lua` for a test, and take it out afterwards. It needs
a runtime of abi `0.1.13` or later, which can say which events it raises.

```
probe/
  mod.json        "lua": "init.lua"; wxl.script and wxl.footprint optional
  init.lua        everything
```

What it writes, each line tagged `[wxl.probe]`:

- at load, how many events it listens to, and the events this runtime never raises;
- the first five occurrences of each event, with every argument in a compact form:
  `OnUnitDeath #1: Unit(0x000000000000002B) "Young Wolf"`;
- every minute, how many times each event came in that minute, busiest first;
- `/mark <text>` in the chat box: `==== MARK 3: opened the bank ====`, what came since the previous
  mark, and the memory at that moment (from `wxl.footprint`); with a Tracy viewer connected, the
  mark is also a message on its timeline. `/run WxlProbeMark("text")` does the same;
- when the client closes, the totals, and the events that never came.

**What it costs.** The runtime skips the work of an event nobody listens to: some events are not
even read out of the client until a mod subscribes. Listening to everything turns all of that on,
for every event, for the whole session: the client runs slower, and a handler runs in Lua for each
texture, file and network message. `OnModelLoad` and `OnMapObjectLoad` hand the handler a copy of
the whole file, which takes memory on the original client. That is the price of seeing everything;
it is why the probe is a test tool. Add an event's name to `SKIP` at the top of `init.lua` to leave
it alone.

`core/tools/read_logs.py` reads the log the probe writes into a summary of each launch: the marks,
the events seen and not seen, the warnings and the errors.
