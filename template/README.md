# Example mod

The smallest mod that is still a real one: a lifecycle, a script in two families, and a service it
uses when the runtime offers it and does without when it does not.

```sh
cmake -S . -B build
cmake --build build --config Release
```

One source, one command, two binaries. The result is a mod folder, not a file:

```
build/dist/wxl.example/
  mod.json
  bin/x64/example.dll     for the recompiled client
  bin/x86/example.dll     for the stock Wow.exe
```

Copy that folder into the runtime's `Mods/` directory. `mod.json` is read before any of the mod's
code runs, so a runtime that cannot meet what the manifest requires refuses the mod with a reason
instead of failing on its first call; its `binaries` table names one file per architecture, and
the build refuses to configure when the two disagree.

`CMakeLists.txt` points at a checkout of the SDK one directory up. Copy the folder elsewhere and
pass `-DWXL_SDK_DIR=<path to WarcraftXL/sdk>`, or install the SDK and use
`find_package(wxl-sdk)`. Everything else is in [docs/building-a-mod.md](../docs/building-a-mod.md).
