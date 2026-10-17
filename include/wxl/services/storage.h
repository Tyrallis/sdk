// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The storage service: reading the client's files, whatever they are packed in.
//
// Paths are the client's own: backslash-separated, case-insensitive, relative to the data root --
// "World\\Maps\\Azeroth\\Azeroth_32_48.adt". Whether that resolves inside an archive, a patch or a
// loose folder is the runtime's business.

#ifndef WXL_SERVICE_STORAGE_H
#define WXL_SERVICE_STORAGE_H

#include "wxl/abi.h"

#define WXL_STORAGE_SERVICE         "wxl.storage"
#define WXL_STORAGE_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wxl_storage_api
{
    uint32_t struct_size;

    // --- whole files --------------------------------------------------------------------------

    wxl_bool(WXL_CALL* exists)(wxl_str path);

    /// The file's length, without reading it.
    wxl_result(WXL_CALL* size)(wxl_str path, uint64_t* out);

    /**
     * @brief Reads a whole file into the caller's buffer.
     *
     * The caller allocates and frees: nothing is handed back to free. A buffer too small is
     * WXL_E_INVALID with @p out_read set to what would have been needed, so the usual shape is
     * size() then read().
     */
    wxl_result(WXL_CALL* read)(wxl_str path, void* buffer, uint64_t capacity, uint64_t* out_read);

    // --- streaming ----------------------------------------------------------------------------
    //
    // For a file too big to want in memory at once. A handle must be closed; a runtime closes
    // whatever is left open at shutdown and logs it.

    /// Opens a file, or returns WXL_HANDLE_NONE.
    wxl_handle(WXL_CALL* open)(wxl_str path);

    wxl_result(WXL_CALL* read_at)(wxl_handle file, uint64_t offset, void* buffer, uint64_t size,
                                  uint64_t* out_read);

    void(WXL_CALL* close)(wxl_handle file);

    // --- what gets searched -------------------------------------------------------------------
    //
    // A mod that ships assets makes them findable here rather than by writing into the client's
    // own data folder. Higher priority wins a name collision; the client's own data is 0.

    /// Adds a directory to the search path.
    wxl_result(WXL_CALL* add_search_path)(wxl_str directory, int32_t priority);

    /// Adds an archive to the search path.
    wxl_result(WXL_CALL* mount_archive)(wxl_str path, int32_t priority);

    /**
     * @brief Lists files matching a pattern.
     *
     * The pattern is a client path with `*` standing for any run of characters. Each match is
     * reported once, from whichever source won it.
     * @param out  receives up to @p capacity paths, each valid until the next storage call; may
     *             be NULL to count only.
     * @return how many matched, which may exceed @p capacity.
     */
    uint32_t(WXL_CALL* find)(wxl_str pattern, wxl_str* out, uint32_t capacity);
} wxl_storage_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_STORAGE_H
