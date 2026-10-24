// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::Player: a player-controlled unit.

#ifndef WXL_OBJECTS_PLAYER_HPP
#define WXL_OBJECTS_PLAYER_HPP

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
        static constexpr bool Accepts(wxl_object_type type) { return type == WXL_OBJECT_PLAYER; }

        using Unit::Unit;

        /// The unit this client controls; null while out of the world or without the service.
        static Player Local()
        {
            const wxl_object_api* api = ObjectService::Raw();
            return Player(api ? api->local_player() : WXL_HANDLE_NONE);
        }

        Expected<uint32_t> Flags() const { return Read(&wxl_object_api::player_flags); }
        /// 0 while unguilded.
        Expected<uint32_t> GuildId() const { return Read(&wxl_object_api::player_guild_id); }
        Expected<uint32_t> GuildRank() const { return Read(&wxl_object_api::player_guild_rank); }
        Expected<uint32_t> ChosenTitle() const { return Read(&wxl_object_api::player_chosen_title); }

        /// The item id shown in an equipment slot; 0 when the slot is empty.
        Expected<uint32_t> VisibleItem(EquipSlot slot) const
        {
            return ReadAt(&wxl_object_api::player_visible_item, static_cast<uint32_t>(slot));
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
