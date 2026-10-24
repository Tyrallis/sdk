// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Objects.hpp"

#include "ObjectNames.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "wxl/events.h"
#include "wxl/sdk.hpp"
#include "wxl/services/object.h"

namespace wxl::lua
{
    namespace
    {
        constexpr const char* kObjectMeta = "wxl.object";

        /// The service table, resolved on first use and retried while absent.
        const wxl_object_api* Api()
        {
            static wxl::Service<wxl_object_api> service(WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION);
            return service.Get();
        }

        using names::KindBit;
        using names::TypeName;
        using names::kAny;
        using names::kUnits;
        using names::kPlayers;
        using names::kItems;
        using names::kContainers;
        using names::kGameObjects;
        using names::kDynamics;
        using names::kCorpses;

        /// A type by its name, or WXL_OBJECT_UNKNOWN.
        uint32_t TypeByName(const char* name)
        {
            for (uint32_t type = WXL_OBJECT_ITEM; type <= WXL_OBJECT_CORPSE; ++type)
                if (std::strcmp(TypeName(type), name) == 0) return type;
            return WXL_OBJECT_UNKNOWN;
        }

        // --- pushing what a getter wrote ----------------------------------------------------

        void PushValue(lua_State* L, uint32_t value) { lua_pushnumber(L, value); }
        void PushValue(lua_State* L, float value) { lua_pushnumber(L, value); }
        void PushValue(lua_State* L, wxl_guid value) { PushGuid(L, value); }
        void PushValue(lua_State* L, wxl_vec3 value)
        {
            lua_createtable(L, 0, 3);
            lua_pushnumber(L, value.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, value.y);
            lua_setfield(L, -2, "y");
            lua_pushnumber(L, value.z);
            lua_setfield(L, -2, "z");
        }

        /// nil and the reason a read gave nothing: what a Lua caller gets instead of a value.
        int PushFailure(lua_State* L, wxl_result result)
        {
            lua_pushnil(L);
            switch (result)
            {
            case WXL_E_NOT_FOUND: lua_pushliteral(L, "gone"); break;
            case WXL_E_INVALID: lua_pushliteral(L, "invalid"); break;
            case WXL_E_UNSUPPORTED: lua_pushliteral(L, "unsupported"); break;
            default: lua_pushliteral(L, "failed"); break;
            }
            return 2;
        }

        /// One getter of the table, called from Lua: the handle, and for an indexed getter the
        /// index at argument 2.
        using Call = int (*)(lua_State* L, const wxl_object_api* api, wxl_handle handle);

        template <class V, wxl_result(WXL_CALL* wxl_object_api::*Getter)(wxl_handle, V*)>
        int Read(lua_State* L, const wxl_object_api* api, wxl_handle handle)
        {
            if (!wxl::detail::Carries(api, Getter)) return PushFailure(L, WXL_E_UNSUPPORTED);
            V                value{};
            const wxl_result result = (api->*Getter)(handle, &value);
            if (result != WXL_OK) return PushFailure(L, result);
            PushValue(L, value);
            return 1;
        }

        template <class V, wxl_result(WXL_CALL* wxl_object_api::*Getter)(wxl_handle, uint32_t, V*)>
        int ReadAt(lua_State* L, const wxl_object_api* api, wxl_handle handle)
        {
            if (!wxl::detail::Carries(api, Getter)) return PushFailure(L, WXL_E_UNSUPPORTED);
            const uint32_t   index = static_cast<uint32_t>(luaL_checkinteger(L, 2));
            V                value{};
            const wxl_result result = (api->*Getter)(handle, index, &value);
            if (result != WXL_OK) return PushFailure(L, result);
            PushValue(L, value);
            return 1;
        }

        /// One row of object_reads.def.
        struct Row
        {
            const char* member;
            uint32_t    kinds;
            Call        call;
            std::string name; ///< the Lua method
        };

