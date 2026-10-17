<div align="center">

# WarcraftXL SDK

**The contract a WarcraftXL mod is written against**

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![ABI](https://img.shields.io/badge/ABI-0.1-blue.svg)](include/wxl/abi.h)
[![C++](https://img.shields.io/badge/C%2B%2B-17%20and%20up-blue.svg)](#-getting-started)
[![Header only](https://img.shields.io/badge/header--only-yes-lightgrey.svg)](include/wxl)

</div>

---

> [!IMPORTANT]
> ABI major 0 means the contract is still moving: any release may break any other. Ids and stage
> numbers are already append-only, but signatures are not frozen until 1.0.

## 🚀 Overview

Write a mod once, in C++, and run it on every WarcraftXL runtime: the stock `Wow.exe` 3.3.5a and
the recompiled client alike. This repository declares what a mod exposes, what a runtime offers it,
and the rules of the boundary between them. There is nothing to link and nothing to ship.

### Key features

- **One source, every target.** `wxl_add_mod()` builds a mod for each architecture in one command.
- **Object-oriented.** Derive from `wxl::Mod` and the script families; world objects are C++
  objects, not handles you dereference by hand.
- **A plain-C boundary.** No std type, vtable or exception crosses it, so a mod and the runtime
  that loads it may be built with different toolchains.
- **A missing service is an answer.** A mod declares what it requires and is refused before it
  loads if a runtime cannot meet it -- never halfway in.
- **Conformance tests included.** Every runtime passes the same file, which is how several
  implementations stay one contract instead of becoming dialects.

## 📋 Table of contents

- [Overview](#-overview)
- [Getting started](#-getting-started)
- [A mod](#-a-mod)
- [What a runtime offers](#-what-a-runtime-offers)
- [Layout](#-layout)
- [Versioning](#-versioning)
- [Documentation](#-documentation)
- [Support](#-support)
- [Contributing](#-contributing)
- [License](#-license)

## ⚡ Getting started

### Prerequisites

- CMake 3.21 or newer
- A C++17 compiler (MSVC 19.3+, GCC 11+, Clang 14+)
- The runtime you are targeting, to run what you build

### Build the template

```sh
cmake -S template -B build
cmake --build build --config Release
```

This produces a complete mod, ready to drop into a runtime's `Mods/` directory:

```
build/dist/wxl.example/
    mod.json
    bin/x86/example.dll
    bin/x64/example.dll
```

### Build the conformance tests

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug
```

## 🧩 A mod

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

[`template/`](template/) is the same thing as a buildable project, with its manifest.

## 🔌 What a runtime offers

Events are grouped into six script families: `WorldScript`, `RenderScript`, `ObjectScript`,
`AssetScript`, `NetScript` and `LuaScript`. Override the hooks you want; the rest cost nothing.

Beyond events, a runtime publishes services, and a mod asks for the ones it needs:

| Service | What it reaches | Portable? |
|---|---|---|
| `wxl.object` | units, players, game objects | yes |
| `wxl.script` | the client's Lua | yes |
| `wxl.storage` | archives and files | yes |
| `wxl.net` | messages to and from the server | yes |
| `wxl.ui` | frames and the interface | yes |
| `wxl.hook` | hook points a runtime names and keeps working | yes |
| `wxl.hook.raw` | a detour at a raw address | no: runtimes that load into a shipped binary |
| `wxl.patch` | bytes written into the image | no: runtimes that load into a shipped binary |

**Portable means a runtime may serve it, not that yours does.** Ask with `wxl::Service<T>()` and
be ready for nothing; require it in your manifest and be refused early instead. Which services a
given runtime publishes today is in that runtime's own README.

A mod that requires a service a runtime does not have is refused, with the reason, before it loads.

## 📂 Layout

```
include/wxl/
    abi.h          the rules of the boundary: types, versioning, export
    events.def     the event table -- the single source of ids, names and args
    events.h       the event enum and the argument structs
    host.h         what a runtime offers a mod
    mod.h          what a mod exposes to a runtime
    sdk.hpp        the C++ layer: Mod, the script families, Service, WXL_MOD
    objects.hpp    world objects as C++ objects
    services/      one header per service
host/              the mod host: discovery, manifests, load order, registry, event bus
runtime/           service scaffolding every runtime shares: handles, enumeration, gating
cmake/             wxl_add_mod(), which builds a mod for every target at once
schema/mod.json    the manifest, as a JSON Schema
template/          a buildable mod to start from
tests/             the conformance checks every runtime has to pass
```

A mod links `wxl::sdk` and nothing else. A runtime links `wxl::host` and `wxl::runtime` too, and
supplies only the bindings that reach its own client.

## 🔢 Versioning

`major.minor`, in [`abi.h`](include/wxl/abi.h).

- **Major** changes when an existing field, signature or id changes meaning. A runtime refuses a
  mod built against a different major.
- **Minor** changes when something is appended. A runtime accepts a minor at or below its own and
  refuses a higher one.

Event ids and stage numbers are appended, never renumbered, never removed.

## 📚 Documentation

- [Building a mod](docs/building-a-mod.md) -- the manifest, the targets, the services
- [`schema/mod.json`](schema/mod.json) -- the manifest schema, for your editor
- [`template/`](template/) -- a working mod to copy

## 💬 Support

- Something does not behave as these headers say: open an issue, and say which runtime.
- A runtime refuses your mod: the reason is in its log, and it names the service and the version.

## 🤝 Contributing

1. Fork and branch.
2. Keep the boundary plain C: no std type, vtable or exception crosses it.
3. Append ids, never renumber them.
4. Add the conformance check with the feature -- a contract without a test is a suggestion.
5. Open a pull request.

## 📄 License

MIT. See [LICENSE](LICENSE).

These headers compile into whoever includes them, so they are deliberately permissive: a mod built
against this SDK carries no obligation from it.

<div align="center">

[⬆ Back to top](#warcraftxl-sdk)

</div>
