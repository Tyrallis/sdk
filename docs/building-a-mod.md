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

## The C++ layer

Everything under `include/wxl/*.hpp` compiles into the mod and resolves to the C tables in
`host.h` and `services/*.h`. Nothing of it crosses the boundary.

### The mod

Derive from `wxl::Mod`, declare it once with `WXL_MOD`. Each boot stage has a named virtual:
`OnEarly`, `OnStorageReady`, `OnGraphicsReady`, `OnScriptReady`, `OnReady`, `OnShutdown`.
`OnStage(wxl_stage)` still runs after each of them, for every stage, including one appended to
the contract after this SDK was built. `OnLoad` runs before any stage and may refuse to load;
`OnUnload` runs last, and `wxl::ScriptMgr` is cleared right after it so no handler outlives the
code it points into.

### Events

Derive from a script family (`WorldScript`, `RenderScript`, `ObjectScript`, `AssetScript`,
`NetScript`, `LuaScript`, `CombatScript`), override the events you want,
`wxl::ScriptMgr::Add(&script)`. `Add` subscribes only the events the script overrides, decided at
compile time, so an event you leave alone costs nothing at all. That needs the script's own type:
pass `&script`, not a pointer to its family, which hides what it overrides and makes `Add`
subscribe the whole family to be safe.

A handler takes the event's args as parameters, one per member of its struct in `fields.def`
order:

```cpp
void OnMessageReceived(uint32_t opcode, wxl::Bytes data, wxl::Answer suppressed) override
{
    if (opcode == 0x1234 && !data.empty()) suppressed.Set(); // the client never sees it
}
```

| Kind in `fields.def` | Parameter |
|---|---|
| `u32`, `i32`, `u64`, `i64`, `f32` | `uint32_t`, `int32_t`, `uint64_t`, `int64_t`, `float` |
| `bool` | `bool` |
| `handle`, `guid`, `vec3` | `wxl_handle`, `wxl_guid`, `wxl_vec3`; `wxl::Unit(handle)` reads the object |
| `str` | `std::string_view`, valid for the call |
| `bytes` | `wxl::Bytes`: `data()`, `size()`, a range of `uint8_t`, `View()` as characters; valid for the call |
| `answer` | `wxl::Answer`: `Value()` is what earlier handlers decided, `Set()` answers |
| `buffer` | `wxl::Buffer`: `Value()` is what it holds, `Write(text)` answers when the text and its terminator fit in `Capacity()` |
| `ptr` | the member's C type |

A member that only gives the length of a bytes member or the capacity of a buffer is not a
parameter: the view carries it. `wxl::HandlerOf<WXL_EV_...>` is the whole list as a function type.
A member function named like a handler but taking other parameters would hide the family's virtual
instead of overriding it; `Add` refuses it at compile time.

For one event outside a family, `wxl::Subscribe<WXL_EV_TargetChanged>(&handler)` calls the
handler with the same parameters and returns a `wxl::Subscription` that unsubscribes in its
destructor. `wxl::EventArgsOf<WXL_EV_...>` names the args struct of any event: what a C handler
receives, and what `wxl::Emit` raises.

### Reads that can fail

Every read through a service answers `wxl::Expected<T>` (`include/wxl/expected.hpp`): the value,
or the `wxl_result` that stood in for it.

```cpp
wxl::Expected<uint32_t> hp = unit.Health();
if (hp)                        // a value; 0 means the unit is dead
else if (hp.IsNotFound())      // the handle is stale: the unit is gone
else if (hp.IsUnsupported())   // this runtime has no object service
```

`*hp`, `hp->`, `hp.Value()` need `hp` to hold a value; `hp.ValueOr(0)` never does.
`hp.ToOptional()` drops the reason for code that does not care.

### Objects

`wxl::Object` and its kinds -- `Item`, `Container`, `Unit`, `Player`, `GameObject`, `DynamicObject`, `Corpse`, one per `wxl_object_type`, one header each under `wxl/objects/` -- are values over a handle: copy them,
compare them, order them, hash them, keep them across frames. `if (unit)` asks whether the unit
is still there (one call); `unit.HasHandle()` asks whether there is a handle at all (free).
`object.As<wxl::Unit>()` is a checked downcast returning `std::optional`.

Finders answer with an object that may be null -- `wxl::Player::Local()`,
`wxl::ObjectService().FromGuid(guid)` -- and reads answer with an `Expected`.

```cpp
wxl::ObjectService objects;
for (wxl::Unit unit : objects.InView<wxl::Unit>())        // a snapshot of what is in view
    ...
auto view  = objects.InView<wxl::Unit>();
auto alive = std::count_if(view.begin(), view.end(),
                           [](const wxl::Unit& u) { return u.Health().ValueOr(0) > 0; });
```

`ObjectView<T>` holds handles and builds a `T` on access, so a unit that left between the
snapshot and the visit reads as gone. `objects.Positions(view)` reads every position in one call.

### Services

Each service has a typed facade with its name and version sealed in: `wxl::ObjectService`,
`wxl::ScriptService`, `wxl::StorageService`, `wxl::NetService`, `wxl::UiService`,
`wxl::HookService`, `wxl::RawHookService`, `wxl::PatchService` (`include/wxl/services/*.hpp`,
or all of them through `include/wxl/services.hpp`). A facade is stateless -- construct one
wherever it is needed -- and resolves its table on first use. It tests false while the runtime
does not publish the service, and every call through it then answers `WXL_E_UNSUPPORTED` or the
empty object.

```cpp
wxl::ScriptService script;
script.RegisterFunction<&Greet>("Greet");       // void Greet(wxl::ScriptCall&)
script.RegisterFunction("Add", &adder);          // (*adder)(wxl::ScriptCall&)
script.FireEvent("MY_EVENT", {wxl::Value::Number(1), wxl::Value::String("x")});

wxl::StorageService storage;
if (wxl::Expected<std::string> text = storage.ReadText("Interface\\x.lua")) ...
```

`wxl::Service<T>(name, version)` still gives the raw table, for a service these do not cover.

### Resources

Whatever the C boundary hands back as a token comes back as a move-only object that releases in
its destructor: `wxl::Subscription`, `wxl::Hook`, `wxl::Patch`, `wxl::File`, `wxl::OpcodeClaim`,
`wxl::Panel`. Each has `Reset()` to release early (safe to repeat) and `Release()` to disown the
token for code that manages it by hand. Keep them as members of the mod or of a script, and they
go when it goes -- before the DLL does.

`wxl::PatchSet` holds several patches and restores them last-applied-first; `Apply` answers
false when a write is refused, so a mod can stop and let what went before undo itself.

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

**`"abi" is "x.y.z" but the SDK headers are a.b.c`** -- the manifest asks for a contract version
the headers it was built against do not carry: another major or minor, or a later feature. Set it
to what the message says.

**`"abi" is "x.y", the two-part numbering the SDK used before 0.1.5`** -- the manifest predates
the three-part version. Set it to what the message says.

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
