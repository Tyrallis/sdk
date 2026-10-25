// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Shims.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "ScriptMod.hpp"
#include "Services.hpp"
#include "wxl/reflect.hpp"
#include "wxl/sdk.hpp"
#include "wxl/services/rows.hpp"
#include "wxl/services/script.h"
#include "wxl/services/storage.h"

namespace wxl::lua::shims
{
    namespace
    {
        constexpr const char* kTag = "wxl.lua";

        /// A context as a bit: the glue's is 1, the game's 2.
        uint32_t Bit(uint32_t context) { return context < 32 ? (1u << context) : 0u; }

        constexpr uint32_t kEveryContext = ~0u;

        /**
         * @brief One global name a Lua mod registered, and the native function behind it.
         *
         * Never freed: the client keeps calling the function it was handed as long as its
         * context lives, whoever holds the name now. A mod that reloads finds its name here,
         * already bound, and only points it at its new function.
         */
        struct Function
        {
            std::string name;
            ScriptMod*  mod      = nullptr; ///< who answers it; null while nobody does
            int         ref      = LUA_NOREF;
            uint32_t    contexts = kEveryContext;
            uint32_t    bound    = 0;       ///< the contexts in which the client holds it
            bool        warned   = false;   ///< a return value that cannot cross was logged
        };

        struct Event
        {
            std::string name;
            ScriptMod*  mod;
            uint32_t    contexts;
        };

        std::deque<Function> g_functions;
        std::vector<Event>   g_events;
        wxl::Subscription    g_ready;
        wxl::Subscription    g_closing;
        int                  g_calling = 0;

        const wxl_script_api* Api() { return wxl::api::Table<wxl_script_api>(); }

        ScriptMod& Self(lua_State* L)
        {
            return *static_cast<ScriptMod*>(lua_touserdata(L, lua_upvalueindex(1)));
        }

        /// The optional context argument: every context, or the one it names.
        uint32_t CheckContexts(lua_State* L, int index)
        {
            if (lua_isnoneornil(L, index)) return kEveryContext;
            const char*                     name = luaL_checkstring(L, index);
            const wxl::reflect::Enum* const contexts = wxl::reflect::FindEnum("wxl_script_context");
            for (uint32_t i = 0; contexts && i < contexts->count; ++i)
                if (std::strcmp(contexts->values[i].name, name) == 0) return Bit(contexts->values[i].value);
            luaL_argerror(L, index, "a context: \"glue\" or \"game\", or nil for both");
            return 0;
        }

        // --- the function the client calls --------------------------------------------------

        /// The first line of an error, without the traceback the log keeps.
        std::string FirstLine(const char* message)
        {
            const char* end = std::strchr(message, '\n');
            return end ? std::string(message, end) : std::string(message);
        }

