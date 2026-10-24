// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "ScriptMod.hpp"

#include <chrono>
#include <cstring>
#include <system_error>
#include <utility>

#include "Args.hpp"
#include "Objects.hpp"
#include "Runtime.hpp"
#include "Sandbox.hpp"
#include "wxl/host/admission.hpp"
#include "wxl/host/manifest.hpp"
#include "wxl/reflect.hpp"

namespace wxl::lua
{
    namespace
    {
        /// The script families, as the C++ SDK names them, in table order.
        std::vector<std::string> Families()
        {
            std::vector<std::string> families;
            uint32_t                 count  = 0;
            const reflect::Event*    events = reflect::Events(&count);
            for (uint32_t i = 0; i < count; ++i)
            {
                const std::string family = std::string(events[i].family) + "Script";
                bool              seen   = false;
                for (const std::string& known : families)
                    if (known == family) seen = true;
                if (!seen) families.push_back(family);
            }
            return families;
        }

        /// "OnUpdate, OnWorldEnter, ...": the methods of one family.
        std::string MethodsOf(const char* family)
        {
            std::string           text;
            uint32_t              count  = 0;
            const reflect::Event* events = reflect::Events(&count);
            for (uint32_t i = 0; i < count; ++i)
            {
                if (std::strcmp(events[i].family, family) != 0) continue;
                if (!text.empty()) text += ", ";
                text += "On";
                text += events[i].name;
            }
            return text;
        }

        std::string Join(const std::vector<std::string>& items)
        {
            std::string text;
            for (const std::string& item : items)
            {
                if (!text.empty()) text += ", ";
                text += item;
            }
            return text;
        }

        /// The arguments of a Lua call, each through tostring, joined by @p separator.
        std::string Concatenate(lua_State* L, int first, const char* separator)
        {
            const int   top = lua_gettop(L);
            std::string text;
            lua_getglobal(L, "tostring");
            for (int i = first; i <= top; ++i)
            {
                lua_pushvalue(L, top + 1);
                lua_pushvalue(L, i);
                lua_call(L, 1, 1);
                size_t      size  = 0;
                const char* piece = lua_tolstring(L, -1, &size);
                if (i > first) text += separator;
                if (piece) text.append(piece, size);
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
            return text;
        }
    } // namespace

    ScriptMod::ScriptMod(Runtime& runtime, host::ModCandidate candidate, uint32_t rank)
        : runtime_(runtime), candidate_(std::move(candidate)), rank_(rank)
    {
        // A folder is found by its init.lua alone only when it has no mod.json.
        std::error_code ignored;
        manifested_ = std::filesystem::exists(candidate_.directory / host::kManifestFileName, ignored);
    }

    ScriptMod::~ScriptMod() { Close(); }

    std::filesystem::path ScriptMod::Entry() const
    {
        return candidate_.directory / std::filesystem::u8path(candidate_.manifest.lua);
    }

    bool ScriptMod::Reread(std::string& problem)
    {
        if (!manifested_) return true;
        host::Manifest manifest;
        if (!host::ReadManifest(candidate_.directory / host::kManifestFileName, manifest, problem))
        {
            problem = "its mod.json no longer reads: " + problem;
        }
        else if (manifest.id != Id())
        {
            problem = "its mod.json now gives the id '" + manifest.id +
                      "', and a mod's id cannot change while the client runs; restart the client "
                      "to load it under the new one";
        }
        else if (manifest.lua.empty())
        {
            problem = "its mod.json no longer names a 'lua' entry; restart the client for the "
                      "runtime to take it as a native mod";
        }
        else
        {
            candidate_.manifest = std::move(manifest);
            return true;
        }
        return false;
    }

    // --- loading -----------------------------------------------------------------------------

    bool ScriptMod::Load()
    {
        Close();
        disabled_ = false;
        errors_   = 0;

        // Recorded before anything is read: a change made while the mod loads is a change.
        Rewatch();

        state_ = luaL_newstate();
        if (!state_)
        {
            runtime_.Log(WXL_LOG_ERROR, "mod " + Id() + ": cannot create a Lua state");
            return false;
        }

        OpenSandbox(state_, candidate_.directory, Id(),
                    [this](const std::filesystem::path& file) { Watch(file); });
        OpenObjects(state_);
        Install();

        lua_State* L = state_;
        lua_pushcfunction(L, &L_traceback);
        traceback_ = luaL_ref(L, LUA_REGISTRYINDEX);

        const std::string entry = candidate_.manifest.lua;
        lua_rawgeti(L, LUA_REGISTRYINDEX, traceback_);
        const int msgh = lua_gettop(L);
        if (LoadFile(L, Entry(), Id() + "/" + entry) != 0 || lua_pcall(L, 0, 0, msgh) != 0)
        {
            runtime_.Log(WXL_LOG_ERROR, "mod " + Id() + " failed to load " + entry + ": " +
                                            (lua_tostring(L, -1) ? lua_tostring(L, -1) : "?"));
            Close();
            return false;
        }
        lua_settop(L, 0);

        std::string problem;
        if (!BindFamilies(problem))
        {
            runtime_.Log(WXL_LOG_ERROR, "mod " + Id() + " failed to load " + entry + ": " + problem);
            runtime_.DropHandlers(*this);
            Close();
            return false;
        }
        runtime_.Log(WXL_LOG_INFO, "loaded " + Id() + " " + candidate_.manifest.version.text +
                                       " (" + candidate_.manifest.name + ") from " + entry);
        return true;
    }

