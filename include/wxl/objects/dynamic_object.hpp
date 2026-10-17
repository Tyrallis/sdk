// wxl::DynamicObject: a spell's area effect, placed in the world for a while.
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

#ifndef WXL_OBJECTS_DYNAMIC_OBJECT_HPP
#define WXL_OBJECTS_DYNAMIC_OBJECT_HPP

#include "wxl/objects/object.hpp"

namespace wxl
{
    /// A spell's area effect -- a blizzard, a consecration: the client's ID_DYNAMICOBJECT.
    class DynamicObject : public Object
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_DYNAMIC};
        static constexpr bool Accepts(wxl_object_type type) { return type == WXL_OBJECT_DYNAMIC; }

        using Object::Object;

        /// The unit that cast it.
        Expected<wxl_guid> CasterGuid() const { return Read(&wxl_object_api::dynamic_caster); }
        Expected<uint32_t> SpellId() const { return Read(&wxl_object_api::dynamic_spell_id); }
        Expected<float>    Radius() const { return Read(&wxl_object_api::dynamic_radius); }
        /// The client time it was cast at, in milliseconds.
        Expected<uint32_t> CastTime() const { return Read(&wxl_object_api::dynamic_cast_time); }
    };
} // namespace wxl

namespace std
{
    template <>
    struct hash<wxl::DynamicObject> : hash<wxl::Object>
    {
    };
} // namespace std

#endif // WXL_OBJECTS_DYNAMIC_OBJECT_HPP
