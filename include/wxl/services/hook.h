// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The hook service: the escape hatch.
//
// Events cover what every runtime can promise. This covers the rest: a mod that needs to sit in
// the middle of something the contract does not model, and accepts what that costs.
//
// Two levels, and the difference matters:
//
//   by name   -- the runtime publishes a table of hook points it is willing to name and keep
//                working. Portable in principle: another runtime that names the same point
//                serves the same mod.
//   by address -- a raw detour. Meaningful only where there are stable addresses, which means
//                the legacy runtime and nowhere else. A mod that uses it is pinned to one
//                binary, and should say so in its manifest by requiring this service.
//
// Detours chain: `original` receives the next link, which ends at the engine's own function. Not
// calling it suppresses both the parties behind and the engine function.

#ifndef WXL_SERVICE_HOOK_H
#define WXL_SERVICE_HOOK_H

#include "wxl/abi.h"

#define WXL_HOOK_SERVICE         "wxl.hook"
#define WXL_HOOK_SERVICE_VERSION 1

/// A runtime that lets a mod detour a raw address publishes this one as well.
#define WXL_HOOK_RAW_SERVICE         "wxl.hook.raw"
#define WXL_HOOK_RAW_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/// Lower runs first, closest to the caller. Equal values keep attach order.
#define WXL_HOOK_PRIORITY_DEFAULT 0

/// A live detour, for detaching. Zero means the attach did not take.
typedef uint32_t wxl_hook;

#define WXL_HOOK_NONE ((wxl_hook)0)

/// What a runtime says about a hook point it names.
typedef struct wxl_hook_point
{
    uint32_t struct_size;
    wxl_str  name;      ///< the name to attach by
    wxl_str  signature; ///< the detour's C signature, as text, for a human to check against
} wxl_hook_point;

typedef struct wxl_hook_api
{
    uint32_t struct_size;

    /**
     * @brief Lists the hook points this runtime names.
     * @param out  receives up to @p capacity points, valid until the next hook call; may be NULL
     *             to count only.
     * @return how many exist, which may exceed @p capacity.
     */
    uint32_t(WXL_CALL* points)(wxl_hook_point* out, uint32_t capacity);

    /// Whether a named point exists, without attaching to it.
    wxl_bool(WXL_CALL* has_point)(const char* name);

    /**
     * @brief Attaches a detour to a named point.
     * @param original  receives the next link in the chain.
     * @return the hook, or WXL_HOOK_NONE when the point is not named by this runtime.
     */
    wxl_hook(WXL_CALL* attach)(const char* name, void* detour, void** original, int32_t priority);

    /// Removes a detour. The chain closes over it; links behind keep working.
    wxl_result(WXL_CALL* detach)(wxl_hook hook);
} wxl_hook_api;

/**
 * @brief Detouring a raw address.
 *
 * Published only by a runtime that loads into a binary whose addresses mean something. Asking for
 * it on a runtime compiled from source returns nothing, and that is the correct answer rather
 * than a fault -- the addresses there are whatever the linker chose that build.
 */
typedef struct wxl_hook_raw_api
{
    uint32_t struct_size;

    /// The image the addresses below are relative to, so a mod can rebase its own table.
    uintptr_t(WXL_CALL* image_base)(void);

    /// The build the loaded image reports, for a mod that hardcodes addresses.
    uint32_t(WXL_CALL* image_build)(void);

    /**
     * @brief Attaches a detour to an address.
     * @param name      a label, for logging; the runtime does not resolve it.
     * @param original  receives the next link in the chain.
     */
    wxl_hook(WXL_CALL* attach_at)(const char* name, uintptr_t target, void* detour, void** original,
                                  int32_t priority);

    wxl_result(WXL_CALL* detach)(wxl_hook hook);
} wxl_hook_raw_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_HOOK_H
