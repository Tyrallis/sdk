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
#include "wxl/reflect.hpp"

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
    /// Not a kind: the row answers for the active player only (the kinds column's Active).
    constexpr uint32_t kActiveOnly  = 0x100u;
    constexpr uint32_t kActive      = kPlayers | kActiveOnly;

    /// The object kinds of a kinds mask, without kActiveOnly.
    constexpr uint32_t KindsOf(uint32_t kinds) { return kinds & 0xFFu; }

    /// The type names Lua sees, by wxl_object_type: enums.def's, as an event's type arrives.
    inline const char* TypeName(uint32_t type)
    {
        static const reflect::Enum* const types = reflect::FindEnum("wxl_object_type");
        const char* name = types ? reflect::NameOf(*types, type) : nullptr;
        return name ? name : "Unknown";
    }

    /// The prefix a kind's members carry, dropped from the Lua method name.
    inline const char* PrefixOf(uint32_t kinds)
    {
        switch (kinds)
        {
        case kUnits: return "unit_";
        case kPlayers:
        case kActive: return "player_";
        case kItems: return "item_";
        case kContainers: return "container_";
        case kGameObjects: return "gameobject_";
        case kDynamics: return "dynamic_";
        case kCorpses: return "corpse_";
        default: return "";
        }
    }

    /// What a name keeps of its kind when it has to say it: GameObjectType, PlayerFlags.
    inline const char* QualifierOf(uint32_t kinds)
    {
        switch (kinds)
        {
        case kUnits: return "Unit";
        case kPlayers:
        case kActive: return "Player";
        case kItems: return "Item";
        case kContainers: return "Container";
        case kGameObjects: return "GameObject";
        case kDynamics: return "Dynamic";
        case kCorpses: return "Corpse";
        default: return "";
        }
    }

    /// The methods every object has, written by hand rather than generated from a row.
    constexpr const char* kHandWritten[] = {"Valid",  "Guid",   "Type",   "TypeId",   "Name",
                                            "Target", "Handle", "Field",  "Fields",   "SetField",
                                            "FieldCount",
                                            // lists and actions, on the kinds they apply to
                                            "Auras",  "Skills", "PartyMembers", "RaidMembers",
                                            "Select", "Interact", "CastAt", "Use", "Follow", "Assist"};

    inline bool IsHandWritten(const std::string& name)
    {
        for (const char* own : kHandWritten)
            if (name == own) return true;
        return false;
    }

    inline uint32_t Bits(uint32_t mask)
    {
        mask           = KindsOf(mask);
        uint32_t count = 0;
        for (; mask; mask >>= 1) count += mask & 1u;
        return count;
    }

    /// unit_max_health -> MaxHealth: the kind's prefix dropped, the rest PascalCased. Qualified,
    /// the prefix is said instead: gameobject_type -> GameObjectType.
    inline std::string LuaName(const char* member, uint32_t kinds, bool qualified = false)
    {
        const char*  prefix = PrefixOf(kinds);
        const size_t length = std::strlen(prefix);
        const bool   has    = length && std::strncmp(member, prefix, length) == 0;
        std::string  name   = qualified && has ? QualifierOf(kinds) : "";
        bool         upper  = true;
        for (const char* c = member + (has ? length : 0); *c; ++c)
        {
            if (*c == '_')
            {
                upper = true;
                continue;
            }
            name += upper && *c >= 'a' && *c <= 'z' ? static_cast<char>(*c - 'a' + 'A') : *c;
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
     * A row is named without its kind's prefix: unit_max_health is MaxHealth. Two rows may share
     * that name (unit_flags and gameobject_flags are both Flags) when no object is of both kinds:
     * the object's kind picks the row. A name says its kind instead -- GameObjectType,
     * PlayerFlags -- when it would otherwise be ambiguous:
     *
     *   - it is the name of a hand-written method (gameobject_type is not Type, the kind's name);
     *   - another row of an overlapping kind has it (unit_flags and player_flags both apply to a
     *     player): the narrower row says its kind, both do when neither is narrower.
     *
     * Nothing is ever shadowed in silence: Clash() names what this leaves ambiguous, and the
     * tests require it to name nothing.
     */
    inline std::vector<std::string> LuaNames(const std::vector<Member>& rows)
    {
        std::vector<std::string> names;
        std::vector<bool>        qualify(rows.size(), false);
        for (const Member& row : rows) names.push_back(LuaName(row.member, row.kinds));
        for (size_t i = 0; i < rows.size(); ++i)
        {
            if (IsHandWritten(names[i])) qualify[i] = true;
            for (size_t j = 0; j < rows.size(); ++j)
            {
                if (i == j || names[i] != names[j] || !(KindsOf(rows[i].kinds) & KindsOf(rows[j].kinds))) continue;
                if (Bits(rows[i].kinds) <= Bits(rows[j].kinds)) qualify[i] = true;
            }
        }
        for (size_t i = 0; i < rows.size(); ++i)
            if (qualify[i]) names[i] = LuaName(rows[i].member, rows[i].kinds, true);
        return names;
    }

    /// What a set of names leaves ambiguous, as text; empty when nothing is.
    inline std::string Clash(const std::vector<Member>& rows, const std::vector<std::string>& names)
    {
        std::string text;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            if (IsHandWritten(names[i]))
                text += std::string(rows[i].member) + " is named " + names[i] + ", a hand-written method; ";
            for (size_t j = i + 1; j < rows.size(); ++j)
                if (names[i] == names[j] && (KindsOf(rows[i].kinds) & KindsOf(rows[j].kinds)))
                    text += std::string(rows[i].member) + " and " + rows[j].member + " are both " +
                            names[i] + " on one object; ";
        }
        return text;
    }

    /// "Unit, Player": the types a kinds mask accepts.
    inline std::string KindsText(uint32_t kinds)
    {
        if (kinds == kAny) return "any object";
        if (kinds == kActive) return "the active player";
        std::string text;
        for (uint32_t type = WXL_OBJECT_ITEM; type <= WXL_OBJECT_CORPSE; ++type)
        {
            if (!(kinds & KindBit(static_cast<wxl_object_type>(type)))) continue;
            if (!text.empty()) text += ", ";
            text += TypeName(type);
        }
        return text;
    }
    // --- the rows, as text ------------------------------------------------------------------

    /// One row of object_reads.def, every column as written, and the version it came with.
    struct RowInfo
    {
        const char* kinds_name; ///< "Units", "Active"
        uint32_t    kinds;      ///< as a mask of the k* constants
        const char* member;
        const char* type;       ///< the C type the getter writes
        const char* source;     ///< field, client, cache or shared
        const char* cxx_name;   ///< the C++ facade's method
        const char* cxx;        ///< what it answers in C++
        const char* arg_c;      ///< the C type of its argument; empty for none
        const char* arg;        ///< the C++ type of its argument
        const char* doc;
        uint32_t    since[3];   ///< major, minor, feature
    };

    /// Every row, in table order.
    inline const std::vector<RowInfo>& Rows()
    {
        static const std::vector<RowInfo> rows = [] {
            // A SINCE row is an entry with no member, which dates the rows after it.
            const RowInfo table[] = {
#define WXL_OBJECT_SINCE(major, minor, feature) \
    RowInfo{"", 0, nullptr, "", "", "", "", "", "", "", {major, minor, feature}},
#define WXL_OBJECT_READ(kinds, member, Method, type, source, Cxx, cxx, doc) \
    RowInfo{#kinds, k##kinds, #member, #type, #source, #Cxx, #cxx, "", "", doc, {0, 1, 5}},
#define WXL_OBJECT_READ_AT(kinds, member, Method, type, source, Cxx, cxx, Arg, arg, doc) \
    RowInfo{#kinds, k##kinds, #member, #type, #source, #Cxx, #cxx, #Arg, #arg, doc, {0, 1, 5}},
#include "wxl/runtime/object_reads.def"
#undef WXL_OBJECT_READ_AT
#undef WXL_OBJECT_READ
#undef WXL_OBJECT_SINCE
            };
            std::vector<RowInfo> out;
            uint32_t             since[3] = {0, 1, 5};
            for (const RowInfo& row : table)
            {
                if (!row.member)
                {
                    for (int i = 0; i < 3; ++i) since[i] = row.since[i];
                    continue;
                }
                RowInfo dated = row;
                for (int i = 0; i < 3; ++i) dated.since[i] = since[i];
                out.push_back(dated);
            }
            return out;
        }();
        return rows;
    }

    /// The Lua method name of every row of Rows(), in order.
    inline const std::vector<std::string>& RowNames()
    {
        static const std::vector<std::string> names = [] {
            std::vector<Member> members;
            for (const RowInfo& row : Rows()) members.push_back(Member{row.member, row.kinds});
            return LuaNames(members);
        }();
        return names;
    }
} // namespace wxl::lua::names

#endif // WXL_LUA_OBJECT_NAMES_HPP
