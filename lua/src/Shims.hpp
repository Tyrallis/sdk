// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// What the service rows cannot marshal by kind: the rows marked `shim`. Two take a callback: a
// Lua mod hands over a Lua function; the shim keeps it, registers a native function that runs
// it, and does again on every new script context what the C service says a mod must redo there.
//
//   wxl.script.RegisterFunction(name, fn [, context])   a global of the client's script that
//                                                       calls fn, in every context or one
//   wxl.script.RegisterEvent(name [, context])          an event its frames can listen to
//
// Both are kept for the mod's life and replayed at each ScriptVmReady; they end when the mod's
// state closes (a reload, a disable, the runtime stopping). A global the client already holds
// keeps pointing at the shim, which answers with an error until the mod registers it again.
//
// Two keep what a mod added, which the host can only see as wxl.lua's:
//
//   wxl.storage.AddSearchPath(directory, priority)      a folder searched for the client's
//   wxl.storage.RemoveSearchPath(directory)             files, until the mod removes it
//
// A mod's folders leave the search when it is disabled or its state closes; one that two mods
// added stays until both have let it go.
//
// Three keep what a mod states about memory, also seen by the host as wxl.lua's:
//
//   wxl.footprint.Snapshot()                            reports the heap of every Lua mod as
//                                                       "<id>/heap", then takes the picture
//   wxl.footprint.Report(name, bytes, count)            a consumer of the mod's own, reported
//                                                       as "<id>/<name>"
//   wxl.footprint.WatchPressure(bytes)                  the mod's threshold; the highest of all
//                                                       Lua mods' is wxl.lua's
//
// They are taken back when the mod is disabled or its state closes.

#ifndef WXL_LUA_SHIMS_HPP
#define WXL_LUA_SHIMS_HPP

#include <string_view>

#include "Lua.hpp"

namespace wxl::lua
{
    class ScriptMod;

    namespace shims
    {
        /// The function serving a shim row (`service`, `Name`), or null when none is written.
        /// It takes the ScriptMod as its first upvalue.
        lua_CFunction Find(std::string_view service, std::string_view name);

        /// Starts replaying registrations at each new script context. Called once, by the runtime.
        void Start();

        /// Stops: nothing is replayed any more.
        void Stop();

        /// Everything a mod registered ends with its state: called before the state closes.
        void Forget(ScriptMod& mod);

        /// What a mod holds goes: the folders it added leave the search path, its footprint
        /// reports and its pressure threshold are taken back. It is disabled, or its state closes.
        void ReleaseHeld(ScriptMod& mod);

        /// Whether a registered function is running, inside the client's script: no mod's state
        /// may be closed then.
        bool Calling();
    } // namespace shims
} // namespace wxl::lua

#endif // WXL_LUA_SHIMS_HPP
