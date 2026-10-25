// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
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
        static constexpr bool Accepts(uint32_t type) { return type == WXL_OBJECT_DYNAMIC; }

        using Object::Object;

        /// The same object as this class, unchecked: for a handler whose event already gave the
        /// kind (`Accepts(type)`). As<T>() is the checked way.
        explicit constexpr DynamicObject(const Object& object) : Object(object.Handle()) {}

#include "wxl/objects/generated/dynamic_object.inc"
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
