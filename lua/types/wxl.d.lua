---@meta wxl
-- The Lua API of wxl.lua, for lua-language-server. Generated from the SDK's event
-- table (reflect.hpp) and object table (object_reads.def) by wxl-lua-types; do not
-- edit by hand.

---A guid, as "0x" and sixteen hex digits: nothing is lost and it keys a table.
---@alias wxl.Guid string

---An event's bytes, copied into a string.
---@alias wxl.Bytes string

---@class wxl.Vec3
---@field x number
---@field y number
---@field z number

-- The script families. A function defined on one of these tables as On<Event> is
-- subscribed when the entry file has run and called with the event's parameters;
-- on an event that can be answered its return values are taken, as Override
-- takes them.

---@class WorldScript
---@field OnUpdate fun(self: WorldScript, delta: number)
---@field OnWorldEnter fun(self: WorldScript, map_id: integer, position: wxl.Vec3)
---@field OnWorldLeave fun(self: WorldScript, map_id: integer, position: wxl.Vec3)
---@field OnWorldClick fun(self: WorldScript, point: wxl.Vec3, object: wxl.Object|nil, button: integer)
---@field OnInput fun(self: WorldScript, message: integer, wparam: integer, lparam: integer, handled: boolean|nil): boolean|nil answers through handled
---@field OnSoundPlay fun(self: WorldScript, sound_id: integer, path: string, suppressed: boolean|nil): boolean|nil answers through suppressed
WorldScript = {}

---@class RenderScript
---@field OnFrameBegin fun(self: RenderScript, index: integer, delta: number)
---@field OnFrameEnd fun(self: RenderScript, index: integer, delta: number)
---@field OnWorldRenderBegin fun(self: RenderScript, index: integer, delta: number)
---@field OnWorldRenderEnd fun(self: RenderScript, index: integer, delta: number)
---@field OnUiRenderEnd fun(self: RenderScript, index: integer, delta: number)
---@field OnSwapchainLost fun(self: RenderScript, width: integer, height: integer)
---@field OnSwapchainRestored fun(self: RenderScript, width: integer, height: integer)
RenderScript = {}

---@class ObjectScript
---@field OnObjectCreate fun(self: ObjectScript, object: wxl.Object|nil, guid: wxl.Guid, type: integer)
---@field OnObjectUpdate fun(self: ObjectScript, object: wxl.Object|nil, guid: wxl.Guid, type: integer)
---@field OnObjectDestroy fun(self: ObjectScript, object: wxl.Object|nil, guid: wxl.Guid, type: integer)
---@field OnTargetChanged fun(self: ObjectScript, previous: wxl.Object|nil, current: wxl.Object|nil)
---@field OnDoodadSpawn fun(self: ObjectScript, doodad: wxl.Object|nil, path: string, position: wxl.Vec3)
---@field OnEquipmentChanged fun(self: ObjectScript, unit: wxl.Object|nil, slot: integer, item_id: integer)
---@field OnUnitDeath fun(self: ObjectScript, unit: wxl.Object|nil)
---@field OnUnitAuraChanged fun(self: ObjectScript, unit: wxl.Object|nil, spell_id: integer, removed: boolean)
---@field OnSpellCastStart fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, cast_item: wxl.Guid, spell_id: integer, cast_count: integer, cast_flags: integer, target_flags: integer, cast_time: integer, hit_count: integer, hits: wxl.Bytes, misses: wxl.Bytes, miss_reasons: wxl.Bytes)
---@field OnSpellCastGo fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, cast_item: wxl.Guid, spell_id: integer, cast_count: integer, cast_flags: integer, target_flags: integer, cast_time: integer, hit_count: integer, hits: wxl.Bytes, misses: wxl.Bytes, miss_reasons: wxl.Bytes)
---@field OnSpellCastFailed fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, custom_error: integer, custom_arg: integer)
---@field OnSpellCastInterrupted fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, custom_error: integer, custom_arg: integer)
---@field OnSpellCastRequest fun(self: ObjectScript, caster: wxl.Object|nil, spell_id: integer, item: wxl.Object|nil, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil): boolean|nil answers through refused
---@field OnItemUseRequest fun(self: ObjectScript, item: wxl.Object|nil, item_guid: wxl.Guid, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil): boolean|nil answers through refused
---@field OnSpellChannelStart fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)
---@field OnSpellChannelUpdate fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)
---@field OnSpellChannelStop fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)
---@field OnSpellCastResume fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)
---@field OnSpellCastDelayed fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, delay: integer)
---@field OnSpellCooldown fun(self: ObjectScript, unit: wxl.Object|nil, unit_guid: wxl.Guid, item: wxl.Object|nil, item_guid: wxl.Guid, spell_id: integer, duration: integer, flags: integer)
---@field OnSpellCooldownChanged fun(self: ObjectScript, unit: wxl.Object|nil, unit_guid: wxl.Guid, spell_id: integer, change: integer, delay: integer)
---@field OnSpellCastCancel fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, notify_server: boolean)
ObjectScript = {}

