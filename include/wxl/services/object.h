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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reading world objects.
 *
 * A getter writes its out-parameter and returns WXL_OK, or leaves it untouched and returns
 * WXL_E_NOT_FOUND for a handle that no longer resolves, or WXL_E_INVALID for a handle of the
 * wrong kind -- asking a game object for its health is a mistake, not a missing value.
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

    /// The handle for a guid, or WXL_HANDLE_NONE when that object is not in view.
    wxl_handle(WXL_CALL* from_guid)(wxl_guid guid);

    /// The handle of the object the local player controls.
    wxl_handle(WXL_CALL* local_player)(void);

    // --- any object -------------------------------------------------------------------------

    wxl_result(WXL_CALL* position)(wxl_handle object, wxl_vec3* out);
    wxl_result(WXL_CALL* facing)(wxl_handle object, float* out_radians);

    /// The display name. The string points into runtime storage and is valid until the next call.
    wxl_result(WXL_CALL* name)(wxl_handle object, wxl_str* out);

    // --- units ------------------------------------------------------------------------------

    wxl_result(WXL_CALL* unit_health)(wxl_handle unit, uint32_t* out);
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
} wxl_object_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_OBJECT_H