    bool ScriptMod::BindFamilies(std::string& error)
    {
        lua_State* L = state_;
        for (const std::string& family : Families())
        {
            const std::string stem = family.substr(0, family.size() - 6);
            lua_getglobal(L, family.c_str());
            if (!lua_istable(L, -1))
            {
                error = family + " must stay a table; it is a " + lua_typename(L, lua_type(L, -1));
                lua_pop(L, 1);
                return false;
            }
            lua_pushvalue(L, -1);
            const int self = luaL_ref(L, LUA_REGISTRYINDEX);
            lua_pushnil(L);
            while (lua_next(L, -2) != 0)
            {
                const char*           key   = lua_type(L, -2) == LUA_TSTRING ? lua_tostring(L, -2) : nullptr;
                const reflect::Event* event = key && std::strncmp(key, "On", 2) == 0
                                                  ? reflect::Find(stem.c_str(), key + 2)
                                                  : nullptr;
                if (!event)
                {
                    error = family + "." + (key ? key : "?") + " is not an event of " + family +
                            "; its events are " + MethodsOf(stem.c_str());
                    lua_pop(L, 3);
                    return false;
                }
                if (!lua_isfunction(L, -1))
                {
                    error = family + "." + key + " must be a function; it is a " +
                            lua_typename(L, lua_type(L, -1));
                    lua_pop(L, 3);
                    return false;
                }
                if (const std::string newer = TooNew(*event); !newer.empty())
                {
                    error = family + "." + key + " " + newer;
                    lua_pop(L, 3);
                    return false;
                }
                lua_pushvalue(L, -1);
                const int ref = luaL_ref(L, LUA_REGISTRYINDEX);
                runtime_.Observe(*this, *event, ref, Answerable(*event), self);
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
        return true;
    }

    void ScriptMod::Close()
    {
        if (state_) lua_close(state_);
        state_     = nullptr;
        traceback_ = LUA_NOREF;
        for (std::vector<int>& refs : callbacks_) refs.clear();
    }

    // --- calling in ------------------------------------------------------------------------

    void ScriptMod::Call(Callback which, float delta)
    {
        if (!Runs()) return;
        lua_State* L = state_;

        // By index over the count there was: a callback registered by a callback waits.
        std::vector<int>& refs  = callbacks_[static_cast<size_t>(which)];
        const size_t      count = refs.size();
        for (size_t i = 0; i < count && Runs(); ++i)
        {
            const int base = lua_gettop(L);
            lua_rawgeti(L, LUA_REGISTRYINDEX, traceback_);
            lua_rawgeti(L, LUA_REGISTRYINDEX, refs[i]);
            int nargs = 0;
            if (which == Callback::Update)
            {
                lua_pushnumber(L, delta);
                nargs = 1;
            }
            if (lua_pcall(L, nargs, 0, base + 1) != 0)
                ReportError(std::string("in its ") + kCallbackNames[static_cast<size_t>(which)],
                            lua_tostring(L, -1) ? lua_tostring(L, -1) : "?");
            lua_settop(L, base);
        }
    }

    void ScriptMod::ReportError(std::string_view what, std::string_view message)
    {
        ++errors_;
        runtime_.Log(WXL_LOG_ERROR, "mod " + Id() + " errored " + std::string(what) + " (" +
                                        std::to_string(errors_) + "/" +
                                        std::to_string(kErrorLimit) + "): " + std::string(message));
        if (errors_ >= kErrorLimit && !disabled_)
            runtime_.Disable(*this, "it errored " + std::to_string(errors_) +
                                        " times since it was loaded");
    }

    // --- the files its code comes from ------------------------------------------------------

    ScriptMod::Watched ScriptMod::Look(const std::filesystem::path& file)
    {
        Watched         seen;
        std::error_code ec;
        seen.file   = file;
        seen.exists = std::filesystem::is_regular_file(file, ec);
        if (!seen.exists) return seen;
        const uintmax_t size = std::filesystem::file_size(file, ec);
        seen.size            = ec ? 0 : static_cast<uint64_t>(size);
        const auto written   = std::filesystem::last_write_time(file, ec);
        seen.written         = ec ? 0 : static_cast<int64_t>(written.time_since_epoch().count());
        return seen;
    }

    void ScriptMod::Watch(const std::filesystem::path& file)
    {
        for (const Watched& known : watched_)
            if (known.file == file) return;
        watched_.push_back(Look(file));
    }

    void ScriptMod::Rewatch()
    {
        watched_.clear();
        if (manifested_) Watch(candidate_.directory / host::kManifestFileName);
        Watch(Entry());
    }

    std::filesystem::path ScriptMod::Changed() const
    {
        for (const Watched& known : watched_)
            if (!Look(known.file).Same(known)) return known.file.lexically_relative(candidate_.directory);
        return std::filesystem::path();
    }

    std::string ScriptMod::TooNew(const reflect::Event& event) const
    {
        const host::AbiVersion since = host::EventSince(event.id);
        const host::AbiVersion asked = candidate_.manifest.abi;
        if (host::AtLeast(asked, since)) return std::string();
        if (!manifested_)
            return "came with abi " + host::AbiText(since) + ", newer than the " +
                   host::AbiText(asked) + " this runtime serves";
        return "came with abi " + host::AbiText(since) + ", and this mod's mod.json asks for " +
               host::AbiText(asked) + "; to use it, set \"abi\": \"" + host::AbiText(since) +
               "\" there";
    }

    // --- the API -----------------------------------------------------------------------------

    ScriptMod& ScriptMod::Self(lua_State* L)
    {
        return *static_cast<ScriptMod*>(lua_touserdata(L, lua_upvalueindex(1)));
    }

    void ScriptMod::PushClosure(lua_CFunction fn)
    {
        lua_pushlightuserdata(state_, this);
        lua_pushcclosure(state_, fn, 1);
    }

    void ScriptMod::Install()
    {
        lua_State* L = state_;

        PushClosure(&L_print);
        lua_setglobal(L, "print");
        PushClosure(&L_registerForEvent);
        lua_setglobal(L, "registerForEvent");
        PushClosure(&L_Observe);
        lua_setglobal(L, "Observe");
        PushClosure(&L_Override);
        lua_setglobal(L, "Override");
        PushClosure(&L_Unobserve);
        lua_setglobal(L, "Unobserve");

        // One table per family: a function defined on it as OnX is subscribed after the entry
        // file has run.
        for (const std::string& family : Families())
        {
            lua_newtable(L);
            lua_setglobal(L, family.c_str());
        }

        lua_newtable(L); // wxl

        PushClosure(&L_log);
        lua_setfield(L, -2, "log");
        PushObjectModule(L);
        lua_setfield(L, -2, "object");

        lua_createtable(L, 0, 4); // wxl.mod
        lua_pushstring(L, Id().c_str());
        lua_setfield(L, -2, "id");
        lua_pushstring(L, candidate_.manifest.name.c_str());
        lua_setfield(L, -2, "name");
        lua_pushstring(L, candidate_.manifest.version.text.c_str());
        lua_setfield(L, -2, "version");
        const std::string directory = candidate_.directory.u8string();
        lua_pushlstring(L, directory.data(), directory.size());
        lua_setfield(L, -2, "directory");
        lua_setfield(L, -2, "mod");

        lua_createtable(L, 0, 7); // wxl.runtime
        if (const wxl_runtime_info* info = runtime_.Host()->runtime())
        {
            lua_pushlstring(L, info->id.data ? info->id.data : "", info->id.data ? info->id.size : 0);
            lua_setfield(L, -2, "id");
            lua_pushlstring(L, info->name.data ? info->name.data : "", info->name.data ? info->name.size : 0);
            lua_setfield(L, -2, "name");
            lua_pushnumber(L, info->version);
            lua_setfield(L, -2, "version");
        }
        // The contract this mod is served, which wxl.lua may cap below the runtime's own.
        const host::AbiVersion abi = runtime_.Abi();
        lua_pushnumber(L, abi.major);
        lua_setfield(L, -2, "abi_major");
        lua_pushnumber(L, abi.minor);
        lua_setfield(L, -2, "abi_minor");
        lua_pushnumber(L, abi.feature);
        lua_setfield(L, -2, "abi_feature");
        const std::string abiText = host::AbiText(abi);
        lua_pushlstring(L, abiText.data(), abiText.size());
        lua_setfield(L, -2, "abi");
        lua_setfield(L, -2, "runtime");

        lua_newtable(L); // wxl.events: family -> { "OnX", ... }
        {
            uint32_t              count  = 0;
            const reflect::Event* events = reflect::Events(&count);
            for (const std::string& family : Families())
            {
                lua_newtable(L);
                int n = 0;
                for (uint32_t i = 0; i < count; ++i)
                {
                    if (family != std::string(events[i].family) + "Script") continue;
                    lua_pushfstring(L, "On%s", events[i].name);
                    lua_rawseti(L, -2, ++n);
                }
                lua_setfield(L, -2, family.c_str());
            }
        }
        lua_setfield(L, -2, "events");

        lua_setglobal(L, "wxl");
    }

    int ScriptMod::L_traceback(lua_State* L)
    {
        const char* message = lua_tostring(L, 1);
        luaL_traceback(L, L, message ? message : "(error object is not a string)", 1);
        return 1;
    }

    int ScriptMod::L_print(lua_State* L)
    {
        ScriptMod& self = Self(L);
        wxl::Log(WXL_LOG_INFO, self.Id().c_str(), Concatenate(L, 1, "\t"));
        return 0;
    }

    int ScriptMod::L_log(lua_State* L)
    {
        ScriptMod&  self  = Self(L);
        const char* level = luaL_checkstring(L, 1);
        wxl_log_level value;
        if (std::strcmp(level, "trace") == 0) value = WXL_LOG_TRACE;
        else if (std::strcmp(level, "debug") == 0) value = WXL_LOG_DEBUG;
        else if (std::strcmp(level, "info") == 0) value = WXL_LOG_INFO;
        else if (std::strcmp(level, "warn") == 0) value = WXL_LOG_WARN;
        else if (std::strcmp(level, "error") == 0) value = WXL_LOG_ERROR;
        else
            return luaL_error(L, "wxl.log: unknown level '%s'; the levels are trace, debug, info, "
                                 "warn, error",
                              level);
        wxl::Log(value, self.Id().c_str(), Concatenate(L, 2, "\t"));
        return 0;
    }

    int ScriptMod::L_registerForEvent(lua_State* L)
    {
        ScriptMod&  self = Self(L);
        const char* name = luaL_checkstring(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);
        for (size_t i = 0; i < static_cast<size_t>(Callback::Count); ++i)
        {
            if (std::strcmp(name, kCallbackNames[i]) != 0) continue;
            lua_pushvalue(L, 2);
            self.callbacks_[i].push_back(luaL_ref(L, LUA_REGISTRYINDEX));
            return 0;
        }
        return luaL_error(L, "registerForEvent: unknown event '%s'; the events are onInit, "
                             "onUpdate, onDraw, onShutdown",
                          name);
    }

    int ScriptMod::L_Observe(lua_State* L) { return Self(L).Subscribe(L, false); }

    int ScriptMod::L_Override(lua_State* L) { return Self(L).Subscribe(L, true); }

    int ScriptMod::Subscribe(lua_State* L, bool override)
    {
        const char* verb   = override ? "Override" : "Observe";
        const char* family = luaL_checkstring(L, 1);
        const char* method = luaL_checkstring(L, 2);
        luaL_checktype(L, 3, LUA_TFUNCTION);

        // "WorldScript" names the family "World" of reflect.hpp; "OnUpdate" its event "Update".
        const size_t familyLength = std::strlen(family);
        std::string  stem;
        if (familyLength > 6 && std::strcmp(family + familyLength - 6, "Script") == 0)
            stem.assign(family, familyLength - 6);
        bool knownFamily = false;
        for (const std::string& known : Families())
            if (known == family) knownFamily = true;
        if (!knownFamily)
            return luaL_error(L, "%s: unknown family '%s'; the families are %s", verb, family,
                              Join(Families()).c_str());

        const reflect::Event* event =
            std::strncmp(method, "On", 2) == 0 ? reflect::Find(stem.c_str(), method + 2) : nullptr;
        if (!event)
            return luaL_error(L, "%s: %s has no '%s'; its events are %s", verb, family, method,
                              MethodsOf(stem.c_str()).c_str());

        if (const std::string newer = TooNew(*event); !newer.empty())
            return luaL_error(L, "%s: %s of %s %s", verb, method, family, newer.c_str());

        if (override && !Answerable(*event))
            return luaL_error(L, "Override: %s of %s cannot be overridden: its args carry no "
                                 "answer or buffer member; use Observe",
                              method, family);

        lua_pushvalue(L, 3);
        const int      ref = luaL_ref(L, LUA_REGISTRYINDEX);
        const uint32_t id  = runtime_.Observe(*this, *event, ref, override);
        lua_pushnumber(L, id);
        return 1;
    }

    int ScriptMod::L_Unobserve(lua_State* L)
    {
        ScriptMod& self = Self(L);
        const uint32_t id = static_cast<uint32_t>(luaL_checkinteger(L, 1));
        lua_pushboolean(L, self.runtime_.Unobserve(self, id));
        return 1;
    }
} // namespace wxl::lua