        /// Runs a registered Lua function: the call's arguments as Lua values, its returns as
        /// the call's. A Lua error fails the call with the error, and counts against the mod.
        void WXL_CALL Invoke(wxl_script_call* call, void* user)
        {
            Function&             function = *static_cast<Function*>(user);
            const wxl_script_api* api      = Api();
            if (!api) return;

            if (!function.mod || !function.mod->Runs())
            {
                const std::string why = function.name + ": the Lua mod that registered it is not running";
                api->fail(call, Str(why));
                return;
            }

            ScriptMod& mod = *function.mod;
            lua_State* L   = mod.State();
            const int  base = lua_gettop(L);
            const uint32_t count = api->arg_count(call);
            if (!lua_checkstack(L, static_cast<int>(count) + 2))
            {
                api->fail(call, Str(function.name + ": too many arguments for the Lua stack"));
                return;
            }

            ++g_calling;
            lua_rawgeti(L, LUA_REGISTRYINDEX, mod.TracebackRef());
            lua_rawgeti(L, LUA_REGISTRYINDEX, function.ref);
            for (uint32_t i = 0; i < count; ++i)
            {
                switch (api->arg_type(call, i))
                {
                case WXL_VALUE_NUMBER:
                {
                    double number = 0;
                    api->arg_number(call, i, &number);
                    lua_pushnumber(L, number);
                    break;
                }
                case WXL_VALUE_STRING:
                {
                    wxl_str text{nullptr, 0};
                    api->arg_string(call, i, &text);
                    lua_pushlstring(L, text.data ? text.data : "", text.data ? text.size : 0);
                    break;
                }
                case WXL_VALUE_BOOL:
                {
                    wxl_bool flag = WXL_FALSE;
                    api->arg_bool(call, i, &flag);
                    lua_pushboolean(L, flag == WXL_TRUE);
                    break;
                }
                default: lua_pushnil(L); break;
                }
            }

            if (lua_pcall(L, static_cast<int>(count), LUA_MULTRET, base + 1) != 0)
            {
                const char* message = lua_tostring(L, -1) ? lua_tostring(L, -1) : "?";
                api->fail(call, Str(function.name + ": " + FirstLine(message)));
                mod.ReportError("in its script function " + function.name, message);
                lua_settop(L, base);
                --g_calling;
                return;
            }

            const int top = lua_gettop(L);
            for (int i = base + 2; i <= top; ++i)
            {
                switch (lua_type(L, i))
                {
                case LUA_TNUMBER: api->push_number(call, lua_tonumber(L, i)); break;
                case LUA_TBOOLEAN: api->push_bool(call, lua_toboolean(L, i) ? WXL_TRUE : WXL_FALSE); break;
                case LUA_TSTRING:
                {
                    size_t      size = 0;
                    const char* text = lua_tolstring(L, i, &size);
                    api->push_string(call, wxl_str{text, static_cast<uint32_t>(size)});
                    break;
                }
                case LUA_TNIL: api->push_nil(call); break;
                default:
                    api->push_nil(call);
                    if (!function.warned)
                    {
                        function.warned = true;
                        wxl::Log(WXL_LOG_WARN, kTag,
                                 "mod " + mod.Id() + ": script function " + function.name + " returned a " +
                                     lua_typename(L, lua_type(L, i)) +
                                     ", which the client's script cannot take; it receives nil (a function "
                                     "returns numbers, strings, booleans or nil)");
                    }
                    break;
                }
            }
            lua_settop(L, base);
            --g_calling;
        }

        /// Binds a function in the live context when it should be there and is not.
        wxl_result Bind(Function& function, uint32_t context)
        {
            const wxl_script_api* api = Api();
            if (!api || !wxl::detail::Carries(api, &wxl_script_api::register_function)) return WXL_E_UNSUPPORTED;
            if (!(function.contexts & Bit(context)) || (function.bound & Bit(context))) return WXL_OK;
            const wxl_result result = api->register_function(function.name.c_str(), &Invoke, &function);
            if (result == WXL_OK) function.bound |= Bit(context);
            return result;
        }

        wxl_result Declare(const Event& event, uint32_t context)
        {
            const wxl_script_api* api = Api();
            if (!api || !wxl::detail::Carries(api, &wxl_script_api::register_event)) return WXL_E_UNSUPPORTED;
            if (!(event.contexts & Bit(context))) return WXL_OK;
            return api->register_event(event.name.c_str());
        }

        /// The live context, or kEveryContext when the service cannot say.
        uint32_t Live()
        {
            const wxl_script_api* api = Api();
            return api && wxl::detail::Carries(api, &wxl_script_api::context) ? api->context() : kEveryContext;
        }

        // --- what a new context and a closing one do -----------------------------------------

        void WXL_CALL OnReady(void*, const void* args)
        {
            if (!args) return;
            const uint32_t context = static_cast<const wxl_script_vm_args*>(args)->context;
            for (Function& function : g_functions)
            {
                if (!function.mod || !function.mod->Runs()) continue;
                const wxl_result result = Bind(function, context);
                if (result != WXL_OK && result != WXL_E_FAILED)
                    wxl::Log(WXL_LOG_WARN, kTag,
                             "mod " + function.mod->Id() + ": script function " + function.name +
                                 " was not registered in the new context (" + ReasonOf(result) +
                                 "): the name is taken there");
            }
            for (const Event& event : g_events)
                if (event.mod && event.mod->Runs()) Declare(event, context);
        }

        void WXL_CALL OnClosing(void*, const void* args)
        {
            if (!args) return;
            const uint32_t context = static_cast<const wxl_script_vm_args*>(args)->context;
            for (Function& function : g_functions) function.bound &= ~Bit(context);
        }

        // --- what a Lua mod calls --------------------------------------------------------------

