// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The event enum and the argument structs the table in events.def binds to.

#ifndef WXL_EVENTS_H
#define WXL_EVENTS_H

#include "wxl/abi.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Cancellation --------------------------------------------------------------------------
//
// An args struct reaches a handler as `const`, so an event a handler can act on carries a pointer
// to a flag the runtime owns and reads back. Writing WXL_TRUE through it is a request, honoured
// unless another handler already decided otherwise -- a handler reads before it writes.

/// A frame's identity and length. Shared by every slot in the frame.
typedef struct wxl_frame_args
{
    uint64_t index; ///< monotonic frame counter since startup
    float    delta; ///< seconds since the previous frame
} wxl_frame_args;

/// The logic tick.
typedef struct wxl_update_args
{
    float delta; ///< seconds since the previous tick
} wxl_update_args;

/// Entering or leaving a map.
typedef struct wxl_world_args
{
    uint32_t map_id;
    wxl_vec3 position; ///< where the player is, or was
} wxl_world_args;

/// A click that landed in the world.
typedef struct wxl_world_click_args
{
    wxl_vec3   point;   ///< where the ray met the world
    wxl_handle object;  ///< what was hit, or WXL_HANDLE_NONE for bare ground
    uint32_t   button;  ///< 0 left, 1 right, 2 middle
} wxl_world_click_args;

/// A platform input message, before the client sees it.
typedef struct wxl_input_args
{
    uint32_t  message; ///< platform message id
    uint64_t  wparam;
    int64_t   lparam;
    wxl_bool* handled; ///< write WXL_TRUE to keep the client from seeing it
} wxl_input_args;

/// A sound about to play.
typedef struct wxl_sound_args
{
    uint32_t  sound_id;   ///< client sound id, 0 when the sound was named by path
    wxl_str   path;       ///< source path, empty when the sound came from an id
    wxl_bool* suppressed; ///< write WXL_TRUE to drop it
} wxl_sound_args;

/// Swapchain resources going away and coming back.
typedef struct wxl_swapchain_args
{
    uint32_t width;
    uint32_t height;
} wxl_swapchain_args;

/// The class of a world object, as the server classifies it.
typedef enum wxl_object_type
{
    WXL_OBJECT_UNKNOWN    = 0,
    WXL_OBJECT_ITEM       = 1,
    WXL_OBJECT_CONTAINER  = 2,
    WXL_OBJECT_UNIT       = 3,
    WXL_OBJECT_PLAYER     = 4,
    WXL_OBJECT_GAMEOBJECT = 5,
    WXL_OBJECT_DYNAMIC    = 6,
    WXL_OBJECT_CORPSE     = 7
} wxl_object_type;

/// An object appearing, changing or leaving.
typedef struct wxl_object_args
{
    wxl_handle object;
    wxl_guid   guid;
    uint32_t   type; ///< a wxl_object_type value
} wxl_object_args;

/// The player's target moving from one object to another.
typedef struct wxl_target_args
{
    wxl_handle previous; ///< WXL_HANDLE_NONE when nothing was targeted
    wxl_handle current;  ///< WXL_HANDLE_NONE when the target was cleared
} wxl_target_args;

/// A doodad placed by the map.
typedef struct wxl_doodad_args
{
    wxl_handle doodad;
    wxl_str    path;     ///< the model it was built from
    wxl_vec3   position;
} wxl_doodad_args;

/// A unit's visible equipment changing.
typedef struct wxl_equipment_args
{
    wxl_handle unit;
    uint32_t   slot;
    uint32_t   item_id; ///< 0 when the slot was cleared
} wxl_equipment_args;

/// A model's bytes, before the parse.
typedef struct wxl_model_load_args
{
    wxl_str      path;
    const void*  data; ///< the bytes as read; valid for the call only
    uint32_t     size;
    const void** replacement;      ///< write a buffer here to parse it instead of @ref data
    uint32_t*    replacement_size; ///< its length; the writer keeps it alive until the next load
} wxl_model_load_args;

/// A texture requested by name.
typedef struct wxl_texture_load_args
{
    wxl_str path;
} wxl_texture_load_args;

/// A terrain chunk being built.
typedef struct wxl_terrain_chunk_args
{
    uint32_t map_id;
    int32_t  tile_x, tile_y;   ///< the tile the chunk belongs to
    int32_t  chunk_x, chunk_y; ///< the chunk inside that tile
} wxl_terrain_chunk_args;

