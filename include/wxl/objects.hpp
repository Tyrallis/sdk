// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// World objects as C++ values: the umbrella over wxl/objects/.

#ifndef WXL_OBJECTS_HPP
#define WXL_OBJECTS_HPP

// One class per kind of object the client knows, each in its own header, plus the view, the
// facade, and ObjectMap for data kept per object. The kinds are the client's own OBJECT_TYPE_ID
// set: Object, Item, Container, Unit, Player, GameObject, DynamicObject, Corpse. A per-type header
// stands alone; this one brings the whole taxonomy and the facade's definitions.

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
#include "wxl/objects/map.hpp"

#endif // WXL_OBJECTS_HPP
