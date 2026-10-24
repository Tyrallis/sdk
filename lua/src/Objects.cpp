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

        struct ObjectBox
        {
            wxl_handle handle;
        };

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

        ObjectBox* CheckBox(lua_State* L, int index)
        {
            return static_cast<ObjectBox*>(luaL_checkudata(L, index, kObjectMeta));
        }

        /// A generated method: the rows sharing its name, the one accepting the object's kind.
        int Generated(lua_State* L)
        {
            const wxl_handle      handle = CheckBox(L, 1)->handle;
            const char*           name   = lua_tostring(L, lua_upvalueindex(1));
            const wxl_object_api* api    = Api();
            if (!api) return PushFailure(L, WXL_E_UNSUPPORTED);

            const uint32_t type = api->type(handle);
            if (type == WXL_OBJECT_UNKNOWN) return PushFailure(L, WXL_E_NOT_FOUND);
            for (const Row& row : Rows())
                if (row.name == name && (row.kinds & KindBit(static_cast<wxl_object_type>(type))))
                    return row.call(L, api, handle);

            lua_pushnil(L);
            lua_pushfstring(L, "a %s has no %s", TypeName(type), name);
            return 2;
        }

        // --- the hand-written methods -------------------------------------------------------

        int Valid(lua_State* L)
        {
            const wxl_handle      handle = CheckBox(L, 1)->handle;
            const wxl_object_api* api    = Api();
            lua_pushboolean(L, api && api->valid(handle) == WXL_TRUE);
            return 1;
        }

        int Guid(lua_State* L)
        {
            const wxl_handle      handle = CheckBox(L, 1)->handle;
            const wxl_object_api* api    = Api();
            if (!api) return PushFailure(L, WXL_E_UNSUPPORTED);
            PushGuid(L, api->guid(handle));
            return 1;
        }

        int Type(lua_State* L)
        {
            const wxl_handle      handle = CheckBox(L, 1)->handle;
            const wxl_object_api* api    = Api();
            if (!api) return PushFailure(L, WXL_E_UNSUPPORTED);
            lua_pushstring(L, TypeName(api->type(handle)));
            return 1;
        }

        int TypeId(lua_State* L)
        {
            const wxl_handle      handle = CheckBox(L, 1)->handle;
            const wxl_object_api* api    = Api();
            if (!api) return PushFailure(L, WXL_E_UNSUPPORTED);
            lua_pushnumber(L, api->type(handle));
            return 1;
        }

        int Name(lua_State* L)
        {
            const wxl_handle      handle = CheckBox(L, 1)->handle;
            const wxl_object_api* api    = Api();
            if (!api) return PushFailure(L, WXL_E_UNSUPPORTED);
            wxl_str          name{nullptr, 0};
            const wxl_result result = api->name(handle, &name);
            if (result != WXL_OK) return PushFailure(L, result);
            lua_pushlstring(L, name.data ? name.data : "", name.data ? name.size : 0);
            return 1;
        }

        int Target(lua_State* L)
        {
            const wxl_handle      handle = CheckBox(L, 1)->handle;
            const wxl_object_api* api    = Api();
            if (!api) return PushFailure(L, WXL_E_UNSUPPORTED);
            wxl_handle       target = WXL_HANDLE_NONE;
            const wxl_result result = api->unit_target(handle, &target);
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
            const wxl_handle      handle = CheckBox(L, 1)->handle;
            const wxl_object_api* api    = Api();
            const char* type = api ? TypeName(api->type(handle)) : "Object";
            char        text[48];
            std::snprintf(text, sizeof text, "%s(0x%016llX)", type,
                          static_cast<unsigned long long>(handle));
            lua_pushstring(L, text);
            return 1;
        }

        int Equal(lua_State* L)
        {
            lua_pushboolean(L, CheckBox(L, 1)->handle == CheckBox(L, 2)->handle);
            return 1;
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
                return luaL_error(L, "FromGuid takes a guid: a hex string like \"0x0F130000000012AB\" "
                                     "or a number");
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
        luaL_newmetatable(L, kObjectMeta);

        lua_newtable(L); // the methods
        for (const Row& row : Rows())
        {
            lua_pushstring(L, row.name.c_str());
            lua_pushcclosure(L, &Generated, 1);
            lua_setfield(L, -2, row.name.c_str());
        }
        const luaL_Reg methods[] = {
            {"Valid", &Valid},   {"Guid", &Guid},     {"Type", &Type},     {"TypeId", &TypeId},
            {"Name", &Name},     {"Target", &Target}, {"Handle", &Handle}, {nullptr, nullptr},
        };
        for (const luaL_Reg* method = methods; method->name; ++method)
        {
            lua_pushcfunction(L, method->func);
            lua_setfield(L, -2, method->name);
        }
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, &ToString);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, &Equal);
        lua_setfield(L, -2, "__eq");
        lua_pushliteral(L, "wxl.object");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 1);
    }

    void PushObject(lua_State* L, wxl_handle handle)
    {
        if (handle == WXL_HANDLE_NONE)
        {
            lua_pushnil(L);
            return;
        }
        ObjectBox* box = static_cast<ObjectBox*>(lua_newuserdata(L, sizeof(ObjectBox)));
        box->handle    = handle;
        luaL_getmetatable(L, kObjectMeta);
        lua_setmetatable(L, -2);
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
            const lua_Number value = lua_tonumber(L, index);
            if (value < 0) return false;
            out = static_cast<wxl_guid>(value);
            return true;
        }
        if (lua_type(L, index) != LUA_TSTRING) return false;
        const char* text = lua_tostring(L, index);
        char*       end  = nullptr;
        const unsigned long long value = std::strtoull(text, &end, 0);
        if (end == text || *end != '\0') return false;
        out = static_cast<wxl_guid>(value);
        return true;
    }
} // namespace wxl::lua
