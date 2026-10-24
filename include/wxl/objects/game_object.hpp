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
        static constexpr bool Accepts(wxl_object_type type) { return type == WXL_OBJECT_GAMEOBJECT; }

        using Object::Object;

        /// Who summoned or spawned it, or WXL_GUID_NONE for a world-placed one.
        Expected<wxl_guid> CreatedByGuid() const { return Read(&wxl_object_api::gameobject_created_by); }

        Expected<uint32_t> DisplayId() const { return Read(&wxl_object_api::gameobject_display_id); }
        Expected<uint32_t> Flags() const { return Read(&wxl_object_api::gameobject_flags); }
        Expected<uint32_t> DynamicFlags() const { return Read(&wxl_object_api::gameobject_dynamic_flags); }
        Expected<uint32_t> Faction() const { return Read(&wxl_object_api::gameobject_faction); }
        Expected<uint32_t> Level() const { return Read(&wxl_object_api::gameobject_level); }

        /// 0 active, 1 ready, 2 destroyed.
        Expected<uint32_t> State() const { return Read(&wxl_object_api::gameobject_state); }

        /// The game-object type, in the server's GameObjectType numbering (door 0, button 1, ...).
        Expected<uint32_t> GoType() const { return Read(&wxl_object_api::gameobject_type); }

        Expected<uint32_t> ArtKit() const { return Read(&wxl_object_api::gameobject_art_kit); }
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
