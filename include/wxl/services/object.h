// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The object service: reading what lives in the world.
//
// Everything is addressed by handle and read through a call. No struct layout crosses, which is
// what lets the same mod read a unit on a client that keeps it at a fixed offset and on one that
// holds a real object.

#ifndef WXL_SERVICE_OBJECT_H
#define WXL_SERVICE_OBJECT_H

#include "wxl/abi.h"
#include "wxl/events.h"

#define WXL_OBJECT_SERVICE         "wxl.object"
#define WXL_OBJECT_SERVICE_VERSION 1

/// write_fields' flags: none is defined yet, so 0 is the only value a runtime accepts.
#define WXL_FIELD_WRITE_PLAIN 0u

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reading world objects.
 *
 * A getter writes its out-parameter and returns WXL_OK, or leaves it untouched and returns
 * WXL_E_NOT_FOUND for a handle that no longer resolves, or WXL_E_INVALID for a handle of the
 * wrong kind -- asking a game object for its health is a mistake, not a missing value.
 *
 * A handle names one visibility episode of an object: from the ObjectCreate that announces it to
 * the ObjectDestroy that sees it leave. An object that leaves view and comes back -- even the same
 * client object, revived before the client freed it -- is announced again with a new handle, and
 * the old one stays stale. The guid is what lasts across episodes.
 *
 * Main thread only: the objects behind the handles belong to the client's object manager, which
 * lives on the thread that runs the game. A read from another thread is a data race.
 */
