// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::GameObject: a placed, interactable thing.

#ifndef WXL_OBJECTS_GAME_OBJECT_HPP
#define WXL_OBJECTS_GAME_OBJECT_HPP

#include "wxl/objects/object.hpp"

namespace wxl
{
    /// A placed, interactable thing: a door, a chest, a node, a transport. The client's ID_GAMEOBJECT.
    class GameObject : public Object
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_GAMEOBJECT};
        static constexpr bool Accepts(uint32_t type) { return type == WXL_OBJECT_GAMEOBJECT; }

        using Object::Object;

        /// The same object as this class, unchecked: for a handler whose event already gave the
        /// kind (`Accepts(type)`). As<T>() is the checked way.
        explicit constexpr GameObject(const Object& object) : Object(object.Handle()) {}

#include "wxl/objects/generated/game_object.inc"
    };
} // namespace wxl

namespace std
{
    template <>
    struct hash<wxl::GameObject> : hash<wxl::Object>
    {
    };
} // namespace std

#endif // WXL_OBJECTS_GAME_OBJECT_HPP
