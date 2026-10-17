# WarcraftXL SDK

**The contract a WarcraftXL mod is written against.**

Header-only. There is nothing to link and nothing to ship: this repository declares what a mod
exposes, what a runtime offers it, and the rules of the boundary between them. The runtimes
implement it.

| Runtime | What it loads into |
|---|---|
| [`warcraftxl/core`](https://github.com/WarcraftXL/core) | the stock `Wow.exe` 3.3.5a, by injection and detour |
| [`warcraftxl/client`](https://github.com/WarcraftXL/client) | a recompiled client built from source, with declared extension points |

A mod is written once against the headers here and built per target. It runs wherever the services
it declares exist.

## A mod

```cpp
#include "wxl/sdk.hpp"

class Greeter final : public wxl::WorldScript
{
    void OnWorldEnter(const wxl_world_args& args) override
    {
        wxl::Log(WXL_LOG_INFO, "greeter", "hello, world");
    }
};

Greeter g_greeter;

class GreeterMod final : public wxl::Mod
{
    wxl_result OnLoad() override
    {
        wxl::ScriptMgr::Add(&g_greeter);
        return WXL_OK;
    }
};

WXL_MOD(GreeterMod, "wxl.greeter", "Greeter", 1)
```

[`template/`](template/) is the same thing as a buildable project, with a manifest.

## Layout

```
include/wxl/
  abi.h             the rules of the boundary: types, versioning, export
  events.def        the event table -- the single source of ids, names and args
  events.h          the event enum and the argument structs
  host.h            what a runtime offers a mod
  mod.h             what a mod exposes to a runtime
  sdk.hpp           the C++ layer: Mod, the script families, Service, WXL_MOD
  objects.hpp       world objects as C++ objects
  services/         one header per service
schema/mod.json     the manifest, as a JSON Schema
template/           a buildable mod to start from
tests/              the conformance checks every runtime has to pass
```

## How it holds together

**The boundary is plain C.** No C++ type crosses: no std types, no vtables, no exceptions. The C++
layer compiles into the mod and resolves to calls through a table of function pointers, so a mod
can be built with a different toolchain than the runtime that loads it. A script's virtual call
happens inside the mod, in a thunk the runtime reaches through an ordinary function pointer.

**Semantics cross, never layouts.** An event carries a map id and a position, not a pointer to a
client structure. An object is a generational handle plus accessors, so a handle kept past its
subject's death answers "invalid" rather than reading whatever took its place. This is what lets
one mod read a unit on a client that keeps it at a fixed address and on one that holds a real
object.

**A missing service is an answer, not a fault.** No runtime offers everything. A mod declares what
it requires in its manifest and is refused before it loads if that cannot be met; anything else it
asks for at runtime and does without.

**Ids are the ABI.** Event ids and stage numbers are appended, never renumbered, never removed.

## Versioning

`major.minor`, in `abi.h`.

- **Major** changes when an existing field, signature or id changes meaning. A runtime refuses a
  mod built against a different major.
- **Minor** changes when something is appended. A runtime accepts a minor at or below its own and
  refuses a higher one.
- **Major 0** means the contract is still moving: any release may break any other.

## Building the tests

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug
```

The checks run against a fake runtime in `tests/fake_host.hpp`. A real runtime passes the same
file by building it against its own host — which is how three implementations stay one contract
instead of becoming three dialects.

## License

MIT. The headers are compiled into whoever includes them, so they are deliberately permissive:
a mod built against this SDK carries no obligation from it.
