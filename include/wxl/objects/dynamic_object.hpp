// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 WarcraftXL
//
// wxl::DynamicObject: a spell's area effect, placed in the world for a while.

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
