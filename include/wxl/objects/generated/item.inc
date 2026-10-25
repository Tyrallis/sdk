// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The members of wxl::Item that read the object, one per row of object_reads.def: written by
// wxl-object-members (sdk/tools/objects), not by hand. Change the table and run the tool.
//
// Included inside the class, in its public part.

        /// The player who owns it.
        Expected<wxl_guid> OwnerGuid() const { return ReadAs<wxl_guid>(&wxl_object_api::item_owner); }

        /// The bag holding it, or 0 in the backpack or when equipped.
        Expected<wxl_guid> ContainedInGuid() const { return ReadAs<wxl_guid>(&wxl_object_api::item_contained_in); }

        /// Who crafted it, or 0.
        Expected<wxl_guid> CreatorGuid() const { return ReadAs<wxl_guid>(&wxl_object_api::item_creator); }

        /// How many the stack holds.
        Expected<int32_t> StackCount() const { return ReadAs<int32_t>(&wxl_object_api::item_stack_count); }

        /// Seconds left before it expires; 0 when it does not.
        Expected<uint32_t> Duration() const { return ReadAs<uint32_t>(&wxl_object_api::item_duration); }

        /// ITEM_FIELD_FLAGS as the server sends it: soulbound 0x1, ...
        Expected<uint32_t> Flags() const { return ReadAs<uint32_t>(&wxl_object_api::item_flags); }

        /// Its durability now.
        Expected<uint32_t> Durability() const { return ReadAs<uint32_t>(&wxl_object_api::item_durability); }

        /// Its durability when repaired; 0 for an item that does not wear.
        Expected<uint32_t> MaxDurability() const { return ReadAs<uint32_t>(&wxl_object_api::item_max_durability); }

        /// The enchantment in a slot, 0 to 11 (0 permanent, 1 temporary, 2 to 4 sockets, ...), a SpellItemEnchantment.dbc id; 0 for none.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> Enchant(uint32_t slot) const
        {
            return ReadAtAs<uint32_t>(&wxl_object_api::item_enchant, static_cast<uint32_t>(slot));
        }

        /// Its quality: poor, common, uncommon, rare, epic, legendary, artifact, heirloom.
        /// WXL_E_FAILED until the client has the item's record from the server.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<wxl_item_quality> Quality() const { return ReadAs<wxl_item_quality>(&wxl_object_api::item_quality); }

        /// Its item level.
        /// WXL_E_FAILED until the client has the item's record from the server.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> Level() const { return ReadAs<uint32_t>(&wxl_object_api::item_level); }

        /// The level it takes to use.
        /// WXL_E_FAILED until the client has the item's record from the server.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> RequiredLevel() const { return ReadAs<uint32_t>(&wxl_object_api::item_required_level); }

        /// Its class, an ItemClass.dbc id: weapon 2, armor 4, ...
        /// WXL_E_FAILED until the client has the item's record from the server.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> Class() const { return ReadAs<uint32_t>(&wxl_object_api::item_class); }

        /// Its subclass within the class: a sword, a cloth robe, ...
        /// WXL_E_FAILED until the client has the item's record from the server.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> Subclass() const { return ReadAs<uint32_t>(&wxl_object_api::item_subclass); }

        /// Where it is worn: head 1, neck 2, ...; 0 for an item that is not worn.
        /// WXL_E_FAILED until the client has the item's record from the server.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> InventoryType() const { return ReadAs<uint32_t>(&wxl_object_api::item_inventory_type); }

        /// How many a stack of it holds at most.
        /// WXL_E_FAILED until the client has the item's record from the server.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> MaxStack() const { return ReadAs<uint32_t>(&wxl_object_api::item_max_stack); }

        /// What a vendor pays for one, in copper.
        /// WXL_E_FAILED until the client has the item's record from the server.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<uint32_t> SellPrice() const { return ReadAs<uint32_t>(&wxl_object_api::item_sell_price); }
