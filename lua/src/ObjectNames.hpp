// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// How a row of object_reads.def is named in Lua. Shared by the object methods and by the
// generator of the editor definitions, so that both say the same.

#ifndef WXL_LUA_OBJECT_NAMES_HPP
#define WXL_LUA_OBJECT_NAMES_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "wxl/events.h"

namespace wxl::lua::names
{
    /// One bit per wxl_object_type; the kinds column of object_reads.def names a mask.
    constexpr uint32_t KindBit(wxl_object_type type) { return 1u << static_cast<uint32_t>(type); }

    constexpr uint32_t kAny         = 0xFFu;
    constexpr uint32_t kUnits       = KindBit(WXL_OBJECT_UNIT) | KindBit(WXL_OBJECT_PLAYER);
    constexpr uint32_t kPlayers     = KindBit(WXL_OBJECT_PLAYER);
    constexpr uint32_t kItems       = KindBit(WXL_OBJECT_ITEM) | KindBit(WXL_OBJECT_CONTAINER);
    constexpr uint32_t kContainers  = KindBit(WXL_OBJECT_CONTAINER);
    constexpr uint32_t kGameObjects = KindBit(WXL_OBJECT_GAMEOBJECT);
    constexpr uint32_t kDynamics    = KindBit(WXL_OBJECT_DYNAMIC);
    constexpr uint32_t kCorpses     = KindBit(WXL_OBJECT_CORPSE);

    /// The type names Lua sees, by wxl_object_type.
    inline const char* TypeName(uint32_t type)
    {
        switch (type)
        {
        case WXL_OBJECT_ITEM: return "Item";
        case WXL_OBJECT_CONTAINER: return "Container";
        case WXL_OBJECT_UNIT: return "Unit";
        case WXL_OBJECT_PLAYER: return "Player";
        case WXL_OBJECT_GAMEOBJECT: return "GameObject";
        case WXL_OBJECT_DYNAMIC: return "DynamicObject";
        case WXL_OBJECT_CORPSE: return "Corpse";
        default: return "Unknown";
        }
    }

    /// The prefix a kind's members carry, dropped from the Lua method name.
    inline const char* PrefixOf(uint32_t kinds)
    {
        switch (kinds)
        {
        case kUnits: return "unit_";
        case kPlayers: return "player_";
        case kItems: return "item_";
        case kContainers: return "container_";
        case kGameObjects: return "gameobject_";
        case kDynamics: return "dynamic_";
        case kCorpses: return "corpse_";
        default: return "";
        }
    }

    inline uint32_t Bits(uint32_t mask)
    {
        uint32_t count = 0;
        for (; mask; mask >>= 1) count += mask & 1u;
        return count;
    }

    /// unit_max_health -> MaxHealth: the kind's prefix dropped, the rest PascalCased.
    inline std::string LuaName(const char* member, uint32_t kinds)
    {
        const char*  prefix = PrefixOf(kinds);
        const size_t skip = std::strncmp(member, prefix, std::strlen(prefix)) == 0 ? std::strlen(prefix) : 0;
        std::string  name;
        bool         upper = true;
        for (const char* c = member + skip; *c; ++c)
        {
            if (*c == '_')
            {
                upper = true;
                continue;
            }
            name += upper ? static_cast<char>(*c - 'a' + 'A') : *c;
            upper = false;
        }
        return name;
    }

    /// One row of object_reads.def, as the naming needs it.
    struct Member
    {
        const char* member;
        uint32_t    kinds;
    };

    /**
     * @brief The Lua method name of every row, in order.
     *
     * Two rows may share a name (unit_flags and gameobject_flags are both Flags) when no object
     * is of both kinds: the object's kind picks the row. When their kinds overlap -- unit_flags
     * and player_flags both apply to a player -- the row of the narrower kind keeps its prefix:
     * PlayerFlags.
     */
    inline std::vector<std::string> LuaNames(const std::vector<Member>& rows)
    {
        std::vector<std::string> names;
        for (const Member& row : rows) names.push_back(LuaName(row.member, row.kinds));
        for (size_t i = 0; i < rows.size(); ++i)
        {
            for (size_t j = 0; j < rows.size(); ++j)
            {
                if (i == j || names[i] != names[j] || !(rows[i].kinds & rows[j].kinds)) continue;
                if (Bits(rows[i].kinds) < Bits(rows[j].kinds)) names[i] = LuaName(rows[i].member, 0);
            }
        }
        return names;
    }

    /// "Unit, Player": the types a kinds mask accepts.
    inline std::string KindsText(uint32_t kinds)
    {
        if (kinds == kAny) return "any object";
        std::string text;
        for (uint32_t type = WXL_OBJECT_ITEM; type <= WXL_OBJECT_CORPSE; ++type)
        {
            if (!(kinds & KindBit(static_cast<wxl_object_type>(type)))) continue;
            if (!text.empty()) text += ", ";
            text += TypeName(type);
        }
        return text;
    }
} // namespace wxl::lua::names

#endif // WXL_LUA_OBJECT_NAMES_HPP
