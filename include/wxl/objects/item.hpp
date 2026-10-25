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

#include "wxl/objects/generated/item.inc"

        /// Uses it, on a unit, or on what the item picks itself for a null one (wxl.action).
        wxl_result Use(Object target = Object()) const
        {
            const wxl_action_api* api = detail::ActionTable::Raw();
            if (!detail::Carries(api, &wxl_action_api::use_item)) return WXL_E_UNSUPPORTED;
            if (!HasHandle()) return WXL_E_NOT_FOUND;
            return api->use_item(handle_, target.Handle());
        }
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
