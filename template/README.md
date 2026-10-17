# Example mod

The smallest mod that is still a real one: a lifecycle, a script in two families, and a service it
uses when the runtime offers it and does without when it does not.

```sh
cmake -S . -B build -DWXL_SDK_DIR=<path to WarcraftXL/sdk>
cmake --build build --config Release
```

The result is `example.dll`. A mod is a folder, not a file:

```
Mods/example/
  mod.json
  bin/x64/example.dll
```

`mod.json` is read before any of the mod's code runs, so a runtime that cannot meet what the
manifest requires refuses the mod with a reason instead of failing on its first call.
