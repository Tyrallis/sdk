// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 WarcraftXL
//
// wxl::Container: a bag, which is an item that holds items.

#ifndef WXL_OBJECTS_CONTAINER_HPP
#define WXL_OBJECTS_CONTAINER_HPP

#include "wxl/objects/item.hpp"

namespace wxl
{
    /// A bag: the client's ID_CONTAINER. Everything an Item has, plus its slots.
    class Container : public Item
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_CONTAINER};
        static constexpr bool Accepts(wxl_object_type type) { return type == WXL_OBJECT_CONTAINER; }

        using Item::Item;

        Expected<uint32_t> NumSlots() const { return Read(&wxl_object_api::container_num_slots); }

        /// The item in a slot as a guid, WXL_GUID_NONE for an empty one; invalid past NumSlots().
        Expected<wxl_guid> SlotGuid(uint32_t index) const
        {
            return ReadAt(&wxl_object_api::container_slot, index);
        }
    };
} // namespace wxl

namespace std
{
    template <>
    struct hash<wxl::Container> : hash<wxl::Object>
    {
    };
} // namespace std

#endif // WXL_OBJECTS_CONTAINER_HPP
