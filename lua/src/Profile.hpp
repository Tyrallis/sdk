// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The Lua mods on the profiler's timeline, through wxl.profile. The host names every handler
// zone after the mod that subscribed it, and to the host every Lua mod is wxl.lua; so each call
// into a Lua mod -- an event handler, a callback, a script function, its entry file -- is a zone
// of its own inside wxl.lua's, named after that mod: "lua-hello: OnWorldEnter".
//
// What a Lua mod adds itself:
//
//   wxl.profile.Begin(name)    opens a zone inside the call running now
//   wxl.profile.End()          closes the innermost one this call opened
//   wxl.profile.Text(text)     adds a line to it
//
// A call closes what its code left open when it returns or errors, so a missing End never
// leaves a zone running past the handler. Everything runs on the main thread.

#ifndef WXL_LUA_PROFILE_HPP
#define WXL_LUA_PROFILE_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Lua.hpp"

namespace wxl::lua
{
    class ScriptMod;

    namespace profile
    {
        /// A location for zones of a mod: "<mod id>: <what>". 0 without the service.
        uintptr_t Location(const ScriptMod& mod, std::string_view what);

        /// A zone around one call into a mod, closing what the call left open.
        class Call
        {
        public:
            /// @p location from Location(); 0 makes the zone a no-op but still closes the mod's.
            explicit Call(uintptr_t location);
            ~Call();

            Call(const Call&)            = delete;
            Call& operator=(const Call&) = delete;

        private:
            size_t    base_;
            uintptr_t zone_ = 0;
        };

        /// The shims of the rows `zone_begin`, `zone_end` and `zone_text`.
        int L_Begin(lua_State* L);
        int L_End(lua_State* L);
        int L_Text(lua_State* L);
    } // namespace profile
} // namespace wxl::lua

#endif // WXL_LUA_PROFILE_HPP
