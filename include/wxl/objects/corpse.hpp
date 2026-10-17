// wxl::Corpse: what a player leaves behind.
// Copyright (C) 2026 WarcraftXL
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef WXL_OBJECTS_CORPSE_HPP
#define WXL_OBJECTS_CORPSE_HPP

#include "wxl/objects/object.hpp"

namespace wxl
{
    /// A player's corpse: the client's ID_CORPSE. A dead creature is still a Unit.
    class Corpse : public Object
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_CORPSE};
        static constexpr bool Accepts(wxl_object_type type) { return type == WXL_OBJECT_CORPSE; }

        using Object::Object;

        /// The player it belongs to.
        Expected<wxl_guid> OwnerGuid() const { return Read(&wxl_object_api::corpse_owner); }
        /// The party the owner was in when they died, or WXL_GUID_NONE.
        Expected<wxl_guid> PartyGuid() const { return Read(&wxl_object_api::corpse_party); }
        Expected<uint32_t> DisplayId() const { return Read(&wxl_object_api::corpse_display_id); }
        Expected<uint32_t> Guild() const { return Read(&wxl_object_api::corpse_guild); }
        Expected<uint32_t> Flags() const { return Read(&wxl_object_api::corpse_flags); }
        Expected<uint32_t> DynamicFlags() const { return Read(&wxl_object_api::corpse_dynamic_flags); }
    };
} // namespace wxl

namespace std
{
    template <>
    struct hash<wxl::Corpse> : hash<wxl::Object>
    {
    };
} // namespace std

#endif // WXL_OBJECTS_CORPSE_HPP
