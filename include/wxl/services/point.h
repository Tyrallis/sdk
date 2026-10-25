// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The point service: running code before, instead of or after a function of the engine.
//
// A point is a function of the client the runtime names and keeps working on every runtime it
// lists: the camera's update, the model cache's lookup, the world's render pass. A mod attaches
// handlers to it, on one of three sides:
//
//   Before   runs when the function is entered, and may rewrite its arguments.
//   Replace  runs instead of it. The function, and every Replace attached below this one, run
//            only if the handler calls call_original, with the arguments as it left them; the
//            handler answers in the function's place by writing *call->result.
//   After    runs when it has returned, and may rewrite its answer.
//
// The list of points is wxl/points.def, generated from the runtime's own list: each with its
// signature reduced to sizes and classes ("i32(ptr,u32)"), its policy (whether it can be
// replaced), the thread it is called on and the runtimes that have it. A function the 2008
// compiler inlined into its callers has no address in the stock client and is a point of the
// recompiled client only; find answers WXL_E_UNSUPPORTED for it there, and list says why.
//
// Every handler has one shape: it is handed the call, whose args[i] is the address of argument
// i (`self` first for a member function) and result the address of the answer (NULL for a
// function that answers nothing). The C++ facade (point.hpp) gives each point a typed handler.
//
// Order: lower priority runs first and closest to the caller -- first among Befores, outermost
// among Replaces, last among Afters, so that a pair of handlers of one mod nests; equal
// priorities keep the mods' load order, then attach order. A handler that faults is stopped
// there, its mod is disabled and the call goes on: a Replace that faulted before it called the
// original is followed by the original, one that faulted after it answers what the original did.
//
// Threads: a point of thread `main` is called on the client's main thread. A point of thread
// `any` may be called on any thread at once: its handlers must be thread-safe, and a script mod
// cannot attach to it.
//
// Cost: a point nobody is attached to is not hooked at all on the stock client and is one test
// of a byte on the recompiled one. Attached, a call costs a dispatch like an event's.

#ifndef WXL_SERVICE_POINT_H
#define WXL_SERVICE_POINT_H

#include "wxl/abi.h"

#define WXL_POINT_SERVICE         "wxl.point"
#define WXL_POINT_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/// A point: 1 + its row's position in wxl/points.def. Zero is none.
typedef uint32_t wxl_point;
#define WXL_POINT_NONE ((wxl_point)0)

/// A live attachment, for detaching. Zero means the attach did not take.
typedef uint32_t wxl_point_attachment;
#define WXL_POINT_ATTACHMENT_NONE ((wxl_point_attachment)0)

/// Where a handler runs.
#define WXL_POINT_BEFORE  0u
#define WXL_POINT_REPLACE 1u
#define WXL_POINT_AFTER   2u

/// The thread a point is called on.
#define WXL_POINT_THREAD_MAIN 0u ///< the client's main thread
#define WXL_POINT_THREAD_ANY  1u ///< any thread, several at once: C and C++ handlers only

/// What a runtime says of a point (wxl_point_info.flags).
#define WXL_POINT_ON_X86      0x01u ///< the stock client's runtime has it
#define WXL_POINT_ON_X64      0x02u ///< the recompiled client's runtime has it
#define WXL_POINT_HERE        0x04u ///< the runtime answering has it: find gives its id
#define WXL_POINT_REPLACEABLE 0x08u ///< it takes Replace handlers; without it, Before and After only
#define WXL_POINT_STABLE      0x10u ///< its signature never changes; without it, provisional

/// What a call says of itself (wxl_point_call.flags).
/// Reached at a copy of the function the compiler inlined into a caller: only Before handlers
/// run there, and the arguments are where that copy holds them.
#define WXL_POINT_CALL_INLINED 0x01u

/// The default priority.
#define WXL_POINT_PRIORITY_DEFAULT 0

/**
 * @brief One call of a point, as a handler is handed it. Valid for that handler's call only.
 */
typedef struct wxl_point_call
{
    uint32_t     struct_size;
    wxl_point    point;
    uint32_t     side;      ///< WXL_POINT_BEFORE, _REPLACE or _AFTER: the list running now
    uint32_t     depth;     ///< 1 for the outermost call of this point on this thread, 2 inside it...
    uint32_t     flags;     ///< WXL_POINT_CALL_*
    uint32_t     arg_count;
    void* const* args;      ///< args[i] is the address of argument i; writable in Before and Replace
    void*        result;    ///< the address of the answer, NULL for void; writable in Replace and After
    uintptr_t    runtime[3]; ///< the runtime's: where this call is in its chain; never touched by a mod
} wxl_point_call;

/// A handler: the call, and the pointer given at attach.
typedef void(WXL_CALL* wxl_point_fn)(wxl_point_call* call, void* user);

/// What a runtime says of a point it lists.
typedef struct wxl_point_info
{
    uint32_t struct_size;
    uint32_t point;       ///< its id here, WXL_POINT_NONE when this runtime does not have it
    wxl_str  name;        ///< "CGCamera::UpdateCallback"
    wxl_str  signature;   ///< lowered: "i32(ptr,ptr)"
    uint32_t thread;      ///< WXL_POINT_THREAD_*
    uint32_t flags;       ///< WXL_POINT_ON_X86, ...
    uint32_t layout_hash; ///< what attach compares a typed handler's with
    uint32_t arg_count;
    wxl_str  absent;      ///< why this runtime does not have it, "" when it does
} wxl_point_info;

typedef struct wxl_point_api
{
    uint32_t struct_size;

    /**
     * @brief Lists every point of the table, those this runtime does not have included.
     * @param out  receives up to @p capacity points, valid for the runtime's life; may be NULL to
     *             count only.
     * @return how many exist, which may exceed @p capacity.
     */
    uint32_t(WXL_CALL* list)(wxl_point_info* out, uint32_t capacity);

    /**
     * @brief The id of a point by name.
     * @return WXL_OK with *out set; WXL_E_UNSUPPORTED for a point of the table this runtime does
     *         not have (list says why: "x64 only", folded, too short); WXL_E_NOT_FOUND for a name
     *         the table does not hold.
     */
    wxl_result(WXL_CALL* find)(const char* name, uint32_t* out);

    /**
     * @brief Attaches a handler to a point, on one side.
     *
     * The first handler on a point hooks it and the last one's detach unhooks it. Logged and
     * refused, with WXL_POINT_ATTACHMENT_NONE: an unknown point or side, a point this runtime
     * does not have, a Replace on a point that cannot be replaced, a @p layout_hash other than
     * the point's (the mod was built against another signature of it: rebuild it), and on the
     * stock client a function whose first bytes cannot be moved aside for the detour.
     * @param layout_hash  the point's layout hash as the mod's headers have it, 0 to skip the check.
     * @return the attachment, the calling mod's: detached when the mod unloads or is disabled.
     */
    wxl_point_attachment(WXL_CALL* attach)(uint32_t point, uint32_t side, wxl_point_fn handler,
                                           void* user, int32_t priority, uint32_t layout_hash);

    /// Removes a handler. It is not called again once this returns, except by a call already in it.
    wxl_result(WXL_CALL* detach)(uint32_t attachment);

    /**
     * @brief From a Replace handler: runs the rest of the chain -- the Replace handlers below
     *        this one, then the function -- with the arguments as *call->args holds them now, and
     *        writes the answer to *call->result. May be called more than once, or not at all.
     */
    void(WXL_CALL* call_original)(wxl_point_call* call);
} wxl_point_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_POINT_H
