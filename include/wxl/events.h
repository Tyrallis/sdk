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
#define WXL_ARGS_ENUM(args, member, type)       uint32_t member;
#include "wxl/fields.def"
#undef WXL_ARGS_ENUM
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

// --- The enumerations ----------------------------------------------------------------------
//
// What an args member declared with WXL_ARGS_ENUM holds, declared by enums.def: the values, their
// meanings and the names a script sees are there. The member itself is a uint32_t.

#define WXL_ENUM(type)                              typedef enum type {
#define WXL_ENUM_VALUE(type, constant, value, name) constant = value,
#define WXL_ENUM_END(type)                          } type;
#include "wxl/enums.def"
#undef WXL_ENUM_END
#undef WXL_ENUM_VALUE
#undef WXL_ENUM

// --- The event ids -------------------------------------------------------------------------

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
