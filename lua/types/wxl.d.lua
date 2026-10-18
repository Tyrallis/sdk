---@meta wxl
-- The Lua API of wxl.lua, for lua-language-server. Generated from the SDK's event
-- table (reflect.hpp) and object table (object_reads.def) by wxl-lua-types; do not
-- edit by hand.

-- The args of each event: a userdata read member by member, valid during the
-- handler's call.

---@class wxl.UpdateArgs
---@field delta number

---@class wxl.WorldArgs
---@field map_id integer
---@field position wxl.Vec3

---@class wxl.WorldClickArgs
---@field point wxl.Vec3
---@field object wxl.Object|nil
---@field button integer

---@class wxl.InputArgs
---@field message integer
---@field wparam integer
---@field lparam integer
---@field handled boolean|nil the answer: read what another handler wrote, or write true from an Override

---@class wxl.SoundArgs
---@field sound_id integer
---@field path string
---@field suppressed boolean|nil the answer: read what another handler wrote, or write true from an Override

---@class wxl.FrameArgs
---@field index integer
---@field delta number

---@class wxl.SwapchainArgs
---@field width integer
---@field height integer

---@class wxl.ObjectArgs
---@field object wxl.Object|nil
---@field guid string a guid, as "0x" and sixteen hex digits
---@field type integer

---@class wxl.TargetArgs
---@field previous wxl.Object|nil
---@field current wxl.Object|nil

---@class wxl.DoodadArgs
---@field doodad wxl.Object|nil
---@field path string
---@field position wxl.Vec3

---@class wxl.EquipmentArgs
---@field unit wxl.Object|nil
---@field slot integer
---@field item_id integer

---@class wxl.ModelLoadArgs
---@field path string
---@field data string the bytes, valid for the handler
---@field size integer
---@field replacement lightuserdata|nil native only; nothing in this API takes it
---@field replacement_size lightuserdata|nil native only; nothing in this API takes it

---@class wxl.TextureLoadArgs
---@field path string

---@class wxl.TerrainChunkArgs
---@field map_id integer
---@field tile_x integer
---@field tile_y integer
---@field chunk_x integer
---@field chunk_y integer

---@class wxl.MapObjectArgs
---@field path string
---@field data string the bytes, valid for the handler
---@field size integer

---@class wxl.FileOpenArgs
---@field path string
---@field redirect string|nil the buffer: read what another handler wrote, or write a string from an Override
---@field redirect_capacity integer

---@class wxl.FileOpenedArgs
---@field path string
---@field found boolean
---@field size integer

---@class wxl.ClientDbArgs
---@field name string
---@field records lightuserdata|nil native only; nothing in this API takes it
---@field record_count integer
---@field record_size integer

---@class wxl.MessageArgs
---@field opcode integer
---@field data string the bytes, valid for the handler
---@field size integer
---@field suppressed boolean|nil the answer: read what another handler wrote, or write true from an Override

---@class wxl.ConnectionArgs
---@field address string

---@class wxl.ScriptVmArgs
---@field context integer
---@field native_state lightuserdata|nil native only; nothing in this API takes it

---@class wxl.UnitArgs
---@field unit wxl.Object|nil

---@class wxl.UnitAuraArgs
---@field unit wxl.Object|nil
---@field spell_id integer
---@field removed boolean

---@class wxl.SpellCastArgs
---@field caster wxl.Object|nil
---@field caster_guid string a guid, as "0x" and sixteen hex digits
---@field target wxl.Object|nil
---@field target_guid string a guid, as "0x" and sixteen hex digits
---@field cast_item string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field cast_count integer
---@field cast_flags integer
---@field target_flags integer
---@field cast_time integer
---@field hit_count integer
---@field miss_count integer
---@field hits string the bytes, valid for the handler
---@field hits_size integer
---@field misses string the bytes, valid for the handler
---@field misses_size integer
---@field miss_reasons string the bytes, valid for the handler

---@class wxl.SpellFailArgs
---@field caster wxl.Object|nil
---@field caster_guid string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field cast_count integer
---@field reason integer
---@field custom_error integer
---@field custom_arg integer

