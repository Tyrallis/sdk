// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The members of wxl::Container that read the object, one per row of object_reads.def: written by
// wxl-object-members (sdk/tools/objects), not by hand. Change the table and run the tool.
//
// Included inside the class, in its public part.

        /// How many slots the bag has.
        Expected<uint32_t> NumSlots() const { return ReadAs<uint32_t>(&wxl_object_api::container_num_slots); }

        /// The item in a slot, as a guid; 0 for an empty one, invalid past NumSlots.
        Expected<wxl_guid> SlotGuid(uint32_t slot) const
        {
            return ReadAtAs<wxl_guid>(&wxl_object_api::container_slot, static_cast<uint32_t>(slot));
        }

        /// The item in a slot of the bag; none for an empty one, invalid past NumSlots.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<Item> SlotItem(uint32_t slot) const
        {
            return ReadAtAs<Item>(&wxl_object_api::container_slot_item, static_cast<uint32_t>(slot));
        }
