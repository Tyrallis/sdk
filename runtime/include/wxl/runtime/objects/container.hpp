// The leaf reads of a container: what a runtime supplies for wxl.object's bag getters.
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

#ifndef WXL_RUNTIME_OBJECTS_CONTAINER_HPP
#define WXL_RUNTIME_OBJECTS_CONTAINER_HPP

#include <cstdint>

#include "wxl/abi.h"

namespace wxl::runtime
{
    /// The reads a container answers, on top of ItemReads.
    class ContainerReads
    {
    public:
        virtual ~ContainerReads() = default;

        virtual bool ContainerNumSlots(void* object, uint32_t& out) const = 0;

        /// The guid in a slot, WXL_GUID_NONE for an empty one; false past the container's slots.
        virtual bool ContainerSlot(void* object, uint32_t index, wxl_guid& out) const = 0;
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECTS_CONTAINER_HPP
