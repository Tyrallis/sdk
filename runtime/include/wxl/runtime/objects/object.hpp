// The leaf reads of any object: what a runtime supplies for wxl.object's base getters.
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

#ifndef WXL_RUNTIME_OBJECTS_OBJECT_HPP
#define WXL_RUNTIME_OBJECTS_OBJECT_HPP

#include <cstdint>

#include "wxl/abi.h"
#include "wxl/events.h"

namespace wxl::runtime
{
    /**
     * @brief The reads every object answers.
     *
     * Each takes the native object the handle resolved to -- never a handle, never a null: the
     * service has already checked both. A read that does not apply to this object answers false
     * and leaves @p out alone.
     */
    class ObjectReads
    {
    public:
        virtual ~ObjectReads() = default;

        virtual wxl_guid        Guid(void* object) const = 0;
        virtual wxl_object_type Type(void* object) const = 0;

        virtual bool Entry(void* object, uint32_t& out) const    = 0;
        virtual bool Scale(void* object, float& out) const       = 0;
        virtual bool Position(void* object, wxl_vec3& out) const = 0;
        virtual bool Facing(void* object, float& out) const      = 0;

        /**
         * @brief The display name.
         *
         * The binding owns the storage and keeps it valid until its next call, which is what the
         * service's own contract promises a mod.
         */
        virtual bool Name(void* object, const char*& out, uint32_t& size) const = 0;
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECTS_OBJECT_HPP
