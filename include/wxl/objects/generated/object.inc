// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The members of wxl::Object that read the object, one per row of object_reads.def: written by
// wxl-object-members (sdk/tools/objects), not by hand. Change the table and run the tool.
//
// Included inside the class, in its public part.

        /// Where it stands in the world, in yards; an item has no place.
        Expected<wxl_vec3> Position() const { return ReadAs<wxl_vec3>(&wxl_object_api::position); }

        /// The way it faces, in radians.
        Expected<float> Facing() const { return ReadAs<float>(&wxl_object_api::facing); }

        /// The server's template id: a creature, item or game-object entry; 0 for a player.
        Expected<uint32_t> Entry() const { return ReadAs<uint32_t>(&wxl_object_api::entry); }

        /// Its size, 1 being the model's own.
        Expected<float> Scale() const { return ReadAs<float>(&wxl_object_api::scale); }

        /// Whether it is the unit the client controls.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<bool> IsActivePlayer() const { return ReadAs<bool>(&wxl_object_api::is_active_player); }

        /// Yards from it to another object, centre to centre; invalid when either has no place in the world.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<float> Distance(const Object& other) const
        {
            return ReadAtAs<float>(&wxl_object_api::distance, other.Handle());
        }

        /// Yards from it to a point; invalid for an object with no place in the world.
        /// Came with abi 0.1.18: WXL_E_UNSUPPORTED on an older runtime.
        Expected<float> DistanceTo(wxl_vec3 point) const
        {
            return ReadAtAs<float>(&wxl_object_api::distance_to, point);
        }
