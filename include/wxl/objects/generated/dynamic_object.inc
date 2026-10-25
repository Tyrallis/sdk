// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The members of wxl::DynamicObject that read the object, one per row of object_reads.def: written by
// wxl-object-members (sdk/tools/objects), not by hand. Change the table and run the tool.
//
// Included inside the class, in its public part.

        /// The unit that cast it.
        Expected<wxl_guid> CasterGuid() const { return ReadAs<wxl_guid>(&wxl_object_api::dynamic_caster); }

        /// The spell it is the area of.
        Expected<uint32_t> SpellId() const { return ReadAs<uint32_t>(&wxl_object_api::dynamic_spell_id); }

        /// Its radius, in yards.
        Expected<float> Radius() const { return ReadAs<float>(&wxl_object_api::dynamic_radius); }

        /// The client time it was cast at, in milliseconds.
        Expected<uint32_t> CastTime() const { return ReadAs<uint32_t>(&wxl_object_api::dynamic_cast_time); }
