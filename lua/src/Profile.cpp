// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Profile.hpp"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ScriptMod.hpp"
#include "wxl/host/profile.hpp"
#include "wxl/sdk.hpp"
#include "wxl/services/profile.hpp"

namespace wxl::lua::profile
{
    namespace
    {
        /// A zone a Lua mod opened with Begin, open until End or the end of the call.
        struct Open
        {
            const ScriptMod* mod;
            uintptr_t        zone; ///< 0 when nothing is recorded: kept so End pairs with Begin
        };

        std::vector<Open> g_open;
        size_t            g_base = 0; ///< where the call running now starts in g_open

        /// The locations Begin made, by "<mod id>\n<name>": the runtime's lookup takes a lock.
        std::unordered_map<std::string, uintptr_t> g_named;

        ScriptMod& Self(lua_State* L) { return *static_cast<ScriptMod*>(lua_touserdata(L, lua_upvalueindex(1))); }

        void CloseDownTo(size_t depth)
        {
            const wxl_profile_api* api = wxl::profile::Api();
            while (g_open.size() > depth)
            {
                if (g_open.back().zone && api) api->zone_end(g_open.back().zone);
                g_open.pop_back();
            }
        }
    } // namespace

    uintptr_t Location(const ScriptMod& mod, std::string_view what)
    {
        const wxl_profile_api* api = wxl::profile::Api();
        if (!api || !wxl::detail::Carries(api, &wxl_profile_api::location)) return 0;
        const std::string name = mod.Id() + ": " + std::string(what);
        return api->location(Str(name), Str(mod.Id()), Str(mod.Id()), 0, wxl::host::ModZoneColor(mod.Id()));
    }

    Call::Call(uintptr_t location) : base_(g_base)
    {
        g_base = g_open.size();
        if (location)
            if (const wxl_profile_api* api = wxl::profile::Api()) zone_ = api->zone_begin(location);
    }

    Call::~Call()
    {
        CloseDownTo(g_base);
        g_base = base_;
        if (zone_)
            if (const wxl_profile_api* api = wxl::profile::Api()) api->zone_end(zone_);
    }

    int L_Begin(lua_State* L)
    {
        const ScriptMod& mod = Self(L);
        if (lua_isnoneornil(L, 1))
            return luaL_error(L, "wxl.profile.Begin: 'name' (argument 1) is missing; it takes a string");
        size_t      size = 0;
        const char* text = luaL_checklstring(L, 1, &size);

        uintptr_t              zone = 0;
        const wxl_profile_api* api  = wxl::profile::Api();
        if (api && size && api->connected())
        {
            std::string key = mod.Id() + "\n" + std::string(text, size);
            auto        found = g_named.find(key);
            if (found == g_named.end())
            {
                // Named after the line that first opened it, so the viewer can show where it is.
                lua_Debug where{};
                std::string file = mod.Id();
                uint32_t    line = 0;
                if (lua_getstack(L, 1, &where) && lua_getinfo(L, "Sl", &where))
                {
                    file = where.short_src;
                    line = where.currentline > 0 ? static_cast<uint32_t>(where.currentline) : 0;
                }
                const uintptr_t location = api->location(wxl_str{text, static_cast<uint32_t>(size)}, Str(mod.Id()),
                                                         Str(file), line, 0);
                found = g_named.emplace(std::move(key), location).first;
            }
            zone = api->zone_begin(found->second);
        }
        g_open.push_back(Open{&mod, zone});
        return 0;
    }

    int L_End(lua_State* L)
    {
        const ScriptMod& mod = Self(L);
        if (g_open.size() <= g_base || g_open.back().mod != &mod) return 0;
        CloseDownTo(g_open.size() - 1);
        return 0;
    }

    int L_Text(lua_State* L)
    {
        const ScriptMod& mod  = Self(L);
        size_t           size = 0;
        const char*      text = luaL_checklstring(L, 1, &size);
        if (g_open.size() <= g_base || g_open.back().mod != &mod || !g_open.back().zone) return 0;
        if (const wxl_profile_api* api = wxl::profile::Api())
            if (api->zone_text) api->zone_text(g_open.back().zone, wxl_str{text, static_cast<uint32_t>(size)});
        return 0;
    }
} // namespace wxl::lua::profile