/// A map object's root, before its sub-chunks are walked.
typedef struct wxl_map_object_args
{
    wxl_str     path;
    const void* data; ///< the root bytes; valid for the call only
    uint32_t    size;
} wxl_map_object_args;

/**
 * @brief A file about to be opened by name.
 *
 * Writing a NUL-terminated path into @ref redirect opens that instead. This is how a mod replaces
 * an asset without touching an archive. The last handler to write wins, so a handler that cares
 * reads @ref redirect first to see whether another already answered.
 */
typedef struct wxl_file_open_args
{
    wxl_str  path;              ///< what was asked for
    char*    redirect;          ///< empty on entry; write a path here to answer with another file
    uint32_t redirect_capacity; ///< bytes available in @ref redirect, terminator included
} wxl_file_open_args;

/// The result of an open.
typedef struct wxl_file_opened_args
{
    wxl_str  path;  ///< the path actually opened, after any redirect
    wxl_bool found;
    uint32_t size;  ///< bytes, 0 when not found
} wxl_file_opened_args;

/**
 * @brief A client database that finished loading.
 *
 * The records are the file's own rows, a flat array of @ref record_size bytes each -- a file
 * format, not a client structure, so reading them is portable. They stay valid until the database
 * is unloaded.
 */
typedef struct wxl_client_db_args
{
    wxl_str     name;         ///< the file it came from, e.g. "Spell.dbc"
    const void* records;
    uint32_t    record_count;
    uint32_t    record_size;
} wxl_client_db_args;

/**
 * @brief A message crossing to or from the server.
 *
 * @ref data is the wire body, positioned after the opcode and before any client parsing. It is
 * valid for the call only. Suppressing an incoming message hides it from the client entirely,
 * which is how a mod serves an opcode the client does not know; suppressing an outgoing one stops
 * it being sent.
 */
typedef struct wxl_message_args
{
    uint32_t    opcode;
    const void* data;
    uint32_t    size;
    wxl_bool*   suppressed; ///< write WXL_TRUE to stop it going any further
} wxl_message_args;

/// The connection to the world server coming up or going away.
typedef struct wxl_connection_args
{
    wxl_str address; ///< what was connected to; empty when the runtime does not expose it
} wxl_connection_args;

/// Which script context the client is running.
typedef enum wxl_script_context
{
    WXL_SCRIPT_CONTEXT_GLUE = 0, ///< the login and character-select screens
    WXL_SCRIPT_CONTEXT_GAME = 1  ///< in the world
} wxl_script_context;

/**
 * @brief A script context coming up or going down.
 *
 * Both contexts are created and destroyed more than once in a session, so anything registered on
 * one has to be registered again on the next.
 *
 * @ref native_state is the VM's own state pointer, for a mod that wants to talk to the VM
 * directly. Using it ties the mod to that VM and to that runtime; the script service is the
 * portable way to register a function.
 */
typedef struct wxl_script_vm_args
{
    uint32_t context;      ///< a wxl_script_context value
    void*    native_state; ///< the VM's state, or NULL when the runtime does not expose one
} wxl_script_vm_args;

/// Something that happened to a unit.
typedef struct wxl_unit_args
{
    wxl_handle unit;
} wxl_unit_args;

/// An aura appearing on or leaving a unit.
typedef struct wxl_unit_aura_args
{
    wxl_handle unit;
    uint32_t   spell_id;
    wxl_bool   removed; ///< WXL_TRUE when the aura left, WXL_FALSE when it was applied
} wxl_unit_aura_args;

/**
 * @brief A cast a unit in view started (SpellCastStart) or released (SpellCastGo).
 *
 * Read off the server's message before the client parses it. A handle resolves only for an
 * object the client holds; the guids are what the server sent whether or not it does.
 */
typedef struct wxl_spell_cast_args
{
    wxl_handle caster;       ///< WXL_HANDLE_NONE when the caster is not in view
    wxl_guid   caster_guid;
    wxl_handle target;       ///< the object target; WXL_HANDLE_NONE when none, or not in view
    wxl_guid   target_guid;  ///< 0 when the cast has no object target
    wxl_guid   cast_item;    ///< the item the cast came from, 0 when none
    uint32_t   spell_id;
    uint32_t   cast_count;   ///< the caster's count of the cast, as the server echoes it
    uint32_t   cast_flags;   ///< the server's cast flags
    uint32_t   target_flags; ///< the server's target flags
    uint32_t   cast_time;    ///< milliseconds; SpellCastStart only, 0 on SpellCastGo
    uint32_t   hit_count;    ///< units the cast hit; SpellCastGo only, 0 on SpellCastStart
    uint32_t   miss_count;   ///< units it missed; SpellCastGo only, 0 on SpellCastStart
} wxl_spell_cast_args;

