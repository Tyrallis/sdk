// The leaf reads of an item: what a runtime supplies for wxl.object's item getters.
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

#ifndef WXL_RUNTIME_OBJECTS_ITEM_HPP
#define WXL_RUNTIME_OBJECTS_ITEM_HPP

#include <cstdint>

#include "wxl/abi.h"

namespace wxl::runtime
{
    /// The reads an item or a container answers.
    class ItemReads
    {
    public:
        virtual ~ItemReads() = default;

        virtual bool ItemOwner(void* object, wxl_guid& out) const       = 0;
        virtual bool ItemContainedIn(void* object, wxl_guid& out) const = 0;
        virtual bool ItemCreator(void* object, wxl_guid& out) const     = 0;

        virtual bool ItemStackCount(void* object, uint32_t& out) const    = 0;
        virtual bool ItemDuration(void* object, uint32_t& out) const      = 0;
        virtual bool ItemFlags(void* object, uint32_t& out) const         = 0;
        virtual bool ItemDurability(void* object, uint32_t& out) const    = 0;
        virtual bool ItemMaxDurability(void* object, uint32_t& out) const = 0;
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECTS_ITEM_HPP