        int L_RegisterFunction(lua_State* L)
        {
            ScriptMod&     mod      = Self(L);
            const char*    name     = luaL_checkstring(L, 1);
            luaL_checktype(L, 2, LUA_TFUNCTION);
            const uint32_t contexts = CheckContexts(L, 3);
            if (!*name) return luaL_argerror(L, 1, "a name");

            const wxl_script_api* api = Api();
            if (!api || !wxl::detail::Carries(api, &wxl_script_api::register_function))
                return PushResultFailure(L, WXL_E_UNSUPPORTED);

            Function* function = nullptr;
            for (Function& known : g_functions)
                if (known.name == name) function = &known;
            if (function && function->mod && function->mod != &mod)
            {
                wxl::Log(WXL_LOG_WARN, kTag,
                         "mod " + mod.Id() + ": RegisterFunction '" + name + "': mod " + function->mod->Id() +
                             " registered that name already");
                return PushResultFailure(L, WXL_E_INVALID);
            }
            if (!function)
            {
                g_functions.emplace_back();
                function       = &g_functions.back();
                function->name = name;
            }
            if (function->mod == &mod && function->ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, function->ref);
            lua_pushvalue(L, 2);
            function->ref      = luaL_ref(L, LUA_REGISTRYINDEX);
            function->mod      = &mod;
            function->contexts = contexts;
            function->warned   = false;

            // Bound now when a context it belongs to is live; otherwise at the next one.
            const wxl_result result = Bind(*function, Live());
            if (result != WXL_OK && result != WXL_E_FAILED)
            {
                luaL_unref(L, LUA_REGISTRYINDEX, function->ref);
                function->ref = LUA_NOREF;
                function->mod = nullptr;
                wxl::Log(WXL_LOG_WARN, kTag,
                         "mod " + mod.Id() + ": RegisterFunction '" + name + "': " +
                             (result == WXL_E_INVALID ? "the name is taken in the client's script"
                                                      : std::string("refused (") + ReasonOf(result) + ")"));
                return PushResultFailure(L, result);
            }
            lua_pushboolean(L, 1);
            return 1;
        }

        int L_RegisterEvent(lua_State* L)
        {
            ScriptMod&     mod      = Self(L);
            const char*    name     = luaL_checkstring(L, 1);
            const uint32_t contexts = CheckContexts(L, 2);
            if (!*name) return luaL_argerror(L, 1, "a name");

            const wxl_script_api* api = Api();
            if (!api || !wxl::detail::Carries(api, &wxl_script_api::register_event))
                return PushResultFailure(L, WXL_E_UNSUPPORTED);

            bool known = false;
            for (const Event& event : g_events)
                if (event.mod == &mod && event.name == name) known = true;
            if (!known) g_events.push_back(Event{name, &mod, contexts});

            const uint32_t   live   = Live();
            const wxl_result result = (contexts & Bit(live)) ? api->register_event(name) : WXL_OK;
            if (result != WXL_OK && result != WXL_E_FAILED) return PushResultFailure(L, result);
            lua_pushboolean(L, 1);
            return 1;
        }

        // --- wxl.storage: the folders each mod adds -----------------------------------------
        //
        // The host sees every folder a Lua mod adds as wxl.lua's. Which mod added it is kept
        // here, so that a mod removes only its own and its folders go when it does; a folder two
        // mods add stays until the last of them lets it go.

        struct Folder
        {
            std::string             key;       ///< the folder, absolute, lower case, backslashes
            std::string             directory; ///< as the first mod gave it
            std::vector<ScriptMod*> holders;
        };

        std::vector<Folder> g_folders;

        const wxl_storage_api* Storage() { return wxl::api::Table<wxl_storage_api>(); }

        /// One folder however it is spelled: absolute against the current directory, which is
        /// the client's folder, as the runtime reads it.
        std::string FolderKey(const std::string& directory)
        {
            std::error_code       ignored;
            std::filesystem::path path = std::filesystem::absolute(std::filesystem::u8path(directory), ignored);
            std::string           key  = path.lexically_normal().u8string();
            for (char& c : key) c = c == '/' ? '\\' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            while (key.size() > 3 && key.back() == '\\') key.pop_back();
            return key;
        }

        /// The string at an argument the script may not leave out.
        std::string CheckDirectory(lua_State* L, const char* function)
        {
            if (lua_isnoneornil(L, 1))
                luaL_error(L, "wxl.storage.%s: 'directory' (argument 1) is missing; it takes a string", function);
            size_t      size = 0;
            const char* text = luaL_checklstring(L, 1, &size);
            return std::string(text, size);
        }

        /// Takes a mod off a folder; the runtime lets the folder go with its last holder.
        void LetGo(size_t index, ScriptMod& mod)
        {
            Folder& folder = g_folders[index];
            folder.holders.erase(std::remove(folder.holders.begin(), folder.holders.end(), &mod), folder.holders.end());
            if (!folder.holders.empty()) return;
            const wxl_storage_api* api = Storage();
            if (api && wxl::detail::Carries(api, &wxl_storage_api::remove_search_path))
                api->remove_search_path(Str(folder.directory));
            g_folders.erase(g_folders.begin() + static_cast<std::ptrdiff_t>(index));
        }

