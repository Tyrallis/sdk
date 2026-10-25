// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The members of wxl::Player that read the object, one per row of object_reads.def: written by
// wxl-object-members (sdk/tools/objects), not by hand. Change the table and run the tool.
//
// Included inside the class, in its public part.

        /// PLAYER_FLAGS as the server sends it.
        Expected<uint32_t> Flags() const { return ReadAs<uint32_t>(&wxl_object_api::player_flags); }

        /// Its guild's id; 0 while unguilded.
        Expected<uint32_t> GuildId() const { return ReadAs<uint32_t>(&wxl_object_api::player_guild_id); }

        /// Its rank in the guild, 0 for the guild master.
        Expected<uint32_t> GuildRank() const { return ReadAs<uint32_t>(&wxl_object_api::player_guild_rank); }

        /// The title it shows, a CharTitles.dbc bit index; 0 for none.
        Expected<uint32_t> ChosenTitle() const { return ReadAs<uint32_t>(&wxl_object_api::player_chosen_title); }

        /// The item id an equipment slot shows (0 head .. 18 tabard); 0 when the slot is empty.
        Expected<uint32_t> VisibleItem(EquipSlot slot) const
        {
            return ReadAtAs<uint32_t>(&wxl_object_api::player_visible_item, static_cast<uint32_t>(slot));
        }

        /// Whether it is a ghost, released from its corpse.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<bool> IsGhost() const { return ReadAs<bool>(&wxl_object_api::player_is_ghost); }

        /// Whether it is away from the keyboard.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<bool> IsAfk() const { return ReadAs<bool>(&wxl_object_api::player_is_afk); }

        /// Whether it asked not to be disturbed.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<bool> IsDnd() const { return ReadAs<bool>(&wxl_object_api::player_is_dnd); }

        /// Whether it rests in an inn or a city, gaining rested experience.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<bool> IsResting() const { return ReadAs<bool>(&wxl_object_api::player_is_resting); }

        /// Its money, in copper.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> Money() const { return ReadAs<uint32_t>(&wxl_object_api::player_money); }

        /// Its experience in this level.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> Xp() const { return ReadAs<uint32_t>(&wxl_object_api::player_xp); }

        /// The experience this level takes.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> NextLevelXp() const { return ReadAs<uint32_t>(&wxl_object_api::player_next_level_xp); }

        /// Its rested experience: kills give twice as much until it is spent.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> RestedXp() const { return ReadAs<uint32_t>(&wxl_object_api::player_rested_xp); }

        /// Its honor points.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> HonorPoints() const { return ReadAs<uint32_t>(&wxl_object_api::player_honor_points); }

        /// Its arena points.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> ArenaPoints() const { return ReadAs<uint32_t>(&wxl_object_api::player_arena_points); }

        /// The item in an equipment slot, or the bag in a bag slot (19 to 22); none for an empty slot.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<Item> InventoryItem(wxl_inventory_slot slot) const
        {
            return ReadAtAs<Item>(&wxl_object_api::player_inventory_item, static_cast<uint32_t>(slot));
        }

        /// The item in a slot of the backpack, 0 to 15; none for an empty slot.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<Item> BackpackItem(uint32_t slot) const
        {
            return ReadAtAs<Item>(&wxl_object_api::player_backpack_item, static_cast<uint32_t>(slot));
        }

        /// The skill on a line of the skill list, 0 to 127: its id, value and maximum; an id of 0 for an empty line.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<wxl_skill> Skill(uint32_t line) const
        {
            return ReadAtAs<wxl_skill>(&wxl_object_api::player_skill, static_cast<uint32_t>(line));
        }

        /// A skill by its SkillLine.dbc id; invalid for a skill the player does not have.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<wxl_skill> SkillOf(uint32_t skill_id) const
        {
            return ReadAtAs<wxl_skill>(&wxl_object_api::player_skill_of, static_cast<uint32_t>(skill_id));
        }

        /// How many others are in its party, 0 to 4.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> PartySize() const { return ReadAs<uint32_t>(&wxl_object_api::player_party_size); }

        /// The other party member in a party slot, 0 to 3, whether in view or not; 0 for an empty slot.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<wxl_guid> PartyMemberGuid(uint32_t slot) const
        {
            return ReadAtAs<wxl_guid>(&wxl_object_api::player_party_member_guid, static_cast<uint32_t>(slot));
        }

        /// How many are in its raid, itself included; 0 outside a raid.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> RaidSize() const { return ReadAs<uint32_t>(&wxl_object_api::player_raid_size); }

        /// The raid member in a raid slot, 0 to 39, whether in view or not; 0 for an empty slot.
        /// The active player only: WXL_E_INVALID for another player.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<wxl_guid> RaidMemberGuid(uint32_t slot) const
        {
            return ReadAtAs<wxl_guid>(&wxl_object_api::player_raid_member_guid, static_cast<uint32_t>(slot));
        }