---@class AssetScript
---@field OnModelLoad fun(self: AssetScript, path: string, data: wxl.Bytes, replacement: lightuserdata|nil, replacement_size: lightuserdata|nil)
---@field OnTextureLoad fun(self: AssetScript, path: string)
---@field OnTerrainChunkBuild fun(self: AssetScript, map_id: integer, tile_x: integer, tile_y: integer, chunk_x: integer, chunk_y: integer)
---@field OnMapObjectLoad fun(self: AssetScript, path: string, data: wxl.Bytes)
---@field OnFileOpen fun(self: AssetScript, path: string, redirect: string|nil): string|nil answers through redirect
---@field OnFileOpened fun(self: AssetScript, path: string, found: boolean, size: integer)
---@field OnClientDbLoaded fun(self: AssetScript, name: string, records: lightuserdata|nil, record_count: integer, record_size: integer)
AssetScript = {}

---@class NetScript
---@field OnMessageReceived fun(self: NetScript, opcode: integer, data: wxl.Bytes, suppressed: boolean|nil): boolean|nil answers through suppressed
---@field OnMessageSent fun(self: NetScript, opcode: integer, data: wxl.Bytes, suppressed: boolean|nil): boolean|nil answers through suppressed
---@field OnConnected fun(self: NetScript, address: string)
---@field OnDisconnected fun(self: NetScript, address: string)
NetScript = {}

---@class LuaScript
---@field OnScriptVmReady fun(self: LuaScript, context: integer, native_state: lightuserdata|nil)
---@field OnScriptVmClosing fun(self: LuaScript, context: integer, native_state: lightuserdata|nil)
---@field OnUiEvent fun(self: LuaScript, index: integer, name: string, format: string, values: wxl.Bytes, value_count: integer)
---@field OnScriptError fun(self: LuaScript, context: integer, message: string)
LuaScript = {}

---@class CombatScript
---@field OnCombatAttackStart fun(self: CombatScript, attacker: wxl.Object|nil, attacker_guid: wxl.Guid, victim: wxl.Object|nil, victim_guid: wxl.Guid)
---@field OnCombatAttackStop fun(self: CombatScript, attacker: wxl.Object|nil, attacker_guid: wxl.Guid, victim: wxl.Object|nil, victim_guid: wxl.Guid)
---@field OnCombatDamage fun(self: CombatScript, source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, kind: integer, environment: integer, amount: integer, overkill: integer, school: integer, absorbed: integer, resisted: integer, blocked: integer, crit: boolean, hit_info: integer)
---@field OnCombatHeal fun(self: CombatScript, source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, amount: integer, overheal: integer, absorbed: integer, crit: boolean, periodic: boolean)
---@field OnCombatMiss fun(self: CombatScript, source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, melee: boolean, reason: integer, hit_info: integer)
CombatScript = {}

---@alias wxl.Family "WorldScript"|"RenderScript"|"ObjectScript"|"AssetScript"|"NetScript"|"LuaScript"|"CombatScript"

