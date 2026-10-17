// The leaf reads of a dynamic object: what a runtime supplies for wxl.object's area-effect getters.
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

#ifndef WXL_RUNTIME_OBJECTS_DYNAMIC_OBJECT_HPP
#define WXL_RUNTIME_OBJECTS_DYNAMIC_OBJECT_HPP

#include <cstdint>

#include "wxl/abi.h"

namespace wxl::runtime
{
    /// The reads a dynamic object (a spell's area effect) answers.
    class DynamicObjectReads
    {
    public:
        virtual ~DynamicObjectReads() = default;

        virtual bool DynamicCaster(void* object, wxl_guid& out) const   = 0;
        virtual bool DynamicSpellId(void* object, uint32_t& out) const  = 0;
        virtual bool DynamicRadius(void* object, float& out) const      = 0;
        virtual bool DynamicCastTime(void* object, uint32_t& out) const = 0;
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECTS_DYNAMIC_OBJECT_HPP