        const std::vector<Row>& Rows()
        {
            static const std::vector<Row> rows = [] {
                std::vector<Row> table = {
#define WXL_OBJECT_READ(kinds, member, Method, type) \
    Row{#member, k##kinds, &Read<type, &wxl_object_api::member>, std::string()},
#define WXL_OBJECT_READ_AT(kinds, member, Method, type) \
    Row{#member, k##kinds, &ReadAt<type, &wxl_object_api::member>, std::string()},
#include "wxl/runtime/object_reads.def"
#undef WXL_OBJECT_READ_AT
#undef WXL_OBJECT_READ
                };
                std::vector<names::Member> members;
                for (const Row& row : table) members.push_back(names::Member{row.member, row.kinds});
                const std::vector<std::string> luaNames = names::LuaNames(members);
                for (size_t i = 0; i < table.size(); ++i) table[i].name = luaNames[i];
                return table;
            }();
            return rows;
        }

        // --- identity -----------------------------------------------------------------------
        //
        // Each state keeps, in its registry:
        //
        //   the cache   handle -> userdata, weak in its values: while a mod holds an object, every
        //               push of that handle answers the same userdata, so an object is one Lua
        //               value (a table key, `==` by identity). Once nothing holds it, it is
        //               collected and the next push makes another.
        //   the data    handle -> the table of what the mod stored on the object (`obj.foo = x`),
        //               strong: it outlives the userdata, so it is there again at the next push.
        //
        // A handle is the key in both, as its eight bytes in a string: a Lua number cannot hold
        // every 64-bit value. At ObjectDestroy (Forget) both entries go and a userdata still held
        // is marked dead. The userdata's environment is its data table, or one shared empty
        // table until something is stored, so reading a stored field needs no key of its own.

        char kCacheKey;
        char kDataKey;
        char kEmptyKey;

        struct ObjectBox
        {
            wxl_handle handle;
            uint32_t   kind; ///< the wxl_object_type read at the first push; it never changes
            bool       dead; ///< ObjectDestroy has passed: every getter answers "gone"
        };

        void PushKey(lua_State* L, wxl_handle handle)
        {
            lua_pushlstring(L, reinterpret_cast<const char*>(&handle), sizeof handle);
        }

        void PushRegistry(lua_State* L, const char* key)
        {
            lua_pushlightuserdata(L, const_cast<char*>(key));
            lua_rawget(L, LUA_REGISTRYINDEX);
        }

        ObjectBox* CheckBox(lua_State* L, int index)
        {
            return static_cast<ObjectBox*>(luaL_checkudata(L, index, kObjectMeta));
        }

        /// The service, unless the object is gone: nullptr with the failure pushed otherwise.
        const wxl_object_api* Live(lua_State* L, const ObjectBox* box, int& pushed)
        {
            pushed = 0;
            if (box->dead)
            {
                pushed = PushFailure(L, WXL_E_NOT_FOUND);
                return nullptr;
            }
            const wxl_object_api* api = Api();
            if (!api) pushed = PushFailure(L, WXL_E_UNSUPPORTED);
            return api;
        }

        // --- the methods --------------------------------------------------------------------

        /// A generated method of a kind that has it: upvalue 1 is its row.
        int Generated(lua_State* L)
        {
            const ObjectBox*      box    = CheckBox(L, 1);
            int                   pushed = 0;
            const wxl_object_api* api    = Live(L, box, pushed);
            if (!api) return pushed;
            const Row& row = Rows()[static_cast<size_t>(lua_tointeger(L, lua_upvalueindex(1)))];
            return row.call(L, api, box->handle);
        }

        /// A generated method on a kind without it: upvalue 1 is its name.
        int Absent(lua_State* L)
        {
            const ObjectBox*      box    = CheckBox(L, 1);
            int                   pushed = 0;
            const wxl_object_api* api    = Live(L, box, pushed);
            if (!api) return pushed;
            if (box->kind == WXL_OBJECT_UNKNOWN) return PushFailure(L, WXL_E_NOT_FOUND);
            lua_pushnil(L);
            lua_pushfstring(L, "a %s has no %s", TypeName(box->kind), lua_tostring(L, lua_upvalueindex(1)));
            return 2;
        }

        int Valid(lua_State* L)
        {
            const ObjectBox* box = CheckBox(L, 1);
            if (box->dead)
            {
                lua_pushboolean(L, 0);
                return 1;
            }
            const wxl_object_api* api = Api();
            if (!api) return PushFailure(L, WXL_E_UNSUPPORTED);
            lua_pushboolean(L, api->valid(box->handle) == WXL_TRUE);
            return 1;
        }

        int Guid(lua_State* L)
        {
            const ObjectBox*      box    = CheckBox(L, 1);
            int                   pushed = 0;
            const wxl_object_api* api    = Live(L, box, pushed);
            if (!api) return pushed;
            const wxl_guid guid = api->guid(box->handle);
            if (guid == WXL_GUID_NONE && api->valid(box->handle) != WXL_TRUE)
                return PushFailure(L, WXL_E_NOT_FOUND);
            PushGuid(L, guid);
            return 1;
        }

        /// The kind, checked live: a handle the runtime retired without an event is gone too.
        bool LiveKind(lua_State* L, uint32_t& kind, int& pushed)
        {
            const ObjectBox*      box = CheckBox(L, 1);
            const wxl_object_api* api = Live(L, box, pushed);
            if (!api) return false;
            kind = api->type(box->handle);
            if (kind != WXL_OBJECT_UNKNOWN) return true;
            pushed = PushFailure(L, WXL_E_NOT_FOUND);
            return false;
        }

        int Type(lua_State* L)
        {
            uint32_t kind   = WXL_OBJECT_UNKNOWN;
            int      pushed = 0;
            if (!LiveKind(L, kind, pushed)) return pushed;
            lua_pushstring(L, TypeName(kind));
            return 1;
        }

        int TypeId(lua_State* L)
        {
            uint32_t kind   = WXL_OBJECT_UNKNOWN;
            int      pushed = 0;
            if (!LiveKind(L, kind, pushed)) return pushed;
            lua_pushnumber(L, kind);
            return 1;
        }

        int Name(lua_State* L)
        {
            const ObjectBox*      box    = CheckBox(L, 1);
            int                   pushed = 0;
            const wxl_object_api* api    = Live(L, box, pushed);
            if (!api) return pushed;
            wxl_str          name{nullptr, 0};
            const wxl_result result = api->name(box->handle, &name);
            if (result != WXL_OK) return PushFailure(L, result);
            lua_pushlstring(L, name.data ? name.data : "", name.data ? name.size : 0);
            return 1;
        }

        int Target(lua_State* L)
        {
            const ObjectBox*      box    = CheckBox(L, 1);
            int                   pushed = 0;
            const wxl_object_api* api    = Live(L, box, pushed);
            if (!api) return pushed;
            wxl_handle       target = WXL_HANDLE_NONE;
            const wxl_result result = api->unit_target(box->handle, &target);
            if (result != WXL_OK) return PushFailure(L, result);
            PushObject(L, target);
            return 1;
        }

        int Handle(lua_State* L)
        {
            char text[24];
            std::snprintf(text, sizeof text, "0x%016llX",
                          static_cast<unsigned long long>(CheckBox(L, 1)->handle));
            lua_pushstring(L, text);
            return 1;
        }

        int ToString(lua_State* L)
        {
            const ObjectBox* box = CheckBox(L, 1);
            char             text[56];
            std::snprintf(text, sizeof text, "%s%s(0x%016llX)", box->dead ? "gone " : "",
                          box->kind == WXL_OBJECT_UNKNOWN ? "Object" : TypeName(box->kind),
                          static_cast<unsigned long long>(box->handle));
            lua_pushstring(L, text);
            return 1;
        }

        // --- indexing -----------------------------------------------------------------------

        /// obj.key: the method of the object's kind (upvalue 1: one methods table per kind), or
        /// what the mod stored under the key (the userdata's environment).
        int Index(lua_State* L)
        {
            const ObjectBox* box = static_cast<const ObjectBox*>(lua_touserdata(L, 1));
            lua_rawgeti(L, lua_upvalueindex(1), static_cast<int>(box->kind));
            lua_pushvalue(L, 2);
            lua_rawget(L, -2);
            if (!lua_isnil(L, -1)) return 1;
            lua_pop(L, 2);
            lua_getfenv(L, 1);
            lua_pushvalue(L, 2);
            lua_rawget(L, -2);
            return 1;
        }

        /// obj.key = value: stored for the mod, kept until the object leaves. A method's name
        /// (upvalue 1: every name a method has on any kind) is refused, as is a gone object.
        int NewIndex(lua_State* L)
        {
            ObjectBox* box = static_cast<ObjectBox*>(lua_touserdata(L, 1));
            lua_pushvalue(L, 2);
            lua_rawget(L, lua_upvalueindex(1));
            if (!lua_isnil(L, -1))
                return luaL_error(L, "'%s' is a method of objects; store your data under another name",
                                  lua_tostring(L, 2));
            lua_pop(L, 1);
            if (lua_isnil(L, 2)) return luaL_error(L, "an object's field cannot be named nil");

            const wxl_object_api* api = Api();
            if (box->dead || (api && api->valid(box->handle) != WXL_TRUE))
            {
                lua_pushvalue(L, 2);
                const char* key = lua_type(L, 2) == LUA_TSTRING ? lua_tostring(L, -1) : "a field";
                return luaL_error(L, "cannot store '%s' on an object that is gone", key);
            }

            lua_getfenv(L, 1);
            PushRegistry(L, &kEmptyKey);
            if (lua_rawequal(L, -1, -2))
            {
                // The first field: the object gets its own table, kept in the data by handle.
                lua_pop(L, 2);
                lua_newtable(L);
                PushRegistry(L, &kDataKey);
                PushKey(L, box->handle);
                lua_pushvalue(L, -3);
                lua_rawset(L, -3);
                lua_pop(L, 1);
                lua_pushvalue(L, -1);
                lua_setfenv(L, 1);
            }
            else
            {
                lua_pop(L, 1);
            }
            lua_pushvalue(L, 2);
            lua_pushvalue(L, 3);
            lua_rawset(L, -3);
            return 0;
        }

        // --- wxl.object ---------------------------------------------------------------------

        int LocalPlayer(lua_State* L)
        {
            const wxl_object_api* api = Api();
            PushObject(L, api ? api->local_player() : WXL_HANDLE_NONE);
            return 1;
        }

        int FromGuid(lua_State* L)
        {
            wxl_guid guid = WXL_GUID_NONE;
            if (!ToGuid(L, 1, guid))
                return luaL_error(L, "FromGuid takes a guid: \"0x\" and up to sixteen hex digits, like "
                                     "\"0x0F130000000012AB\", or a whole number");
            const wxl_object_api* api = Api();
            PushObject(L, api ? api->from_guid(guid) : WXL_HANDLE_NONE);
            return 1;
        }

        int InView(lua_State* L)
        {
            uint32_t type = WXL_OBJECT_UNKNOWN;
            if (lua_type(L, 1) == LUA_TSTRING)
            {
                type = TypeByName(lua_tostring(L, 1));
                if (type == WXL_OBJECT_UNKNOWN)
                    return luaL_error(L, "InView: unknown type '%s'; the types are Item, Container, "
                                         "Unit, Player, GameObject, DynamicObject, Corpse",
                                      lua_tostring(L, 1));
            }
            else if (!lua_isnoneornil(L, 1))
            {
                type = static_cast<uint32_t>(luaL_checkinteger(L, 1));
            }

            lua_newtable(L);
            const wxl_object_api* api = Api();
            if (!api) return 1;

            std::vector<wxl_handle> handles(api->enumerate(type, nullptr, 0));
            const uint32_t count = api->enumerate(type, handles.data(), static_cast<uint32_t>(handles.size()));
            const uint32_t given = count < handles.size() ? count : static_cast<uint32_t>(handles.size());
            for (uint32_t i = 0; i < given; ++i)
            {
                PushObject(L, handles[i]);
                lua_rawseti(L, -2, static_cast<int>(i + 1));
            }
            return 1;
        }
    } // namespace

    void OpenObjects(lua_State* L)
    {
        const luaL_Reg own[] = {
            {"Valid", &Valid}, {"Guid", &Guid},     {"Type", &Type},     {"TypeId", &TypeId},
            {"Name", &Name},   {"Target", &Target}, {"Handle", &Handle}, {nullptr, nullptr},
        };

        // Every name a method has, on any kind: what obj.key = value may not use.
        lua_newtable(L);
        const int reserved = lua_gettop(L);
        for (const luaL_Reg* method = own; method->name; ++method)
        {
            lua_pushboolean(L, 1);
            lua_setfield(L, reserved, method->name);
        }
        for (const Row& row : Rows())
        {
            lua_pushboolean(L, 1);
            lua_setfield(L, reserved, row.name.c_str());
        }

        // One methods table per kind, built once: a call looks its method up and runs it, with
        // no walk of the rows. A generated name the kind lacks answers why.
        lua_createtable(L, WXL_OBJECT_CORPSE + 1, 0);
        const int byKind = lua_gettop(L);
        const std::vector<Row>& rows = Rows();
        for (uint32_t kind = WXL_OBJECT_UNKNOWN; kind <= WXL_OBJECT_CORPSE; ++kind)
        {
            lua_newtable(L);
            for (size_t i = 0; i < rows.size(); ++i)
            {
                lua_getfield(L, -1, rows[i].name.c_str());
                const bool named = !lua_isnil(L, -1);
                lua_pop(L, 1);
                const bool accepts = kind != WXL_OBJECT_UNKNOWN &&
                                     (rows[i].kinds & KindBit(static_cast<wxl_object_type>(kind))) != 0;
                if (accepts)
                {
                    lua_pushinteger(L, static_cast<lua_Integer>(i));
                    lua_pushcclosure(L, &Generated, 1);
                }
                else if (!named)
                {
                    lua_pushstring(L, rows[i].name.c_str());
                    lua_pushcclosure(L, &Absent, 1);
                }
                else
                {
                    continue; // another row of the name already answers for this kind
                }
                lua_setfield(L, -2, rows[i].name.c_str());
            }
            for (const luaL_Reg* method = own; method->name; ++method)
            {
                lua_pushcfunction(L, method->func);
                lua_setfield(L, -2, method->name);
            }
            lua_rawseti(L, byKind, static_cast<int>(kind));
        }

        luaL_newmetatable(L, kObjectMeta);
        lua_pushvalue(L, byKind);
        lua_pushcclosure(L, &Index, 1);
        lua_setfield(L, -2, "__index");
        lua_pushvalue(L, reserved);
        lua_pushcclosure(L, &NewIndex, 1);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, &ToString);
        lua_setfield(L, -2, "__tostring");
        lua_pushliteral(L, "wxl.object");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 3);