---Calls `handler` with the event's parameters whenever it is raised. Returns the
---handler's id.
---@overload fun(family: "WorldScript", method: "OnUpdate", handler: fun(delta: number)): integer
---@overload fun(family: "WorldScript", method: "OnWorldEnter", handler: fun(map_id: integer, position: wxl.Vec3)): integer
---@overload fun(family: "WorldScript", method: "OnWorldLeave", handler: fun(map_id: integer, position: wxl.Vec3)): integer
---@overload fun(family: "WorldScript", method: "OnWorldClick", handler: fun(point: wxl.Vec3, object: wxl.Object|nil, button: integer)): integer
---@overload fun(family: "WorldScript", method: "OnInput", handler: fun(message: integer, wparam: integer, lparam: integer, handled: boolean|nil)): integer
---@overload fun(family: "WorldScript", method: "OnSoundPlay", handler: fun(sound_id: integer, path: string, suppressed: boolean|nil)): integer
---@overload fun(family: "RenderScript", method: "OnFrameBegin", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnFrameEnd", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnWorldRenderBegin", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnWorldRenderEnd", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnUiRenderEnd", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnSwapchainLost", handler: fun(width: integer, height: integer)): integer
---@overload fun(family: "RenderScript", method: "OnSwapchainRestored", handler: fun(width: integer, height: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectCreate", handler: fun(object: wxl.Object|nil, guid: wxl.Guid, type: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectUpdate", handler: fun(object: wxl.Object|nil, guid: wxl.Guid, type: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectDestroy", handler: fun(object: wxl.Object|nil, guid: wxl.Guid, type: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnTargetChanged", handler: fun(previous: wxl.Object|nil, current: wxl.Object|nil)): integer
---@overload fun(family: "ObjectScript", method: "OnDoodadSpawn", handler: fun(doodad: wxl.Object|nil, path: string, position: wxl.Vec3)): integer
---@overload fun(family: "ObjectScript", method: "OnEquipmentChanged", handler: fun(unit: wxl.Object|nil, slot: integer, item_id: integer)): integer
---@overload fun(family: "AssetScript", method: "OnModelLoad", handler: fun(path: string, data: wxl.Bytes, replacement: lightuserdata|nil, replacement_size: lightuserdata|nil)): integer
---@overload fun(family: "AssetScript", method: "OnTextureLoad", handler: fun(path: string)): integer
---@overload fun(family: "AssetScript", method: "OnTerrainChunkBuild", handler: fun(map_id: integer, tile_x: integer, tile_y: integer, chunk_x: integer, chunk_y: integer)): integer
---@overload fun(family: "AssetScript", method: "OnMapObjectLoad", handler: fun(path: string, data: wxl.Bytes)): integer
---@overload fun(family: "AssetScript", method: "OnFileOpen", handler: fun(path: string, redirect: string|nil)): integer
---@overload fun(family: "AssetScript", method: "OnFileOpened", handler: fun(path: string, found: boolean, size: integer)): integer
---@overload fun(family: "AssetScript", method: "OnClientDbLoaded", handler: fun(name: string, records: lightuserdata|nil, record_count: integer, record_size: integer)): integer
---@overload fun(family: "NetScript", method: "OnMessageReceived", handler: fun(opcode: integer, data: wxl.Bytes, suppressed: boolean|nil)): integer
---@overload fun(family: "NetScript", method: "OnMessageSent", handler: fun(opcode: integer, data: wxl.Bytes, suppressed: boolean|nil)): integer
---@overload fun(family: "NetScript", method: "OnConnected", handler: fun(address: string)): integer
---@overload fun(family: "NetScript", method: "OnDisconnected", handler: fun(address: string)): integer
---@overload fun(family: "LuaScript", method: "OnScriptVmReady", handler: fun(context: integer, native_state: lightuserdata|nil)): integer
---@overload fun(family: "LuaScript", method: "OnScriptVmClosing", handler: fun(context: integer, native_state: lightuserdata|nil)): integer
---@overload fun(family: "ObjectScript", method: "OnUnitDeath", handler: fun(unit: wxl.Object|nil)): integer
---@overload fun(family: "ObjectScript", method: "OnUnitAuraChanged", handler: fun(unit: wxl.Object|nil, spell_id: integer, removed: boolean)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastStart", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, cast_item: wxl.Guid, spell_id: integer, cast_count: integer, cast_flags: integer, target_flags: integer, cast_time: integer, hit_count: integer, hits: wxl.Bytes, misses: wxl.Bytes, miss_reasons: wxl.Bytes)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastGo", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, cast_item: wxl.Guid, spell_id: integer, cast_count: integer, cast_flags: integer, target_flags: integer, cast_time: integer, hit_count: integer, hits: wxl.Bytes, misses: wxl.Bytes, miss_reasons: wxl.Bytes)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastFailed", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, custom_error: integer, custom_arg: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastInterrupted", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, custom_error: integer, custom_arg: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastRequest", handler: fun(caster: wxl.Object|nil, spell_id: integer, item: wxl.Object|nil, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil)): integer
---@overload fun(family: "ObjectScript", method: "OnItemUseRequest", handler: fun(item: wxl.Object|nil, item_guid: wxl.Guid, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil)): integer
---@overload fun(family: "LuaScript", method: "OnUiEvent", handler: fun(index: integer, name: string, format: string, values: wxl.Bytes, value_count: integer)): integer
---@overload fun(family: "LuaScript", method: "OnScriptError", handler: fun(context: integer, message: string)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellChannelStart", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellChannelUpdate", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellChannelStop", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastResume", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastDelayed", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, delay: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCooldown", handler: fun(unit: wxl.Object|nil, unit_guid: wxl.Guid, item: wxl.Object|nil, item_guid: wxl.Guid, spell_id: integer, duration: integer, flags: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCooldownChanged", handler: fun(unit: wxl.Object|nil, unit_guid: wxl.Guid, spell_id: integer, change: integer, delay: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastCancel", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, notify_server: boolean)): integer
---@overload fun(family: "CombatScript", method: "OnCombatAttackStart", handler: fun(attacker: wxl.Object|nil, attacker_guid: wxl.Guid, victim: wxl.Object|nil, victim_guid: wxl.Guid)): integer
---@overload fun(family: "CombatScript", method: "OnCombatAttackStop", handler: fun(attacker: wxl.Object|nil, attacker_guid: wxl.Guid, victim: wxl.Object|nil, victim_guid: wxl.Guid)): integer
---@overload fun(family: "CombatScript", method: "OnCombatDamage", handler: fun(source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, kind: integer, environment: integer, amount: integer, overkill: integer, school: integer, absorbed: integer, resisted: integer, blocked: integer, crit: boolean, hit_info: integer)): integer
---@overload fun(family: "CombatScript", method: "OnCombatHeal", handler: fun(source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, amount: integer, overheal: integer, absorbed: integer, crit: boolean, periodic: boolean)): integer
---@overload fun(family: "CombatScript", method: "OnCombatMiss", handler: fun(source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, melee: boolean, reason: integer, hit_info: integer)): integer
---@param family wxl.Family
---@param method string
---@param handler function
---@return integer id
function Observe(family, method, handler) end

---Like Observe, on an event that can be answered. The handler returns one value per
---member it answers through, in member order: true sets an answer, a string fills a
---buffer, nil leaves the member as it is. Taken on the main thread only.
---@overload fun(family: "WorldScript", method: "OnInput", handler: fun(message: integer, wparam: integer, lparam: integer, handled: boolean|nil): boolean|nil): integer
---@overload fun(family: "WorldScript", method: "OnSoundPlay", handler: fun(sound_id: integer, path: string, suppressed: boolean|nil): boolean|nil): integer
---@overload fun(family: "AssetScript", method: "OnFileOpen", handler: fun(path: string, redirect: string|nil): string|nil): integer
---@overload fun(family: "NetScript", method: "OnMessageReceived", handler: fun(opcode: integer, data: wxl.Bytes, suppressed: boolean|nil): boolean|nil): integer
---@overload fun(family: "NetScript", method: "OnMessageSent", handler: fun(opcode: integer, data: wxl.Bytes, suppressed: boolean|nil): boolean|nil): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastRequest", handler: fun(caster: wxl.Object|nil, spell_id: integer, item: wxl.Object|nil, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil): boolean|nil): integer
---@overload fun(family: "ObjectScript", method: "OnItemUseRequest", handler: fun(item: wxl.Object|nil, item_guid: wxl.Guid, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil): boolean|nil): integer
---@param family wxl.Family
---@param method string
---@param handler function
---@return integer id
function Override(family, method, handler) end

---Drops a handler by the id Observe or Override returned.
---@param id integer
---@return boolean dropped
function Unobserve(id) end

---@alias wxl.CallbackName "onInit"|"onUpdate"|"onDraw"|"onShutdown"

---Registers a callback: onInit at WXL_STAGE_READY (or right after a reload),
---onUpdate(delta) at every World.Update, onDraw at every Render.UiRenderEnd,
---onShutdown at WXL_STAGE_SHUTDOWN and before a reload.
---@overload fun(event: "onInit", fn: fun())
---@overload fun(event: "onUpdate", fn: fun(delta: number))
---@overload fun(event: "onDraw", fn: fun())
---@overload fun(event: "onShutdown", fn: fun())
---@param event wxl.CallbackName
---@param fn function
function registerForEvent(event, fn) end

---@alias wxl.ObjectType "Item"|"Container"|"Unit"|"Player"|"GameObject"|"DynamicObject"|"Corpse"|"Unknown"

---A world object, by handle. Every getter answers the value, or nil and why:
---"gone" for an object that left, "unsupported" for a runtime without the read, or
---what the object is not ("a GameObject has no Health").
---@class wxl.Object
local Object = {}

---Whether the handle still resolves.
---@return boolean
function Object:Valid() end

---The server's id for the object, as "0x" and sixteen hex digits.
---@return string|nil guid, string|nil reason
function Object:Guid() end

---@return wxl.ObjectType|nil type, string|nil reason
function Object:Type() end

---@return integer|nil type, string|nil reason
function Object:TypeId() end

---@return string|nil name, string|nil reason
function Object:Name() end

---What a unit targets; nil when nothing in view.
---@return wxl.Object|nil target, string|nil reason
function Object:Target() end

---The handle itself, as "0x" and sixteen hex digits: for keys and logs.
---@return string
function Object:Handle() end

---For any object (position).
---@return wxl.Vec3|nil value, string|nil reason
function Object:Position() end

---For any object (facing).
---@return number|nil value, string|nil reason
function Object:Facing() end

---For any object (entry).
---@return integer|nil value, string|nil reason
function Object:Entry() end

---For any object (scale).
---@return number|nil value, string|nil reason
function Object:Scale() end

---For Unit, Player (unit_health).
---@return integer|nil value, string|nil reason
function Object:Health() end

---For Unit, Player (unit_max_health).
---@return integer|nil value, string|nil reason
function Object:MaxHealth() end

---For Unit, Player; GameObject (unit_level, gameobject_level).
---@return integer|nil value, string|nil reason
function Object:Level() end

---For Unit, Player (unit_power).
---@param index integer
---@return integer|nil value, string|nil reason
function Object:Power(index) end

---For Unit, Player (unit_max_power).
---@param index integer
---@return integer|nil value, string|nil reason
function Object:MaxPower(index) end

---For Unit, Player (unit_power_type).
---@return integer|nil value, string|nil reason
function Object:PowerType() end

---For Unit, Player (unit_race).
---@return integer|nil value, string|nil reason
function Object:Race() end

---For Unit, Player (unit_class).
---@return integer|nil value, string|nil reason
function Object:Class() end

---For Unit, Player (unit_sex).
---@return integer|nil value, string|nil reason
function Object:Sex() end

---For Unit, Player; GameObject; Corpse (unit_display_id, gameobject_display_id, corpse_display_id).
---@return integer|nil value, string|nil reason
function Object:DisplayId() end

---For Unit, Player (unit_native_display_id).
---@return integer|nil value, string|nil reason
function Object:NativeDisplayId() end

---For Unit, Player (unit_mount_display_id).
---@return integer|nil value, string|nil reason
function Object:MountDisplayId() end

---For Unit, Player (unit_faction_template).
---@return integer|nil value, string|nil reason
function Object:FactionTemplate() end

---For Unit, Player; Item, Container; GameObject; Corpse (unit_flags, item_flags, gameobject_flags, corpse_flags).
---@return integer|nil value, string|nil reason
function Object:Flags() end

---For Unit, Player (unit_flags2).
---@return integer|nil value, string|nil reason
function Object:Flags2() end

---For Unit, Player; GameObject; Corpse (unit_dynamic_flags, gameobject_dynamic_flags, corpse_dynamic_flags).
---@return integer|nil value, string|nil reason
function Object:DynamicFlags() end

---For Unit, Player (unit_npc_flags).
---@return integer|nil value, string|nil reason
function Object:NpcFlags() end

---For Unit, Player (unit_bounding_radius).
---@return number|nil value, string|nil reason
function Object:BoundingRadius() end

---For Unit, Player (unit_combat_reach).
---@return number|nil value, string|nil reason
function Object:CombatReach() end

---For Unit, Player (unit_charm).
---@return string|nil value, string|nil reason
function Object:Charm() end

---For Unit, Player (unit_summon).
---@return string|nil value, string|nil reason
function Object:Summon() end

---For Unit, Player (unit_charmed_by).
---@return string|nil value, string|nil reason
function Object:CharmedBy() end

---For Unit, Player (unit_summoned_by).
---@return string|nil value, string|nil reason
function Object:SummonedBy() end

---For Unit, Player; GameObject (unit_created_by, gameobject_created_by).
---@return string|nil value, string|nil reason
function Object:CreatedBy() end

---For Player (player_flags).
---@return integer|nil value, string|nil reason
function Object:PlayerFlags() end

---For Player (player_guild_id).
---@return integer|nil value, string|nil reason
function Object:GuildId() end

---For Player (player_guild_rank).
---@return integer|nil value, string|nil reason
function Object:GuildRank() end

---For Player (player_chosen_title).
---@return integer|nil value, string|nil reason
function Object:ChosenTitle() end

---For Player (player_visible_item).
---@param index integer
---@return integer|nil value, string|nil reason
function Object:VisibleItem(index) end

---For Item, Container; Corpse (item_owner, corpse_owner).
---@return string|nil value, string|nil reason
function Object:Owner() end

---For Item, Container (item_contained_in).
---@return string|nil value, string|nil reason
function Object:ContainedIn() end

---For Item, Container (item_creator).
---@return string|nil value, string|nil reason
function Object:Creator() end

---For Item, Container (item_stack_count).
---@return integer|nil value, string|nil reason
function Object:StackCount() end

---For Item, Container (item_duration).
---@return integer|nil value, string|nil reason
function Object:Duration() end

---For Item, Container (item_durability).
---@return integer|nil value, string|nil reason
function Object:Durability() end

---For Item, Container (item_max_durability).
---@return integer|nil value, string|nil reason
function Object:MaxDurability() end

---For Container (container_num_slots).
---@return integer|nil value, string|nil reason
function Object:NumSlots() end

---For Container (container_slot).
---@param index integer
---@return string|nil value, string|nil reason
function Object:Slot(index) end

---For GameObject (gameobject_faction).
---@return integer|nil value, string|nil reason
function Object:Faction() end

---For GameObject (gameobject_state).
---@return integer|nil value, string|nil reason
function Object:State() end

---For GameObject (gameobject_type).
---@return integer|nil value, string|nil reason
function Object:Type() end

---For GameObject (gameobject_art_kit).
---@return integer|nil value, string|nil reason
function Object:ArtKit() end

---For DynamicObject (dynamic_caster).
---@return string|nil value, string|nil reason
function Object:Caster() end

---For DynamicObject (dynamic_spell_id).
---@return integer|nil value, string|nil reason
function Object:SpellId() end

---For DynamicObject (dynamic_radius).
---@return number|nil value, string|nil reason
function Object:Radius() end

---For DynamicObject (dynamic_cast_time).
---@return integer|nil value, string|nil reason
function Object:CastTime() end

---For Corpse (corpse_party).
---@return string|nil value, string|nil reason
function Object:Party() end

---For Corpse (corpse_guild).
---@return integer|nil value, string|nil reason
function Object:Guild() end

---@class wxl.ModInfo
---@field id string
---@field name string
---@field version string
---@field directory string

---@class wxl.RuntimeInfo
---@field id string
---@field name string
---@field version integer
---@field abi string the contract this mod is served, "major.minor.feature"
---@field abi_major integer
---@field abi_minor integer
---@field abi_feature integer

---@class wxl.object
local object = {}

---The object the local player controls, or nil while out of the world.
---@return wxl.Object|nil
function object.LocalPlayer() end

---The object carrying a guid, or nil when it is not in view.
---@param guid string|integer
---@return wxl.Object|nil
function object.FromGuid(guid) end

---The objects of a type in view, or every object.
---@param type? wxl.ObjectType|integer
---@return wxl.Object[]
function object.InView(type) end

---@class wxl
---@field mod wxl.ModInfo this mod, from its manifest or its folder
---@field runtime wxl.RuntimeInfo the runtime that loaded wxl.lua
---@field object wxl.object
---@field events table<wxl.Family, string[]> every family's events, as On<Event>
wxl = {}

---Writes a line to the runtime's log, tagged with the mod's id.
---@param level "trace"|"debug"|"info"|"warn"|"error"
---@param ... any
function wxl.log(level, ...) end
