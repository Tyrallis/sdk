// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 WarcraftXL
//
// World objects in Lua: a userdata over a wxl_handle whose methods are the getters of
// wxl.object, generated from object_reads.def.

#ifndef WXL_LUA_OBJECTS_HPP
#define WXL_LUA_OBJECTS_HPP

#include "Lua.hpp"
#include "wxl/abi.h"

namespace wxl::lua
{
    /// Registers the object metatable in a state.
    void OpenObjects(lua_State* L);

    /// Pushes an object userdata for a handle; nil for WXL_HANDLE_NONE.
    void PushObject(lua_State* L, wxl_handle handle);

    /// The handle of the object userdata at an index; raises a Lua error for anything else.
    wxl_handle CheckObject(lua_State* L, int index);

    /// Pushes the `wxl.object` table: LocalPlayer, FromGuid, InView.
    void PushObjectModule(lua_State* L);

    /// A guid as Lua carries it: a hex string, "0x" and sixteen digits, since a double cannot
    /// hold every 64-bit value.
    void PushGuid(lua_State* L, wxl_guid guid);

    /// Reads a guid from a hex string or a number. False for anything else.
    bool ToGuid(lua_State* L, int index, wxl_guid& out);
} // namespace wxl::lua

#endif // WXL_LUA_OBJECTS_HPP
