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

// --- The argument structs ----------------------------------------------------------------
//
// Declared by fields.def, which also describes each member for a runtime that is not C, so the
// struct and its description cannot disagree. The rows and their comments are there.

#define WXL_ARGS_TYPE_u32    uint32_t
#define WXL_ARGS_TYPE_i32    int32_t
#define WXL_ARGS_TYPE_u64    uint64_t
#define WXL_ARGS_TYPE_i64    int64_t
#define WXL_ARGS_TYPE_f32    float
#define WXL_ARGS_TYPE_bool   wxl_bool
#define WXL_ARGS_TYPE_handle wxl_handle
#define WXL_ARGS_TYPE_guid   wxl_guid
#define WXL_ARGS_TYPE_vec3   wxl_vec3
#define WXL_ARGS_TYPE_str    wxl_str

#define WXL_ARGS(args)                          typedef struct args {
#define WXL_ARGS_END(args)                      } args;
#define WXL_ARGS_FIELD(args, member, kind)      WXL_ARGS_TYPE_##kind member;
#define WXL_ARGS_BYTES(args, member, size)      const void* member;
#define WXL_ARGS_ANSWER(args, member)           wxl_bool* member;
#define WXL_ARGS_BUFFER(args, member, capacity) char* member;
#define WXL_ARGS_PTR(args, member, type)        type member;
#include "wxl/fields.def"
#undef WXL_ARGS_PTR
#undef WXL_ARGS_BUFFER
#undef WXL_ARGS_ANSWER
#undef WXL_ARGS_BYTES
#undef WXL_ARGS_FIELD
#undef WXL_ARGS_END
#undef WXL_ARGS

#undef WXL_ARGS_TYPE_str
#undef WXL_ARGS_TYPE_vec3
#undef WXL_ARGS_TYPE_guid
#undef WXL_ARGS_TYPE_handle
#undef WXL_ARGS_TYPE_bool
#undef WXL_ARGS_TYPE_f32
#undef WXL_ARGS_TYPE_i64
#undef WXL_ARGS_TYPE_u64
#undef WXL_ARGS_TYPE_i32
#undef WXL_ARGS_TYPE_u32

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

/// Which script context the client is running.
typedef enum wxl_script_context
{
    WXL_SCRIPT_CONTEXT_GLUE = 0, ///< the login and character-select screens
    WXL_SCRIPT_CONTEXT_GAME = 1  ///< in the world
} wxl_script_context;

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
