// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// World objects in Lua: a userdata over a wxl_handle whose methods are the getters of
// wxl.object, generated from object_reads.def. One userdata per handle and state while the mod
// holds it, with the fields the mod stored on it, until the object leaves.

#ifndef WXL_LUA_OBJECTS_HPP
#define WXL_LUA_OBJECTS_HPP

#include "Lua.hpp"
#include "wxl/abi.h"

namespace wxl::lua
{
    /// Registers the object metatable, the methods of each kind and the per-state tables.
    void OpenObjects(lua_State* L);

    /// Pushes the object userdata for a handle -- the same one at every push while the mod holds
    /// it -- or nil for WXL_HANDLE_NONE.
    void PushObject(lua_State* L, wxl_handle handle);

    /// The object left (ObjectDestroy): its userdata, if the state still holds it, answers "gone"
    /// from now on, and what the mod stored on it is dropped.
    void ForgetObject(lua_State* L, wxl_handle handle);

    /// The handle of the object userdata at an index; raises a Lua error for anything else.
    wxl_handle CheckObject(lua_State* L, int index);

    /// Pushes the `wxl.object` table: LocalPlayer, FromGuid, InView.
    void PushObjectModule(lua_State* L);

    /// A guid as Lua carries it: a hex string, "0x" and sixteen digits, since a double cannot
    /// hold every 64-bit value.
    void PushGuid(lua_State* L, wxl_guid guid);

    /// Reads a guid from "0x" and one to sixteen hex digits, or from a whole number a double
    /// holds exactly. False for anything else.
    bool ToGuid(lua_State* L, int index, wxl_guid& out);
} // namespace wxl::lua

#endif // WXL_LUA_OBJECTS_HPP
