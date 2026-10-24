// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// What the service rows cannot marshal by kind: the rows marked `shim`, which take a callback.
// A Lua mod hands over a Lua function; the shim keeps it, registers a native function that runs
// it, and does again on every new script context what the C service says a mod must redo there.
//
//   wxl.script.RegisterFunction(name, fn [, context])   a global of the client's script that
//                                                       calls fn, in every context or one
//   wxl.script.RegisterEvent(name [, context])          an event its frames can listen to
//
// Both are kept for the mod's life and replayed at each ScriptVmReady; they end when the mod's
// state closes (a reload, a disable, the runtime stopping). A global the client already holds
// keeps pointing at the shim, which answers with an error until the mod registers it again.

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

        /// Whether a registered function is running, inside the client's script: no mod's state
        /// may be closed then.
        bool Calling();
    } // namespace shims
} // namespace wxl::lua

#endif // WXL_LUA_SHIMS_HPP
