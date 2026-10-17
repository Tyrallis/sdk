// wxl::Container: a bag, which is an item that holds items.
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
