// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// World objects in Lua: a userdata over a wxl_handle whose methods are the getters of
// wxl.object, generated from object_reads.def. One userdata per handle and state while the mod
// holds it, with the fields the mod stored on it, until the object leaves.

#ifndef WXL_LUA_OBJECTS_HPP
#define WXL_LUA_OBJECTS_HPP

#include <cstdint>
#include <vector>

#include "Lua.hpp"
#include "wxl/abi.h"
#include "wxl/events.h"

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

    /**
     * @brief The fields one Lua mod watches, through its wxl.object.Watch.
     *
     * The host knows wxl.lua, not the Lua mods in it: each watch_field call is wxl.lua's, and
     * this keeps which mod asked, so that a mod receives ObjectFieldChanged for its own fields
     * only and gives them back when it closes. One watch per field and type: asking again for
     * one the mod watches changes nothing.
     */
    class FieldWatches
    {
    public:
        FieldWatches() = default;
        ~FieldWatches() { Release(); }

        FieldWatches(const FieldWatches&)            = delete;
        FieldWatches& operator=(const FieldWatches&) = delete;

        /// Whether the mod watches the element an event carries, on that object's type.
        bool Covers(const wxl_object_field_args& args) const;

        /// Gives every watch back to the runtime: the mod closes, reloads or is disabled.
        void Release();

        /// One field the mod watches: the type it asked for, the kind and the field's dwords.
        struct Entry
        {
            uint32_t        type;
            wxl_object_type kind;
            uint32_t        index;
            uint32_t        size;
        };
        std::vector<Entry> entries;
    };

    /// Pushes the `wxl.object` table: LocalPlayer, FromGuid, InView, Fields, Watch, Unwatch; the
    /// last two keep what they do in @p watches.
    void PushObjectModule(lua_State* L, FieldWatches& watches);

    /// Pushes the `wxl.field` table: kind -> field name -> dword index, from update_fields.def.
    void PushFieldModule(lua_State* L);

    /// Pushes an element of an update field from its dwords, as Lua reads that field's value:
    /// a number, a guid's string, or a table of its bytes or halves. Pushes one value.
    void PushFieldElement(lua_State* L, uint32_t type, uint32_t index, const uint32_t* dwords);

    /// A guid as Lua carries it: a hex string, "0x" and sixteen digits, since a double cannot
    /// hold every 64-bit value.
    void PushGuid(lua_State* L, wxl_guid guid);

    /// Reads a guid from "0x" and one to sixteen hex digits, or from a whole number a double
    /// holds exactly. False for anything else.
    bool ToGuid(lua_State* L, int index, wxl_guid& out);
} // namespace wxl::lua

#endif // WXL_LUA_OBJECTS_HPP
