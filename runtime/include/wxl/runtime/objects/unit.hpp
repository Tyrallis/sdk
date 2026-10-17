// The leaf reads of a unit: what a runtime supplies for wxl.object's unit getters.
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

#ifndef WXL_RUNTIME_OBJECTS_UNIT_HPP
#define WXL_RUNTIME_OBJECTS_UNIT_HPP

#include <cstdint>

#include "wxl/abi.h"

namespace wxl::runtime
{
    /// The reads a unit or a player answers. The service only asks for those two kinds.
    class UnitReads
    {
    public:
        virtual ~UnitReads() = default;

        virtual bool UnitHealth(void* object, uint32_t& out) const    = 0;
        virtual bool UnitMaxHealth(void* object, uint32_t& out) const = 0;
        virtual bool UnitLevel(void* object, uint32_t& out) const     = 0;

        /// The guid the unit targets; WXL_GUID_NONE for nothing. The service resolves it.
        virtual bool UnitTarget(void* object, wxl_guid& out) const = 0;

        /// Power by index; false past the client's power slots.
        virtual bool UnitPower(void* object, uint32_t index, uint32_t& out) const    = 0;
        virtual bool UnitMaxPower(void* object, uint32_t index, uint32_t& out) const = 0;
        virtual bool UnitPowerType(void* object, uint32_t& out) const                = 0;

        virtual bool UnitRace(void* object, uint32_t& out) const  = 0;
        virtual bool UnitClass(void* object, uint32_t& out) const = 0;
        virtual bool UnitSex(void* object, uint32_t& out) const   = 0;

        virtual bool UnitDisplayId(void* object, uint32_t& out) const       = 0;
        virtual bool UnitNativeDisplayId(void* object, uint32_t& out) const = 0;
        virtual bool UnitMountDisplayId(void* object, uint32_t& out) const  = 0;

        virtual bool UnitFactionTemplate(void* object, uint32_t& out) const = 0;
        virtual bool UnitFlags(void* object, uint32_t& out) const           = 0;
        virtual bool UnitFlags2(void* object, uint32_t& out) const          = 0;
        virtual bool UnitDynamicFlags(void* object, uint32_t& out) const    = 0;
        virtual bool UnitNpcFlags(void* object, uint32_t& out) const        = 0;

        virtual bool UnitBoundingRadius(void* object, float& out) const = 0;
        virtual bool UnitCombatReach(void* object, float& out) const    = 0;

        virtual bool UnitCharm(void* object, wxl_guid& out) const      = 0;
        virtual bool UnitSummon(void* object, wxl_guid& out) const     = 0;
        virtual bool UnitCharmedBy(void* object, wxl_guid& out) const  = 0;
        virtual bool UnitSummonedBy(void* object, wxl_guid& out) const = 0;
        virtual bool UnitCreatedBy(void* object, wxl_guid& out) const  = 0;
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECTS_UNIT_HPP
