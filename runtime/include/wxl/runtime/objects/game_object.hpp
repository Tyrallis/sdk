// The leaf reads of a game object: what a runtime supplies for wxl.object's game-object getters.
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

#ifndef WXL_RUNTIME_OBJECTS_GAME_OBJECT_HPP
#define WXL_RUNTIME_OBJECTS_GAME_OBJECT_HPP

#include <cstdint>

#include "wxl/abi.h"

namespace wxl::runtime
{
    /// The reads a game object answers.
    class GameObjectReads
    {
    public:
        virtual ~GameObjectReads() = default;

        virtual bool GameObjectCreatedBy(void* object, wxl_guid& out) const    = 0;
        virtual bool GameObjectDisplayId(void* object, uint32_t& out) const    = 0;
        virtual bool GameObjectFlags(void* object, uint32_t& out) const        = 0;
        virtual bool GameObjectDynamicFlags(void* object, uint32_t& out) const = 0;
        virtual bool GameObjectFaction(void* object, uint32_t& out) const      = 0;
        virtual bool GameObjectLevel(void* object, uint32_t& out) const        = 0;
        virtual bool GameObjectState(void* object, uint32_t& out) const        = 0;
        virtual bool GameObjectType(void* object, uint32_t& out) const         = 0;
        virtual bool GameObjectArtKit(void* object, uint32_t& out) const       = 0;
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECTS_GAME_OBJECT_HPP
