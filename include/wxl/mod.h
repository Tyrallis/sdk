// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// What a mod exposes: the entry points a runtime resolves out of it, and what it reports about
// itself before any of its code runs.

#ifndef WXL_MOD_H
#define WXL_MOD_H

#include "wxl/abi.h"
#include "wxl/host.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief How far the client has booted.
 *
 * Stages run in the order listed below and are reported to every mod, on the main thread.
 * Shutdown runs mods in reverse load order. A stage added later gets the next free number and
 * takes its place in the list wherever it falls in the boot, so the order is the list's, not the
 * numbers'. A runtime that cannot reach a stage never reports it.
 */
typedef enum wxl_stage
{
    /// Nothing is up yet. The slot for anything that must be in place before a subsystem starts.
    WXL_STAGE_EARLY = 0,
    /// Archives are mounted and files can be read.
    WXL_STAGE_STORAGE = 1,
    /// A graphics device exists.
    WXL_STAGE_GRAPHICS = 2,
    /// The script VM exists and an API can be registered on it.
    WXL_STAGE_SCRIPT = 3,
    /// Fully booted, before the first frame.
    WXL_STAGE_READY = 4,
    /// Coming down. Release what the stages above acquired.
    WXL_STAGE_SHUTDOWN = 5
} wxl_stage;

/**
 * @brief What a mod reports about itself before any of its code runs.
 *
 * Returned by wxl_mod_query, which must have no side effects: it is the only point at which an
 * incompatible mod can be refused having executed nothing.
 */
typedef struct wxl_mod_info
{
    uint32_t    struct_size;
    uint32_t    abi_major; ///< WXL_ABI_VERSION_MAJOR seen at compile time
    uint32_t    abi_minor; ///< WXL_ABI_VERSION_MINOR seen at compile time
    const char* id;        ///< stable identity, e.g. "wxl.foliage"; matches the manifest
    const char* name;      ///< display name, used in logs
    uint32_t    version;   ///< the mod's own version, opaque to the runtime

    // --- appended at 0.1.5 -------------------------------------------------------------------

    /**
     * @brief WXL_ABI_VERSION_FEATURE seen at compile time.
     *
     * On x64 it lands in what was the struct's tail padding, so @ref struct_size alone does not
     * tell a binary built before the three-part numbering apart: the abi_minor it reports does.
     */
    uint32_t abi_feature;
} wxl_mod_info;

// --- Entry points --------------------------------------------------------------------------
//
// Declared only while compiling the mod itself: a runtime includes this header for the types and
// must not export the symbols it goes looking for.

#ifdef WXL_BUILDING_MOD

/**
 * @brief Describes the mod. Must have no side effects.
 * @return the mod's info, or NULL to decline being loaded.
 */
WXL_EXPORT const wxl_mod_info* WXL_CALL wxl_mod_query(void);

/**
 * @brief The mod sets itself up.
 *
 * Called on the main thread before WXL_STAGE_EARLY. Nothing the boot brings up exists yet: no
 * graphics device, no script VM, no world. Subscribe here, or wait for the stage that provides
 * what is needed.
 * @param host  the runtime's table, valid for the process lifetime.
 * @return WXL_OK to stay loaded; anything else unloads the mod and logs the reason.
 */
WXL_EXPORT wxl_result WXL_CALL wxl_mod_load(const wxl_host* host);

/// The client reached a boot stage. Optional: a runtime that cannot resolve it skips it.
WXL_EXPORT void WXL_CALL wxl_mod_stage(wxl_stage stage);

/**
 * @brief The mod is being unloaded. Optional.
 *
 * Runs after WXL_STAGE_SHUTDOWN and is the last of the mod's code to run.
 */
WXL_EXPORT void WXL_CALL wxl_mod_unload(void);

#endif // WXL_BUILDING_MOD

/// The entry points as a runtime resolves them, by name, out of a loaded mod.
typedef const wxl_mod_info*(WXL_CALL* wxl_mod_query_fn)(void);
typedef wxl_result(WXL_CALL* wxl_mod_load_fn)(const wxl_host* host);
typedef void(WXL_CALL* wxl_mod_stage_fn)(wxl_stage stage);
typedef void(WXL_CALL* wxl_mod_unload_fn)(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_MOD_H
