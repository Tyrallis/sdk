// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The members of wxl::Corpse that read the object, one per row of object_reads.def: written by
// wxl-object-members (sdk/tools/objects), not by hand. Change the table and run the tool.
//
// Included inside the class, in its public part.

        /// The player it belongs to.
        Expected<wxl_guid> OwnerGuid() const { return ReadAs<wxl_guid>(&wxl_object_api::corpse_owner); }

        /// The party the owner was in when they died, or 0.
        Expected<wxl_guid> PartyGuid() const { return ReadAs<wxl_guid>(&wxl_object_api::corpse_party); }

        /// Its model.
        Expected<uint32_t> DisplayId() const { return ReadAs<uint32_t>(&wxl_object_api::corpse_display_id); }

        /// The owner's guild id.
        Expected<uint32_t> Guild() const { return ReadAs<uint32_t>(&wxl_object_api::corpse_guild); }

        /// CORPSE_FIELD_FLAGS: bones 0x1, ...
        Expected<uint32_t> Flags() const { return ReadAs<uint32_t>(&wxl_object_api::corpse_flags); }

        /// CORPSE_FIELD_DYNAMIC_FLAGS: lootable 0x1.
        Expected<uint32_t> DynamicFlags() const { return ReadAs<uint32_t>(&wxl_object_api::corpse_dynamic_flags); }
