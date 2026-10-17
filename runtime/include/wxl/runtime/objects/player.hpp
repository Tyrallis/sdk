// The leaf reads of a player: what a runtime supplies for wxl.object's player getters.
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

#ifndef WXL_RUNTIME_OBJECTS_PLAYER_HPP
#define WXL_RUNTIME_OBJECTS_PLAYER_HPP

#include <cstdint>

namespace wxl::runtime
{
    /// The reads a player answers, on top of UnitReads.
    class PlayerReads
    {
    public:
        virtual ~PlayerReads() = default;

        virtual bool PlayerFlags(void* object, uint32_t& out) const       = 0;
        virtual bool PlayerGuildId(void* object, uint32_t& out) const     = 0;
        virtual bool PlayerGuildRank(void* object, uint32_t& out) const   = 0;
        virtual bool PlayerChosenTitle(void* object, uint32_t& out) const = 0;

        /// The item id shown in an equipment slot, 0 for an empty one; false past the slots.
        virtual bool PlayerVisibleItem(void* object, uint32_t slot, uint32_t& out) const = 0;
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECTS_PLAYER_HPP
