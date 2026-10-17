// World objects as C++ values: the umbrella over wxl/objects/.
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

#ifndef WXL_OBJECTS_HPP
#define WXL_OBJECTS_HPP

// One class per kind of object the client knows, each in its own header, plus the view and the
// facade. The kinds are the client's own OBJECT_TYPE_ID set: Object, Item, Container, Unit,
// Player, GameObject, DynamicObject, Corpse. A per-type header stands alone; this one brings the
// whole taxonomy and the facade's definitions.

#include "wxl/objects/facade.hpp"
#include "wxl/objects/object.hpp"
#include "wxl/objects/item.hpp"
#include "wxl/objects/container.hpp"
#include "wxl/objects/unit.hpp"
#include "wxl/objects/player.hpp"
#include "wxl/objects/game_object.hpp"
#include "wxl/objects/dynamic_object.hpp"
#include "wxl/objects/corpse.hpp"
#include "wxl/objects/view.hpp"
#include "wxl/objects/service.hpp"

#endif // WXL_OBJECTS_HPP