---@class wxl.SpellRequestArgs
---@field caster wxl.Object|nil
---@field spell_id integer
---@field item wxl.Object|nil
---@field target_guid string a guid, as "0x" and sixteen hex digits
---@field confirmed boolean
---@field refused boolean|nil the answer: read what another handler wrote, or write true from an Override

---@class wxl.ItemUseArgs
---@field item wxl.Object|nil
---@field item_guid string a guid, as "0x" and sixteen hex digits
---@field target_guid string a guid, as "0x" and sixteen hex digits
---@field confirmed boolean
---@field refused boolean|nil the answer: read what another handler wrote, or write true from an Override

---@class wxl.UiEventArgs
---@field index integer
---@field name string
---@field format string
---@field values string the bytes, valid for the handler
---@field values_size integer
---@field value_count integer

---@class wxl.ScriptErrorArgs
---@field context integer
---@field message string

---@class wxl.SpellProgressArgs
---@field caster wxl.Object|nil
---@field caster_guid string a guid, as "0x" and sixteen hex digits
---@field target wxl.Object|nil
---@field target_guid string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field duration integer
---@field remaining integer

---@class wxl.SpellDelayArgs
---@field caster wxl.Object|nil
---@field caster_guid string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field delay integer

---@class wxl.SpellCooldownArgs
---@field unit wxl.Object|nil
---@field unit_guid string a guid, as "0x" and sixteen hex digits
---@field item wxl.Object|nil
---@field item_guid string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field duration integer
---@field flags integer

---@class wxl.SpellCooldownChangeArgs
---@field unit wxl.Object|nil
---@field unit_guid string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field change integer
---@field delay integer

---@class wxl.SpellCancelArgs
---@field caster wxl.Object|nil
---@field caster_guid string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field cast_count integer
---@field reason integer
---@field notify_server boolean

---@class wxl.CombatAttackArgs
---@field attacker wxl.Object|nil
---@field attacker_guid string a guid, as "0x" and sixteen hex digits
---@field victim wxl.Object|nil
---@field victim_guid string a guid, as "0x" and sixteen hex digits

---@class wxl.CombatDamageArgs
---@field source wxl.Object|nil
---@field source_guid string a guid, as "0x" and sixteen hex digits
---@field target wxl.Object|nil
---@field target_guid string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field kind integer
---@field environment integer
---@field amount integer
---@field overkill integer
---@field school integer
---@field absorbed integer
---@field resisted integer
---@field blocked integer
---@field crit boolean
---@field hit_info integer

---@class wxl.CombatHealArgs
---@field source wxl.Object|nil
---@field source_guid string a guid, as "0x" and sixteen hex digits
---@field target wxl.Object|nil
---@field target_guid string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field amount integer
---@field overheal integer
---@field absorbed integer
---@field crit boolean
---@field periodic boolean

---@class wxl.CombatMissArgs
---@field source wxl.Object|nil
---@field source_guid string a guid, as "0x" and sixteen hex digits
---@field target wxl.Object|nil
---@field target_guid string a guid, as "0x" and sixteen hex digits
---@field spell_id integer
---@field melee boolean
---@field reason integer
---@field hit_info integer

---@class wxl.Vec3
---@field x number
---@field y number
---@field z number

-- The script families. A function defined on one of these tables as On<Event> is
-- subscribed when the entry file has run; on an event that can be answered its
-- return value is taken, as Override takes it.

---@class WorldScript
---@field OnUpdate fun(self: WorldScript, args: wxl.UpdateArgs)
---@field OnWorldEnter fun(self: WorldScript, args: wxl.WorldArgs)
---@field OnWorldLeave fun(self: WorldScript, args: wxl.WorldArgs)
---@field OnWorldClick fun(self: WorldScript, args: wxl.WorldClickArgs)
---@field OnInput fun(self: WorldScript, args: wxl.InputArgs): boolean|nil
---@field OnSoundPlay fun(self: WorldScript, args: wxl.SoundArgs): boolean|nil
WorldScript = {}