typedef struct wxl_object_api
{
    uint32_t struct_size;

    // --- identity ---------------------------------------------------------------------------

    /// Whether the handle still resolves. A stale handle answers WXL_FALSE; it never misreads.
    wxl_bool(WXL_CALL* valid)(wxl_handle object);

    /// The object's server identity, or WXL_GUID_NONE for something the server does not own.
    wxl_guid(WXL_CALL* guid)(wxl_handle object);

    /// A wxl_object_type value, or WXL_OBJECT_UNKNOWN.
    uint32_t(WXL_CALL* type)(wxl_handle object);

    /// The handle for a guid, or WXL_HANDLE_NONE when that object is not in view. Any object the
    /// client holds resolves, announced or not.
    wxl_handle(WXL_CALL* from_guid)(wxl_guid guid);

    /// The handle of the object the local player controls.
    wxl_handle(WXL_CALL* local_player)(void);

    // --- any object -------------------------------------------------------------------------

    wxl_result(WXL_CALL* position)(wxl_handle object, wxl_vec3* out);
    wxl_result(WXL_CALL* facing)(wxl_handle object, float* out_radians);

    /// The display name. The string points into runtime storage and is valid until the next call.
    wxl_result(WXL_CALL* name)(wxl_handle object, wxl_str* out);

    // --- units ------------------------------------------------------------------------------

    /// Signed, as the client holds UNIT_FIELD_HEALTH and tests it (`<= 0` is dead).
    wxl_result(WXL_CALL* unit_health)(wxl_handle unit, int32_t* out);
    wxl_result(WXL_CALL* unit_max_health)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_level)(wxl_handle unit, uint32_t* out);

    /// What the unit is targeting, or WXL_HANDLE_NONE.
    wxl_result(WXL_CALL* unit_target)(wxl_handle unit, wxl_handle* out);

    // --- in bulk ----------------------------------------------------------------------------
    //
    // One call per object is fine for a hundred and wasteful for ten thousand. These exist so a
    // per-frame pass over everything in view does not have to reach past the contract.

    /**
     * @brief Lists the objects of a type currently in view.
     * @param type  a wxl_object_type value, or WXL_OBJECT_UNKNOWN for every object.
     * @param out   receives up to @p capacity handles; may be NULL to count only.
     * @return how many there are, which may exceed @p capacity.
     */
    uint32_t(WXL_CALL* enumerate)(uint32_t type, wxl_handle* out, uint32_t capacity);

    /// Positions for many handles at once. Writes WXL_HANDLE_NONE entries as the origin.
    wxl_result(WXL_CALL* positions)(const wxl_handle* objects, uint32_t count, wxl_vec3* out);

    // --- appended in 0.1.2: the client's own object taxonomy----------------------------------
    //
    // The client knows eight kinds of object (wxl_object_type), and each kind carries its own
    // descriptor block. One getter per field a mod can use, gated like the unit getters above:
    // WXL_E_INVALID for a handle of another kind. A table from a runtime older than this header
    // ends before these members; the typed facades read struct_size and answer WXL_E_UNSUPPORTED.
    //
    // A field that names another object is handed over as a guid, not a handle: the object it
    // names may be out of view, and from_guid resolves it when it is not.

    // --- any object, continued --------------------------------------------------------------

    /// The server's template id: a creature, item or game-object entry; 0 for a player.
    wxl_result(WXL_CALL* entry)(wxl_handle object, uint32_t* out);
    wxl_result(WXL_CALL* scale)(wxl_handle object, float* out);

    // --- units, continued -------------------------------------------------------------------

    /// Power by index (0 mana, 1 rage, 2 focus, 3 energy, 4 happiness, 5 runes, 6 runic power).
    wxl_result(WXL_CALL* unit_power)(wxl_handle unit, uint32_t index, uint32_t* out);
    wxl_result(WXL_CALL* unit_max_power)(wxl_handle unit, uint32_t index, uint32_t* out);
    /// The index of the power the unit uses, as unit_power takes it.
    wxl_result(WXL_CALL* unit_power_type)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_race)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_class)(wxl_handle unit, uint32_t* out);
    /// 0 male, 1 female, 2 none.
    wxl_result(WXL_CALL* unit_sex)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_display_id)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_native_display_id)(wxl_handle unit, uint32_t* out);
    /// 0 while not mounted.
    wxl_result(WXL_CALL* unit_mount_display_id)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_faction_template)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_flags)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_flags2)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_dynamic_flags)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_npc_flags)(wxl_handle unit, uint32_t* out);
    wxl_result(WXL_CALL* unit_bounding_radius)(wxl_handle unit, float* out);
    wxl_result(WXL_CALL* unit_combat_reach)(wxl_handle unit, float* out);
    /// Who the unit charms, summons, is charmed by, was summoned by, was created by; none as 0.
    wxl_result(WXL_CALL* unit_charm)(wxl_handle unit, wxl_guid* out);
    wxl_result(WXL_CALL* unit_summon)(wxl_handle unit, wxl_guid* out);
    wxl_result(WXL_CALL* unit_charmed_by)(wxl_handle unit, wxl_guid* out);
    wxl_result(WXL_CALL* unit_summoned_by)(wxl_handle unit, wxl_guid* out);
    wxl_result(WXL_CALL* unit_created_by)(wxl_handle unit, wxl_guid* out);

    // --- players ----------------------------------------------------------------------------

    wxl_result(WXL_CALL* player_flags)(wxl_handle player, uint32_t* out);
    /// 0 while unguilded.
    wxl_result(WXL_CALL* player_guild_id)(wxl_handle player, uint32_t* out);
    wxl_result(WXL_CALL* player_guild_rank)(wxl_handle player, uint32_t* out);
    wxl_result(WXL_CALL* player_chosen_title)(wxl_handle player, uint32_t* out);
    /// The item shown in an equipment slot (0 head .. 18 tabard); 0 when the slot is empty. The
    /// client keeps a disarmed weapon's id negated in PLAYER_VISIBLE_ITEM_n_ENTRYID; this answers
    /// the item, without the sign, which the field itself still carries.
    wxl_result(WXL_CALL* player_visible_item)(wxl_handle player, uint32_t slot, uint32_t* out);

    // --- items, containers included ---------------------------------------------------------

    wxl_result(WXL_CALL* item_owner)(wxl_handle item, wxl_guid* out);
    /// The bag holding the item, or 0 when it sits in the backpack or is equipped.
    wxl_result(WXL_CALL* item_contained_in)(wxl_handle item, wxl_guid* out);
    wxl_result(WXL_CALL* item_creator)(wxl_handle item, wxl_guid* out);
    /// Signed, as the client holds ITEM_FIELD_STACK_COUNT.
    wxl_result(WXL_CALL* item_stack_count)(wxl_handle item, int32_t* out);
    /// Seconds left before the item expires; 0 when it does not.
    wxl_result(WXL_CALL* item_duration)(wxl_handle item, uint32_t* out);
    wxl_result(WXL_CALL* item_flags)(wxl_handle item, uint32_t* out);
    wxl_result(WXL_CALL* item_durability)(wxl_handle item, uint32_t* out);
    wxl_result(WXL_CALL* item_max_durability)(wxl_handle item, uint32_t* out);

    // --- containers -------------------------------------------------------------------------

    wxl_result(WXL_CALL* container_num_slots)(wxl_handle container, uint32_t* out);
    /// The item in a slot, or 0 for an empty one; WXL_E_INVALID past the container's slots.
    wxl_result(WXL_CALL* container_slot)(wxl_handle container, uint32_t index, wxl_guid* out);

    // --- game objects -----------------------------------------------------------------------

    wxl_result(WXL_CALL* gameobject_created_by)(wxl_handle object, wxl_guid* out);
    wxl_result(WXL_CALL* gameobject_display_id)(wxl_handle object, uint32_t* out);
    wxl_result(WXL_CALL* gameobject_flags)(wxl_handle object, uint32_t* out);
    wxl_result(WXL_CALL* gameobject_dynamic_flags)(wxl_handle object, uint32_t* out);
    wxl_result(WXL_CALL* gameobject_faction)(wxl_handle object, uint32_t* out);
    wxl_result(WXL_CALL* gameobject_level)(wxl_handle object, uint32_t* out);
    /// 0 active, 1 ready, 2 destroyed, as the client's GAMEOBJECT_BYTES_1 state byte.
    wxl_result(WXL_CALL* gameobject_state)(wxl_handle object, uint32_t* out);
    /// The game-object type: door, button, chest, ... as the server's GameObjectType numbering.
    wxl_result(WXL_CALL* gameobject_type)(wxl_handle object, uint32_t* out);
    wxl_result(WXL_CALL* gameobject_art_kit)(wxl_handle object, uint32_t* out);

    // --- dynamic objects (area effects) -----------------------------------------------------

    wxl_result(WXL_CALL* dynamic_caster)(wxl_handle object, wxl_guid* out);
    wxl_result(WXL_CALL* dynamic_spell_id)(wxl_handle object, uint32_t* out);
    wxl_result(WXL_CALL* dynamic_radius)(wxl_handle object, float* out);
    wxl_result(WXL_CALL* dynamic_cast_time)(wxl_handle object, uint32_t* out);

    // --- corpses ----------------------------------------------------------------------------

    wxl_result(WXL_CALL* corpse_owner)(wxl_handle corpse, wxl_guid* out);
    wxl_result(WXL_CALL* corpse_party)(wxl_handle corpse, wxl_guid* out);
    wxl_result(WXL_CALL* corpse_display_id)(wxl_handle corpse, uint32_t* out);
    wxl_result(WXL_CALL* corpse_guild)(wxl_handle corpse, uint32_t* out);
    wxl_result(WXL_CALL* corpse_flags)(wxl_handle corpse, uint32_t* out);
    wxl_result(WXL_CALL* corpse_dynamic_flags)(wxl_handle corpse, uint32_t* out);

    // --- appended in 0.1.9: the descriptor fields, by dword index ---------------------------
    //
    // Every object carries a block of dwords the server fills through update packets: its
    // descriptor fields, OBJECT_FIELD_GUID at dword 0 and each kind's own fields after its
    // parent's. wxl/update_fields.def names every one of them with its dword index, its size and
    // what it holds; the getters above read a few of them by name, these reach all of them by
    // index. A table from a runtime older than this header ends before these members: the typed
    // facades read struct_size and answer WXL_E_UNSUPPORTED.
    //
    // Each of them refuses what it cannot do with WXL_E_INVALID -- a range past the object's
    // block, a null pointer, a flag it does not know -- and says why in the runtime's log
    // ("write_fields: dword 0x95 is past a Unit's 0x94").

    /**
     * @brief The size of the object's descriptor block, in dwords; 0 for a handle that does not
     *        resolve.
     *
     * Each kind's end in update_fields.def (a Unit's 0x94), except for a player other than the
     * active one: the client keeps the fields past PLAYER_FIELD_PAD_0 -- inventory, skills,
     * money, experience -- for the player it controls only, so another player's block ends at
     * 0x144.
     */
    uint32_t(WXL_CALL* field_count)(wxl_handle object);

    /// Dwords [first, first + count) of the object's block into @p out, as the client holds them.
    wxl_result(WXL_CALL* read_fields)(wxl_handle object, uint32_t first, uint32_t count, uint32_t* out);

    /**
     * @brief The same dwords of many objects at once.
     *
     * Writes @p dwords dwords from @p field for each of @p count objects, one after the other,
     * into @p out, which holds count * dwords. An object that does not answer -- a stale handle,
     * a block too short for the range -- is written as zeros, and @p results, when not null,
     * receives each object's result in order: what read_fields would have answered for it.
     * Returns WXL_OK when the call itself was well formed, whatever each object answered.
     */
    wxl_result(WXL_CALL* read_field_many)(const wxl_handle* objects, uint32_t count, uint32_t field,
                                          uint32_t dwords, uint32_t* out, wxl_result* results);

    /**
     * @brief Writes dwords [first, first + count) of the object's block in the client.
     *
     * A client-side write: the client's copy of the field holds the value from here on, and the
     * next update of the server that carries the field replaces it, as it replaces anything the
     * client holds. Nothing else happens -- no handler of the client runs, no model is rebuilt,
     * the server is not told, and no ObjectFieldChanged is raised -- so a field the client reads
     * each frame shows the value at once and one it reads on change shows it when it next looks.
     * OBJECT_FIELD_GUID and OBJECT_FIELD_TYPE, dwords 0 to 2, are the keys the client finds the
     * object by and are refused.
     *
     * @param flags  WXL_FIELD_WRITE_PLAIN. A flag this runtime does not know is refused, so that
     *               a later one -- a write the runtime keeps re-applying over the server's -- is
     *               never taken for a plain write.
     */
    wxl_result(WXL_CALL* write_fields)(wxl_handle object, uint32_t first, uint32_t count,
                                       const uint32_t* in, uint32_t flags);

    /**
     * @brief Asks for ObjectFieldChanged when a field of objects of a type changes.
     *
     * Nothing is raised for a field nobody asked for. From this call on, each update of the
     * server that changes an element of dwords [first, first + count) on an object of @p type --
     * a wxl_object_type, or WXL_OBJECT_UNKNOWN for every object -- raises one ObjectFieldChanged
     * per element that changed, with the element's old and new value, once the client has
     * applied the update: where the client runs its own reactions to a field's change. A field
     * of a parent kind is watched on the type asked for only: watching UNIT_FIELD_HEALTH for
     * WXL_OBJECT_PLAYER raises nothing for a creature.
     *
     * The range starts and ends on elements (a guid is one element of two dwords) of fields the
     * client sees change: update_fields.def's watch column. The client finds a change by
     * comparing an element with the copy it keeps from before the update, and keeps one only for
     * some fields; the others are refused with WXL_E_UNSUPPORTED and named in the log. The
     * player's fields past PLAYER_FIELD_PAD_0 (scope local) are seen on the active player only.
     *
     * A watch lasts as long as the runtime, across worlds; asking again for what is watched
     * changes nothing. Every mod subscribed to ObjectFieldChanged receives every watched change,
     * whoever asked for it.
     */
    wxl_result(WXL_CALL* watch_field)(uint32_t type, uint32_t first, uint32_t count);
} wxl_object_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_OBJECT_H
