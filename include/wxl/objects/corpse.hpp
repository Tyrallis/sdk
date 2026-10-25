// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::Corpse: what a player leaves behind.

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
        static constexpr bool Accepts(uint32_t type) { return type == WXL_OBJECT_CORPSE; }

        using Object::Object;

        /// The same object as this class, unchecked: for a handler whose event already gave the
        /// kind (`Accepts(type)`). As<T>() is the checked way.
        explicit constexpr Corpse(const Object& object) : Object(object.Handle()) {}

#include "wxl/objects/generated/corpse.inc"
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