---@class RenderScript
---@field OnFrameBegin fun(self: RenderScript, args: wxl.FrameArgs)
---@field OnFrameEnd fun(self: RenderScript, args: wxl.FrameArgs)
---@field OnWorldRenderBegin fun(self: RenderScript, args: wxl.FrameArgs)
---@field OnWorldRenderEnd fun(self: RenderScript, args: wxl.FrameArgs)
---@field OnUiRenderEnd fun(self: RenderScript, args: wxl.FrameArgs)
---@field OnSwapchainLost fun(self: RenderScript, args: wxl.SwapchainArgs)
---@field OnSwapchainRestored fun(self: RenderScript, args: wxl.SwapchainArgs)
RenderScript = {}

---@class ObjectScript
---@field OnObjectCreate fun(self: ObjectScript, args: wxl.ObjectArgs)
---@field OnObjectUpdate fun(self: ObjectScript, args: wxl.ObjectArgs)
---@field OnObjectDestroy fun(self: ObjectScript, args: wxl.ObjectArgs)
---@field OnTargetChanged fun(self: ObjectScript, args: wxl.TargetArgs)
---@field OnDoodadSpawn fun(self: ObjectScript, args: wxl.DoodadArgs)
---@field OnEquipmentChanged fun(self: ObjectScript, args: wxl.EquipmentArgs)
---@field OnUnitDeath fun(self: ObjectScript, args: wxl.UnitArgs)
---@field OnUnitAuraChanged fun(self: ObjectScript, args: wxl.UnitAuraArgs)
---@field OnSpellCastStart fun(self: ObjectScript, args: wxl.SpellCastArgs)
---@field OnSpellCastGo fun(self: ObjectScript, args: wxl.SpellCastArgs)
---@field OnSpellCastFailed fun(self: ObjectScript, args: wxl.SpellFailArgs)
---@field OnSpellCastInterrupted fun(self: ObjectScript, args: wxl.SpellFailArgs)
---@field OnSpellCastRequest fun(self: ObjectScript, args: wxl.SpellRequestArgs): boolean|nil
---@field OnItemUseRequest fun(self: ObjectScript, args: wxl.ItemUseArgs): boolean|nil
---@field OnSpellChannelStart fun(self: ObjectScript, args: wxl.SpellProgressArgs)
---@field OnSpellChannelUpdate fun(self: ObjectScript, args: wxl.SpellProgressArgs)
---@field OnSpellChannelStop fun(self: ObjectScript, args: wxl.SpellProgressArgs)
---@field OnSpellCastResume fun(self: ObjectScript, args: wxl.SpellProgressArgs)
---@field OnSpellCastDelayed fun(self: ObjectScript, args: wxl.SpellDelayArgs)
---@field OnSpellCooldown fun(self: ObjectScript, args: wxl.SpellCooldownArgs)
---@field OnSpellCooldownChanged fun(self: ObjectScript, args: wxl.SpellCooldownChangeArgs)
---@field OnSpellCastCancel fun(self: ObjectScript, args: wxl.SpellCancelArgs)
ObjectScript = {}

---@class AssetScript
---@field OnModelLoad fun(self: AssetScript, args: wxl.ModelLoadArgs)
---@field OnTextureLoad fun(self: AssetScript, args: wxl.TextureLoadArgs)
---@field OnTerrainChunkBuild fun(self: AssetScript, args: wxl.TerrainChunkArgs)
---@field OnMapObjectLoad fun(self: AssetScript, args: wxl.MapObjectArgs)
---@field OnFileOpen fun(self: AssetScript, args: wxl.FileOpenArgs): string|nil
---@field OnFileOpened fun(self: AssetScript, args: wxl.FileOpenedArgs)
---@field OnClientDbLoaded fun(self: AssetScript, args: wxl.ClientDbArgs)
AssetScript = {}

---@class NetScript
---@field OnMessageReceived fun(self: NetScript, args: wxl.MessageArgs): boolean|nil
---@field OnMessageSent fun(self: NetScript, args: wxl.MessageArgs): boolean|nil
---@field OnConnected fun(self: NetScript, args: wxl.ConnectionArgs)
---@field OnDisconnected fun(self: NetScript, args: wxl.ConnectionArgs)
NetScript = {}

