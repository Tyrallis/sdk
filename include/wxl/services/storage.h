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

    /**
     * @brief Adds a directory to the search path, for the calling mod.
     *
     * The folder is the mod's: adding it again changes its priority, and it leaves the search
     * when the mod removes it (remove_search_path) or goes -- disabled or unloaded. Another mod
     * may add the same folder; each holds its own.
     */
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

    // --- appended in 0.1.10 -----------------------------------------------------------------

    /**
     * @brief Takes a folder the calling mod added back out of the search path.
     *
     * Named as add_search_path was given it, or by any spelling of the same folder. From here on
     * no name is answered from it; a file opened from it already stays open. WXL_E_NOT_FOUND
     * when the mod holds no such folder -- another mod's stays.
     */
    wxl_result(WXL_CALL* remove_search_path)(wxl_str directory);

    // --- appended in 0.1.15 -----------------------------------------------------------------

    /**
     * @brief Opens one name in place of another, for the calling mod, on every thread.
     *
     * From here on, whatever opens @p from -- the client on any of its threads, or a mod --
     * opens @p to instead, found as any name is: in a folder of the search path or in an
     * archive. Names are compared as the client compares them, in any case and with either
     * slash. This is the way to answer an open that happens off the main thread, where a
     * FileOpen handler that runs later cannot answer it.
     *
     * The redirect is the mod's: redirecting the same name again changes where it leads, and
     * it lasts until the mod takes it back (unredirect) or goes -- disabled or unloaded. When
     * two mods redirect one name, the latest wins, and taking it back uncovers the earlier
     * one. A FileOpen handler that answers still has the last word. WXL_E_INVALID for an empty
     * name or one longer than 511 bytes.
     */
    wxl_result(WXL_CALL* redirect)(wxl_str from, wxl_str to);

    /// Takes back the calling mod's redirect of a name: WXL_E_NOT_FOUND when it holds none.
    wxl_result(WXL_CALL* unredirect)(wxl_str from);
} wxl_storage_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_STORAGE_H