/**
 * @brief A cast that did not go through.
 *
 * SpellCastFailed is the local player's own cast, refused by the server (SMSG_CAST_FAILED) or
 * by the client before it asked; SpellCastInterrupted is a cast in progress that the server
 * stopped (SMSG_SPELL_FAILURE), for any caster in view. @ref reason is the client's
 * SPELL_FAILED_* code in both.
 */
typedef struct wxl_spell_fail_args
{
    wxl_handle caster;       ///< WXL_HANDLE_NONE when the caster is not in view
    wxl_guid   caster_guid;
    uint32_t   spell_id;
    uint32_t   cast_count;
    uint32_t   reason;
    int32_t    custom_error; ///< the error sent with SPELL_FAILED_CUSTOM_ERROR, -1 for none
    int32_t    custom_arg;   ///< its argument, -1 for none
} wxl_spell_fail_args;

/**
 * @brief The local player asking to cast a spell, before the client checks or sends anything.
 *
 * Writing WXL_TRUE through @ref refused answers the client as a cast it could not make, and
 * nothing is sent. Raised at every ask, including the client's own second ask once a target
 * has been picked or a dialog confirmed, which @ref confirmed tells apart.
 */
typedef struct wxl_spell_request_args
{
    wxl_handle caster;      ///< the player, or the pet or charmed unit it casts through
    uint32_t   spell_id;
    wxl_handle item;        ///< the item the cast comes from, WXL_HANDLE_NONE for none
    wxl_guid   target_guid; ///< 0 when the client is to pick the target
    wxl_bool   confirmed;   ///< WXL_TRUE on the client's own second ask
    wxl_bool*  refused;     ///< write WXL_TRUE to keep the client from casting
} wxl_spell_request_args;

/**
 * @brief The local player asking to use an item, before the client checks or sends anything.
 *
 * Writing WXL_TRUE through @ref refused answers the client as an item it could not use.
 */
typedef struct wxl_item_use_args
{
    wxl_handle item;
    wxl_guid   item_guid;
    wxl_guid   target_guid; ///< 0 when the use has no target
    wxl_bool   confirmed;   ///< WXL_TRUE on the client's own second ask, after a dialog
    wxl_bool*  refused;     ///< write WXL_TRUE to keep the client from using it
} wxl_item_use_args;

/**
 * @brief An event the client fires to its interface, with the arguments it passes along.
 *
 * The client formats its arguments from a C format string ("%d%s", ...) and pushes each as a
 * Lua value. They cross here already rendered, because a va_list has no layout a mod or a
 * script runtime could read: @ref values holds one entry per argument, back to back, each a
 * kind byte followed by the text and a NUL. The kind is the format letter the client used --
 * 'd' and 'u' an integer in decimal, 'f' a double printed with %.17g, 'b' a boolean as 1 or 0,
 * 's' a string as it is -- or 'n' for a string argument the client passed as null, which it
 * pushes as nil. Integers and doubles round-trip exactly; a runtime that wants typed values
 * parses them back by kind. Valid for the call only.
 */
typedef struct wxl_ui_event_args
{
    uint32_t    index;       ///< the event's index in the client's table
    wxl_str     name;        ///< the event's name, "PLAYER_ENTERING_WORLD"
    wxl_str     format;      ///< the client's format string; empty for an event without arguments
    const void* values;      ///< the entries described above, NULL when there are none
    uint32_t    values_size; ///< bytes in @ref values
    uint32_t    value_count; ///< entries in @ref values
} wxl_ui_event_args;

/// An error the interface's Lua raised, as the VM reports it, before the client's handler sees it.
typedef struct wxl_script_error_args
{
    uint32_t context; ///< a wxl_script_context value: which interface raised it
    wxl_str  message; ///< the error value as text; "UNKNOWN ERROR" when it is not text
} wxl_script_error_args;

// --- The enum ------------------------------------------------------------------------------

/// Event ids. The numbers are the ABI; see events.def.
typedef enum wxl_event
{
#define WXL_EVENT(family, name, id, args) WXL_EV_##name = id,
#include "wxl/events.def"
#undef WXL_EVENT
    WXL_EV_COUNT
} wxl_event;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_EVENTS_H