---@class LuaScript
---@field OnScriptVmReady fun(self: LuaScript, args: wxl.ScriptVmArgs)
---@field OnScriptVmClosing fun(self: LuaScript, args: wxl.ScriptVmArgs)
---@field OnUiEvent fun(self: LuaScript, args: wxl.UiEventArgs)
---@field OnScriptError fun(self: LuaScript, args: wxl.ScriptErrorArgs)
LuaScript = {}

---@class CombatScript
---@field OnCombatAttackStart fun(self: CombatScript, args: wxl.CombatAttackArgs)
---@field OnCombatAttackStop fun(self: CombatScript, args: wxl.CombatAttackArgs)
---@field OnCombatDamage fun(self: CombatScript, args: wxl.CombatDamageArgs)
---@field OnCombatHeal fun(self: CombatScript, args: wxl.CombatHealArgs)
---@field OnCombatMiss fun(self: CombatScript, args: wxl.CombatMissArgs)
CombatScript = {}

---@alias wxl.Family "WorldScript"|"RenderScript"|"ObjectScript"|"AssetScript"|"NetScript"|"LuaScript"|"CombatScript"

---Calls `handler(args)` whenever the event is raised. Returns the handler's id.
---@overload fun(family: "WorldScript", method: "OnUpdate", handler: fun(args: wxl.UpdateArgs)): integer
---@overload fun(family: "WorldScript", method: "OnWorldEnter", handler: fun(args: wxl.WorldArgs)): integer
---@overload fun(family: "WorldScript", method: "OnWorldLeave", handler: fun(args: wxl.WorldArgs)): integer
---@overload fun(family: "WorldScript", method: "OnWorldClick", handler: fun(args: wxl.WorldClickArgs)): integer
---@overload fun(family: "WorldScript", method: "OnInput", handler: fun(args: wxl.InputArgs)): integer
---@overload fun(family: "WorldScript", method: "OnSoundPlay", handler: fun(args: wxl.SoundArgs)): integer
---@overload fun(family: "RenderScript", method: "OnFrameBegin", handler: fun(args: wxl.FrameArgs)): integer
---@overload fun(family: "RenderScript", method: "OnFrameEnd", handler: fun(args: wxl.FrameArgs)): integer
---@overload fun(family: "RenderScript", method: "OnWorldRenderBegin", handler: fun(args: wxl.FrameArgs)): integer
---@overload fun(family: "RenderScript", method: "OnWorldRenderEnd", handler: fun(args: wxl.FrameArgs)): integer
---@overload fun(family: "RenderScript", method: "OnUiRenderEnd", handler: fun(args: wxl.FrameArgs)): integer
---@overload fun(family: "RenderScript", method: "OnSwapchainLost", handler: fun(args: wxl.SwapchainArgs)): integer
---@overload fun(family: "RenderScript", method: "OnSwapchainRestored", handler: fun(args: wxl.SwapchainArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectCreate", handler: fun(args: wxl.ObjectArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectUpdate", handler: fun(args: wxl.ObjectArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectDestroy", handler: fun(args: wxl.ObjectArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnTargetChanged", handler: fun(args: wxl.TargetArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnDoodadSpawn", handler: fun(args: wxl.DoodadArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnEquipmentChanged", handler: fun(args: wxl.EquipmentArgs)): integer
---@overload fun(family: "AssetScript", method: "OnModelLoad", handler: fun(args: wxl.ModelLoadArgs)): integer
---@overload fun(family: "AssetScript", method: "OnTextureLoad", handler: fun(args: wxl.TextureLoadArgs)): integer
---@overload fun(family: "AssetScript", method: "OnTerrainChunkBuild", handler: fun(args: wxl.TerrainChunkArgs)): integer
---@overload fun(family: "AssetScript", method: "OnMapObjectLoad", handler: fun(args: wxl.MapObjectArgs)): integer
---@overload fun(family: "AssetScript", method: "OnFileOpen", handler: fun(args: wxl.FileOpenArgs)): integer
---@overload fun(family: "AssetScript", method: "OnFileOpened", handler: fun(args: wxl.FileOpenedArgs)): integer
---@overload fun(family: "AssetScript", method: "OnClientDbLoaded", handler: fun(args: wxl.ClientDbArgs)): integer
---@overload fun(family: "NetScript", method: "OnMessageReceived", handler: fun(args: wxl.MessageArgs)): integer
---@overload fun(family: "NetScript", method: "OnMessageSent", handler: fun(args: wxl.MessageArgs)): integer
---@overload fun(family: "NetScript", method: "OnConnected", handler: fun(args: wxl.ConnectionArgs)): integer
---@overload fun(family: "NetScript", method: "OnDisconnected", handler: fun(args: wxl.ConnectionArgs)): integer
---@overload fun(family: "LuaScript", method: "OnScriptVmReady", handler: fun(args: wxl.ScriptVmArgs)): integer
---@overload fun(family: "LuaScript", method: "OnScriptVmClosing", handler: fun(args: wxl.ScriptVmArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnUnitDeath", handler: fun(args: wxl.UnitArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnUnitAuraChanged", handler: fun(args: wxl.UnitAuraArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastStart", handler: fun(args: wxl.SpellCastArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastGo", handler: fun(args: wxl.SpellCastArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastFailed", handler: fun(args: wxl.SpellFailArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastInterrupted", handler: fun(args: wxl.SpellFailArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastRequest", handler: fun(args: wxl.SpellRequestArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnItemUseRequest", handler: fun(args: wxl.ItemUseArgs)): integer
---@overload fun(family: "LuaScript", method: "OnUiEvent", handler: fun(args: wxl.UiEventArgs)): integer
---@overload fun(family: "LuaScript", method: "OnScriptError", handler: fun(args: wxl.ScriptErrorArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellChannelStart", handler: fun(args: wxl.SpellProgressArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellChannelUpdate", handler: fun(args: wxl.SpellProgressArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellChannelStop", handler: fun(args: wxl.SpellProgressArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastResume", handler: fun(args: wxl.SpellProgressArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastDelayed", handler: fun(args: wxl.SpellDelayArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCooldown", handler: fun(args: wxl.SpellCooldownArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCooldownChanged", handler: fun(args: wxl.SpellCooldownChangeArgs)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastCancel", handler: fun(args: wxl.SpellCancelArgs)): integer
---@overload fun(family: "CombatScript", method: "OnCombatAttackStart", handler: fun(args: wxl.CombatAttackArgs)): integer
---@overload fun(family: "CombatScript", method: "OnCombatAttackStop", handler: fun(args: wxl.CombatAttackArgs)): integer
---@overload fun(family: "CombatScript", method: "OnCombatDamage", handler: fun(args: wxl.CombatDamageArgs)): integer
---@overload fun(family: "CombatScript", method: "OnCombatHeal", handler: fun(args: wxl.CombatHealArgs)): integer
---@overload fun(family: "CombatScript", method: "OnCombatMiss", handler: fun(args: wxl.CombatMissArgs)): integer
---@param family wxl.Family
---@param method string
---@param handler function
---@return integer id
function Observe(family, method, handler) end

---Like Observe, on an event that can be answered: returning true writes the answer,
---returning a string fills the buffer. Only on the main thread.
---@overload fun(family: "WorldScript", method: "OnInput", handler: fun(args: wxl.InputArgs): boolean|nil): integer
---@overload fun(family: "WorldScript", method: "OnSoundPlay", handler: fun(args: wxl.SoundArgs): boolean|nil): integer
---@overload fun(family: "AssetScript", method: "OnFileOpen", handler: fun(args: wxl.FileOpenArgs): string|nil): integer
---@overload fun(family: "NetScript", method: "OnMessageReceived", handler: fun(args: wxl.MessageArgs): boolean|nil): integer
---@overload fun(family: "NetScript", method: "OnMessageSent", handler: fun(args: wxl.MessageArgs): boolean|nil): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastRequest", handler: fun(args: wxl.SpellRequestArgs): boolean|nil): integer
---@overload fun(family: "ObjectScript", method: "OnItemUseRequest", handler: fun(args: wxl.ItemUseArgs): boolean|nil): integer
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
---@field abi_major integer
---@field abi_minor integer

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

---Every member of an event's args, as a plain table.
---@param args any
---@return table
function wxl.totable(args) end
