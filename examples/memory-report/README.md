# memory-report

A Lua mod that measures where the client's memory goes and writes it down. Copy the folder into
a runtime's `Mods/` next to `wxl.lua`; it needs a runtime of abi `0.1.11` or later, which
publishes `wxl.footprint`.

```
memory-report/
  mod.json    "lua": "init.lua", requires wxl.footprint
  init.lua    the counting, the report, when it is written
```

## Running it

1. Copy `memory-report/` into the client's `Mods/` folder and start the client. From the moment
   the mod loads, the client's allocations are counted by the source file that made them.
2. Play as you would. Every five minutes the mod writes a report; it writes one more when the
   largest free block of the address space falls under 256 MB, and a last one when the client
   closes.
3. For a report now, type `/memreport` in the chat box once in the world, or
   `/run MemoryReport()` anywhere.

`INTERVAL` and `THRESHOLD` at the top of `init.lua` change how often it writes and when it warns.
Each report also leaves a line in `Logs/WarcraftXL.log`, tagged `[wxl.memory-report]`.

## Reading the report

The mod writes three files into its own folder:

| File | What it holds |
|---|---|
| `memory-report.txt` | the last report, to read |
| `memory-sources.csv` | every source file of the last report, for a spreadsheet |
| `memory-history.csv` | one line per report since the file was made: the totals over a session |

`memory-report.txt` has five parts.

**Address space.** What the process has committed, by kind (`private`: heaps and the rest;
`image`: the program and its DLLs; `mapped`: files mapped into memory), what it has reserved, and
what is free. On the original client, a 32-bit program, *the largest free block* is the number
that matters: an allocation larger than it fails, however much is free in all. When it shrinks
report after report, something is fragmenting the address space or holding on to memory.

**Consumers.** The big holders of memory, largest first:

| Name | What it is |
|---|---|
| `storm` | everything the client allocated through Storm since the mod loaded, still live |
| `textures.managed` | textures kept in video memory and, as a copy, in the process's own memory: on the original client they cost address space twice over |
| `textures.default` | render targets: video memory only |
| `buffers.*` | the vertex and index buffers, by the same pools |
| `sound` | the sound engine's heap |
| `script` | the interface's Lua: the client's own scripts and its addons |
| `archives`, `tables` | the tables of the mounted archives; the client databases |
| `accounting` | what the counting itself holds |
| `<mod id>/heap` | the heap of a Lua mod; `<mod id>/<name>` anything a mod reported of its own |

They overlap: `script`, `tables` and `archives` are allocated through Storm, so they are inside
`storm` as well. `count` is how many blocks, textures or entries; `peak` the most seen since the
mod loaded.

**Storm by source file.** Each file the client named when it allocated, with what it holds now,
in how many blocks, its peak, and how many allocations it made in all. Names are the client's own:
`.\Client.cpp`, a build path of the engine, or `new` for everything the client's C++ allocated
with `new`, which is a large share and names no file. The client's growable arrays name the type
of what they hold instead of a file, and the report spells it as C++ does: `unsigned char` for a
byte buffer, `const TaxiPathRec*` for an array of pointers, `CSimpleTexture` for objects of
that class. A few other names are labels the client chose, such as `CM2Model` or `FMod`.

**Archives.** Each archive the client mounted, its priority, the entries of its hash and block
tables, and what its tables hold in memory. A folder named like an archive (`Data\Patch-4.MPQ\`,
a directory the client mounts as one) ends in a backslash and has no tables: its files are read
from the disk.

**Client databases.** The largest loaded tables: records, bytes per record, and what records,
strings and index hold together.

[docs/lua-mods.md](../../docs/lua-mods.md), "Where the memory goes", explains the module it uses.