        int L_AddSearchPath(lua_State* L)
        {
            ScriptMod&        mod       = Self(L);
            const std::string directory = CheckDirectory(L, "AddSearchPath");
            if (lua_isnoneornil(L, 2))
                return luaL_error(L, "wxl.storage.AddSearchPath: 'priority' (argument 2) is missing; it takes a "
                                     "whole number");
            const lua_Number priority = luaL_checknumber(L, 2);
            if (std::floor(priority) != priority || priority < INT32_MIN || priority > INT32_MAX)
                return luaL_argerror(L, 2, "a whole number from -2147483648 to 2147483647");

            const wxl_storage_api* api = Storage();
            if (!api || !wxl::detail::Carries(api, &wxl_storage_api::add_search_path))
                return PushResultFailure(L, WXL_E_UNSUPPORTED);
            const wxl_result result = api->add_search_path(Str(directory), static_cast<int32_t>(priority));
            if (result != WXL_OK) return PushResultFailure(L, result);

            const std::string key   = FolderKey(directory);
            const auto        known = std::find_if(g_folders.begin(), g_folders.end(),
                                                   [&](const Folder& folder) { return folder.key == key; });
            if (known == g_folders.end())
                g_folders.push_back(Folder{key, directory, {&mod}});
            else if (std::find(known->holders.begin(), known->holders.end(), &mod) == known->holders.end())
                known->holders.push_back(&mod);
            lua_pushboolean(L, 1);
            return 1;
        }

        int L_RemoveSearchPath(lua_State* L)
        {
            ScriptMod&        mod       = Self(L);
            const std::string directory = CheckDirectory(L, "RemoveSearchPath");
            const wxl_storage_api* api = Storage();
            if (!api || !wxl::detail::Carries(api, &wxl_storage_api::remove_search_path))
                return PushResultFailure(L, WXL_E_UNSUPPORTED);

            const std::string key = FolderKey(directory);
            for (size_t i = 0; i < g_folders.size(); ++i)
            {
                const Folder& folder = g_folders[i];
                if (folder.key != key ||
                    std::find(folder.holders.begin(), folder.holders.end(), &mod) == folder.holders.end())
                    continue;
                LetGo(i, mod);
                lua_pushboolean(L, 1);
                return 1;
            }
            return PushResultFailure(L, WXL_E_NOT_FOUND);
        }
    } // namespace

    lua_CFunction Find(std::string_view service, std::string_view name)
    {
        if (service == "script" && name == "RegisterFunction") return &L_RegisterFunction;
        if (service == "script" && name == "RegisterEvent") return &L_RegisterEvent;
        if (service == "storage" && name == "AddSearchPath") return &L_AddSearchPath;
        if (service == "storage" && name == "RemoveSearchPath") return &L_RemoveSearchPath;
        return nullptr;
    }

    void ReleaseFolders(ScriptMod& mod)
    {
        for (size_t i = g_folders.size(); i-- > 0;)
            if (std::find(g_folders[i].holders.begin(), g_folders[i].holders.end(), &mod) != g_folders[i].holders.end())
                LetGo(i, mod);
    }

    void Start()
    {
        g_ready   = wxl::Subscribe(WXL_EV_ScriptVmReady, &OnReady, nullptr, INT32_MIN);
        g_closing = wxl::Subscribe(WXL_EV_ScriptVmClosing, &OnClosing, nullptr, INT32_MAX);
    }

    void Stop()
    {
        g_ready.Reset();
        g_closing.Reset();
        for (Function& function : g_functions)
        {
            function.mod = nullptr;
            function.ref = LUA_NOREF;
        }
        g_events.clear();
    }

    void Forget(ScriptMod& mod)
    {
        ReleaseFolders(mod);
        // The state closes right after: its references go with it, nothing to release here.
        for (Function& function : g_functions)
        {
            if (function.mod != &mod) continue;
            function.mod = nullptr;
            function.ref = LUA_NOREF;
        }
        for (size_t i = g_events.size(); i-- > 0;)
            if (g_events[i].mod == &mod) g_events.erase(g_events.begin() + static_cast<std::ptrdiff_t>(i));
    }

    bool Calling() { return g_calling != 0; }
} // namespace wxl::lua::shims
