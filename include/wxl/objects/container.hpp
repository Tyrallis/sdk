// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
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
        static constexpr bool Accepts(uint32_t type) { return type == WXL_OBJECT_CONTAINER; }

        using Item::Item;

#include "wxl/objects/generated/container.inc"
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
