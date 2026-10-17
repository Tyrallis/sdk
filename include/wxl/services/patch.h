// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The patch service: writing bytes into the client's image.
//
// For the change a hook cannot express: a branch turned unconditional, a constant replaced, a
// wildcard widened in a format string. Like wxl.hook.raw it is meaningful only where there are
// stable addresses, so only the runtime that loads into a known binary publishes it, and a mod
// that uses it says so in its manifest by requiring it. Asking for it elsewhere returns nothing,
// and that is the correct answer.
//
// Every write names what it expects to find. A write over bytes that are not the expected ones
// is refused and logged, never applied: a wrong address becomes a log line, not a corrupted
// client. Every applied write is journaled and can be undone.

#ifndef WXL_SERVICE_PATCH_H
#define WXL_SERVICE_PATCH_H

#include "wxl/abi.h"

#define WXL_PATCH_SERVICE         "wxl.patch"
#define WXL_PATCH_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/// An applied patch, for restoring. Zero means the write was refused.
typedef uint32_t wxl_patch;

#define WXL_PATCH_NONE ((wxl_patch)0)

typedef struct wxl_patch_api
{
    uint32_t struct_size;

    /**
     * @brief Writes @p size bytes at @p address, provided the memory holds @p expected first.
     *
     * The protection is lifted for the write and put back after it; code is flushed. The bytes
     * found are saved so the write can be undone. A mismatch, an unreadable address or a null
     * argument is refused with a log line naming the patch and what was found.
     * @param name      a label, for the log and the journal.
     * @param expected  the @p size bytes the memory must hold right now.
     * @param bytes     the @p size bytes to write.
     * @return the patch, or WXL_PATCH_NONE when nothing was written.
     */
    wxl_patch(WXL_CALL* apply)(const char* name, uintptr_t address, const void* expected,
                               const void* bytes, uint32_t size);

    /**
     * @brief Puts back the bytes a patch replaced.
     *
     * The write is unconditional: another party may have written the same bytes since, and a
     * restore in the wrong order puts back what the earlier patch found. Restore in reverse
     * order of apply.
     * @return WXL_E_NOT_FOUND for a patch that is not live.
     */
    wxl_result(WXL_CALL* restore)(wxl_patch patch);

    /// Whether the memory at @p address holds @p expected right now, without writing.
    wxl_bool(WXL_CALL* matches)(uintptr_t address, const void* expected, uint32_t size);

    /**
     * @brief Finds a byte pattern in the image's read-only data.
     *
     * Searches the sections of the process image that hold initialised data and are neither
     * writable nor executable: where string literals and constants live. Include the
     * terminator in a string pattern to match a whole literal rather than a prefix.
     * @param after  0 starts from the beginning; an earlier result continues past it.
     * @return the address of the first match after @p after, or 0.
     */
    uintptr_t(WXL_CALL* find)(const void* pattern, uint32_t size, uintptr_t after);
} wxl_patch_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_PATCH_H
