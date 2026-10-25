// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::Player: a player-controlled unit.

#ifndef WXL_OBJECTS_PLAYER_HPP
#define WXL_OBJECTS_PLAYER_HPP

#include "wxl/objects/container.hpp"
#include "wxl/objects/unit.hpp"

namespace wxl
{
    /// The equipment slots a player shows, as player_visible_item takes them.
    enum class EquipSlot : uint32_t
    {
        Head     = 0,
        Neck     = 1,
        Shoulder = 2,
        Shirt    = 3,
        Chest    = 4,
        Waist    = 5,
        Legs     = 6,
        Feet     = 7,
        Wrist    = 8,
        Hands    = 9,
        Finger1  = 10,
        Finger2  = 11,
        Trinket1 = 12,
        Trinket2 = 13,
        Back     = 14,
        MainHand = 15,
        OffHand  = 16,
        Ranged   = 17,
        Tabard   = 18,
        Count    = 19
    };

    /// A player-controlled unit: the client's ID_PLAYER. Everything a Unit has, plus the player's own fields.
    class Player : public Unit
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_PLAYER};
        static constexpr bool Accepts(uint32_t type) { return type == WXL_OBJECT_PLAYER; }

        using Unit::Unit;

        /// The unit this client controls; null while out of the world or without the service.
        static Player Local()
        {
            const wxl_object_api* api = ObjectService::Raw();
            return Player(api ? api->local_player() : WXL_HANDLE_NONE);
        }

#include "wxl/objects/generated/player.inc"

        /// The lines of its skill list that hold a skill, in list order. The active player only.
        Expected<std::vector<wxl_skill>> Skills() const
        {
            std::vector<wxl_skill> skills;
            for (uint32_t line = 0; line < 128; ++line)
            {
                Expected<wxl_skill> skill = Skill(line);
                if (!skill) return Unexpected{skill.Error()};
                if (skill->id) skills.push_back(*skill);
            }
            return skills;
        }

        /// The other members of its party, by guid, in party order. The active player only.
        Expected<std::vector<wxl_guid>> PartyMembers() const
        {
            std::vector<wxl_guid> members;
            for (uint32_t slot = 0; slot < 4; ++slot)
            {
                Expected<wxl_guid> guid = PartyMemberGuid(slot);
                if (!guid) return Unexpected{guid.Error()};
                if (*guid) members.push_back(*guid);
            }
            return members;
        }

        /// The members of its raid, itself included, by guid. The active player only.
        Expected<std::vector<wxl_guid>> RaidMembers() const
        {
            std::vector<wxl_guid> members;
            for (uint32_t slot = 0; slot < 40; ++slot)
            {
                Expected<wxl_guid> guid = RaidMemberGuid(slot);
                if (!guid) return Unexpected{guid.Error()};
                if (*guid) members.push_back(*guid);
            }
            return members;
        }
    };
} // namespace wxl

namespace std
{
    template <>
    struct hash<wxl::Player> : hash<wxl::Object>
    {
    };
} // namespace std

#endif // WXL_OBJECTS_PLAYER_HPP
