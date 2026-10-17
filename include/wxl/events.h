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
