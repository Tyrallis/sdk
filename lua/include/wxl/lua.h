// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The service wxl.lua publishes: what a runtime, a console or a test may ask of the Lua mods.

#ifndef WXL_LUA_H
#define WXL_LUA_H

#include "wxl/abi.h"

#define WXL_LUA_SERVICE         "wxl.lua"
#define WXL_LUA_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Driving the Lua mods. Main thread only.
 */
typedef struct wxl_lua_api
{
    uint32_t struct_size;

    /// How many script mods were found, loaded or not.
    uint32_t(WXL_CALL* count)(void);

    /// Checks every script mod's folder now, instead of at the next poll, and reloads the ones
    /// whose files changed. Returns how many were reloaded.
    uint32_t(WXL_CALL* poll)(void);

    /// Reloads one script mod by id, changed or not. WXL_E_NOT_FOUND for an id that is not a
    /// script mod.
    wxl_result(WXL_CALL* reload)(const char* id);
} wxl_lua_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_LUA_H