        // The cache, weak in its values; the data; the environment of an object with none.
        lua_pushlightuserdata(L, &kCacheKey);
        lua_newtable(L);
        lua_createtable(L, 0, 1);
        lua_pushliteral(L, "v");
        lua_setfield(L, -2, "__mode");
        lua_setmetatable(L, -2);
        lua_rawset(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, &kDataKey);
        lua_newtable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, &kEmptyKey);
        lua_newtable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);
    }

    void PushObject(lua_State* L, wxl_handle handle)
    {
        if (handle == WXL_HANDLE_NONE)
        {
            lua_pushnil(L);
            return;
        }

        PushRegistry(L, &kCacheKey);
        PushKey(L, handle);
        lua_rawget(L, -2);
        if (lua_isuserdata(L, -1))
        {
            lua_remove(L, -2);
            return;
        }
        lua_pop(L, 1);

        const wxl_object_api* api  = Api();
        ObjectBox*            box  = static_cast<ObjectBox*>(lua_newuserdata(L, sizeof(ObjectBox)));
        box->handle                = handle;
        box->kind                  = api ? api->type(handle) : static_cast<uint32_t>(WXL_OBJECT_UNKNOWN);
        if (box->kind > WXL_OBJECT_CORPSE) box->kind = WXL_OBJECT_UNKNOWN;
        // A handle the service no longer resolves is gone from its first push.
        box->dead = api && box->kind == WXL_OBJECT_UNKNOWN;
        luaL_getmetatable(L, kObjectMeta);
        lua_setmetatable(L, -2);

        // What the mod stored on this handle before the last userdata for it was collected.
        PushRegistry(L, &kDataKey);
        PushKey(L, handle);
        lua_rawget(L, -2);
        lua_remove(L, -2);
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            PushRegistry(L, &kEmptyKey);
        }
        lua_setfenv(L, -2);

        PushKey(L, handle);
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
        lua_remove(L, -2);
    }

    void ForgetObject(lua_State* L, wxl_handle handle)
    {
        if (handle == WXL_HANDLE_NONE) return;

        PushRegistry(L, &kCacheKey);
        PushKey(L, handle);
        lua_rawget(L, -2);
        if (lua_isuserdata(L, -1))
        {
            ObjectBox* box = static_cast<ObjectBox*>(lua_touserdata(L, -1));
            box->dead      = true;
            PushRegistry(L, &kEmptyKey);
            lua_setfenv(L, -2);
        }
        lua_pop(L, 1);
        PushKey(L, handle);
        lua_pushnil(L);
        lua_rawset(L, -3);
        lua_pop(L, 1);

        PushRegistry(L, &kDataKey);
        PushKey(L, handle);
        lua_pushnil(L);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }

    wxl_handle CheckObject(lua_State* L, int index) { return CheckBox(L, index)->handle; }

    void PushObjectModule(lua_State* L)
    {
        lua_createtable(L, 0, 3);
        lua_pushcfunction(L, &LocalPlayer);
        lua_setfield(L, -2, "LocalPlayer");
        lua_pushcfunction(L, &FromGuid);
        lua_setfield(L, -2, "FromGuid");
        lua_pushcfunction(L, &InView);
        lua_setfield(L, -2, "InView");
    }

    void PushGuid(lua_State* L, wxl_guid guid)
    {
        char text[24];
        std::snprintf(text, sizeof text, "0x%016llX", static_cast<unsigned long long>(guid));
        lua_pushstring(L, text);
    }

    bool ToGuid(lua_State* L, int index, wxl_guid& out)
    {
        if (lua_type(L, index) == LUA_TNUMBER)
        {
            // A whole number a double holds exactly: past 2^53 the string form is the only one.
            const lua_Number value = lua_tonumber(L, index);
            if (!(value >= 0) || value > 9007199254740992.0 || value != static_cast<lua_Number>(
                                                                       static_cast<uint64_t>(value)))
                return false;
            out = static_cast<wxl_guid>(value);
            return true;
        }
        if (lua_type(L, index) != LUA_TSTRING) return false;

        // "0x" and one to sixteen hex digits, the form PushGuid writes; nothing else.
        size_t      length = 0;
        const char* text   = lua_tolstring(L, index, &length);
        if (length < 3 || length > 18 || text[0] != '0' || (text[1] != 'x' && text[1] != 'X')) return false;
        uint64_t value = 0;
        for (size_t i = 2; i < length; ++i)
        {
            const char c = text[i];
            uint64_t   digit;
            if (c >= '0' && c <= '9') digit = static_cast<uint64_t>(c - '0');
            else if (c >= 'a' && c <= 'f') digit = static_cast<uint64_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') digit = static_cast<uint64_t>(c - 'A' + 10);
            else return false;
            value = (value << 4) | digit;
        }
        out = static_cast<wxl_guid>(value);
        return true;
    }
} // namespace wxl::lua
