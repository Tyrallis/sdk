# Building a mod

One source, one command, two binaries. A mod is written once against the SDK headers and built
for both runtimes in a single step: a 64-bit DLL for the recompiled client and a 32-bit DLL for
the stock `Wow.exe`. Architecture lives in the folder, never in the file name.

## What you need

- **CMake 3.21 or newer.**
- **Visual Studio 2022 or newer** with the *Desktop development with C++* workload. The component
  *MSVC v14x - C++ x64/x86 build tools* carries both compilers; the Build Tools edition is enough.
- **The SDK**: a checkout of `WarcraftXL/sdk`, or an install of it (`cmake --install`), reached
  with `find_package(wxl-sdk)`.

Nothing else. The SDK is header-only and fetches nothing.

## The mod's `CMakeLists.txt`

Start from [`template/`](../template/). The whole file is:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my-mod VERSION 1.0.0 LANGUAGES NONE)

set(WXL_SDK_DIR "path/to/sdk" CACHE PATH "Path to the WarcraftXL SDK")
include("${WXL_SDK_DIR}/cmake/wxl_add_mod.cmake")

wxl_add_mod(my-mod
    SOURCES src/mod.cpp)
```

`wxl_add_mod` takes:

| Argument | Meaning | Default |
|---|---|---|
| `SOURCES` | the mod's translation units | required |
| `MANIFEST` | the manifest to validate and ship | `mod.json` beside the `CMakeLists.txt` |
| `TARGETS` | `win-x64`, `win-x86`, or both | both |
| `OUTPUT_NAME` | the DLL's base name | the target name |
| `OUTPUT_DIR` | where the mod folder is assembled | `build/dist` |

`LANGUAGES NONE` is deliberate: the top level drives the per-architecture builds and never needs a
compiler; each child enables C++ itself.

## The one command

From the mod's directory:

```sh
cmake -S . -B build
cmake --build build --config Release
```

Without `--config`, Visual Studio builds Debug.

## What comes out

```
build/dist/<id>/            <id> is the manifest's "id"
  mod.json
  bin/x64/<name>.dll        win-x64: the recompiled client
  bin/x86/<name>.dll        win-x86: the stock Wow.exe
```

That folder is the mod. The build also leaves one tree per architecture, `build/win-x64/` and
`build/win-x86/`, where the PDBs and intermediate files live.

The manifest's `binaries` table is checked at configure time against what the build produces:

```json
"binaries": {
  "win-x64": "bin/x64/<name>.dll",
  "win-x86": "bin/x86/<name>.dll"
}
```

Every architecture built must be listed at that path, and every architecture listed must be
built. A manifest that promises a binary the folder does not hold is a packaging bug; the build
says so instead of letting a runtime discover it.

## Installing into a client

Copy `build/dist/<id>/` into the runtime's mod directory, `Mods/` beside the client executable,
keeping the folder name. Each runtime reads `mod.json`, picks the entry for its own architecture
and ignores the other. A mod with no entry for a runtime is skipped, not failed.

A loaded DLL is locked by the game: close the client before rebuilding into a folder it loaded
from.

## Building one architecture

Narrow the targets from the command line, and make the manifest agree:

```sh
cmake -S . -B build -DWXL_MOD_TARGETS=win-x64
```

The configure output names what it skipped:

```
-- my-mod: win-x64  -> .../build/dist/my.mod/bin/x64/my-mod.dll
-- my-mod: win-x86  skipped (not in the targets: win-x64)
```

The manifest still has to match the folder, so `binaries` must list only `win-x64` for this to
configure. That is strictness about the package, not about iteration: for a quick loop on one
architecture, configure a child tree by hand and skip the packaging entirely:

```sh
cmake -S . -B build-x86 -A Win32 -DWXL_MOD_ARCH=win-x86
cmake --build build-x86 --config Release        # DLL in build-x86/Release/
```

`-A x64` with `win-x64` for the other one. The tree checks that the platform and the architecture
agree.

## Adding to the target

At the top level of the superbuild there is no library target, only the per-architecture drivers.
Anything that touches the target belongs inside `if(TARGET ...)`, which is true in each child:

```cmake
wxl_add_mod(my-mod SOURCES src/mod.cpp src/other.cpp)

if(TARGET my-mod)
    target_include_directories(my-mod PRIVATE third_party/include)
    target_compile_definitions(my-mod PRIVATE MY_MOD_VERBOSE=1)
endif()
```

`WXL_MOD_ARCH` holds `win-x64` or `win-x86` in a child, and is empty at the top level, for
anything that differs per architecture.

The mod is compiled as C++17 with `/W4 /permissive-` (MSVC) or `-Wall -Wextra`.

## When it fails

**`the 32-bit (x86) MSVC toolchain is not installed`** -- Visual Studio has no x86 compiler. Visual
Studio Installer > Modify > Individual components > *MSVC v14x - C++ x64/x86 build tools*. Or
build `win-x64` alone, as above.

**`mod.json promises "win-x86" ... but win-x86 is not being built`** -- the targets were narrowed
and the manifest was not. Remove the entry, or build both.

**`mod.json promises no binary for win-x86, which this build produces`** -- the opposite: add the
entry, at the path the message gives.

**`mod.json promises win-x64 at "..." but the build writes it to "bin/x64/<name>.dll"`** -- the
path in the manifest is not where the DLL lands. `OUTPUT_NAME` sets the file name; the folder is
always `bin/<arch>/`.

**`"abi" is "x.y" but the SDK headers are a.b`** -- the manifest claims a contract version the
headers it was built against do not carry. Set it to what the message says.

**`building win-x64;win-x86 from one tree needs a Visual Studio generator`** -- with Ninja or
Makefiles, CMake cannot pick the architecture per child project. Use `-G "Visual Studio 17 2022"`
(any installed version), or build one architecture from a developer prompt of that architecture
with `-DWXL_MOD_TARGETS=`.

**`this tree is WXL_MOD_ARCH=win-x64 (64-bit) but the compiler targets 32-bit`** -- a hand-made
child tree with the wrong `-A`. Match them.

**`cannot find the SDK next to ...`** -- `WXL_SDK_DIR` does not point at a checkout, or the
installed package is incomplete. Pass `-DWXL_SDK_DIR=<path to WarcraftXL/sdk>`.

**`No CMAKE_CXX_COMPILER could be found`** or **`MSB8020: The build tools for v14x cannot be
found`** -- the C++ workload is not installed in the Visual Studio CMake picked. Install it, or
delete `build/` and configure again after installing.

**`LNK1104: cannot open file '....dll'`** -- the DLL is loaded by a running client. Close it.

**The mod folder is stale after renaming the mod or its id** -- the old folder is not removed.
Delete `build/dist/` and build again.
