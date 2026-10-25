// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::Item: something that sits in a bag, a bank or an equipment slot.

#ifndef WXL_OBJECTS_ITEM_HPP
#define WXL_OBJECTS_ITEM_HPP

#include "wxl/objects/object.hpp"

namespace wxl
{
    /**
     * @brief An item: the client's ID_ITEM, and the base of a Container.
     *
     * Items have no place in the world: Position() and Facing() answer WXL_E_INVALID. Entry() is
     * the item id a tooltip shows.
     */
    class Item : public Object
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_ITEM, WXL_OBJECT_CONTAINER};
        static constexpr bool            Accepts(uint32_t type)
        {
            return type == WXL_OBJECT_ITEM || type == WXL_OBJECT_CONTAINER;
        }

        using Object::Object;

        /// The same object as this class, unchecked: for a handler whose event already gave the
        /// kind (`Accepts(type)`). As<T>() is the checked way.
        explicit constexpr Item(const Object& object) : Object(object.Handle()) {}

        /// The player who owns it; resolve it with Object::Resolve when it is in view.
        Expected<wxl_guid> OwnerGuid() const { return Read(&wxl_object_api::item_owner); }

        /// The bag holding it, or WXL_GUID_NONE in the backpack or when equipped.
        Expected<wxl_guid> ContainedInGuid() const { return Read(&wxl_object_api::item_contained_in); }

        /// Who crafted it, or WXL_GUID_NONE.
        Expected<wxl_guid> CreatorGuid() const { return Read(&wxl_object_api::item_creator); }

        Expected<int32_t> StackCount() const { return Read(&wxl_object_api::item_stack_count); }

        /// Seconds before it expires; 0 when it does not.
        Expected<uint32_t> Duration() const { return Read(&wxl_object_api::item_duration); }

        Expected<uint32_t> Flags() const { return Read(&wxl_object_api::item_flags); }
        Expected<uint32_t> Durability() const { return Read(&wxl_object_api::item_durability); }
        Expected<uint32_t> MaxDurability() const { return Read(&wxl_object_api::item_max_durability); }
    };
} // namespace wxl

namespace std
{
    template <>
    struct hash<wxl::Item> : hash<wxl::Object>
    {
    };
} // namespace std

#endif // WXL_OBJECTS_ITEM_HPP
