// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The data service: the client's own databases, served from a mod's files instead of its own.
//
// A client database is a .dbc the client reads once, early, into a table it keeps for the rest
// of the session. It never holds a pointer into the file and never reads it again, so there is
// nothing to intercept afterwards: the only moment at which a mod can change what the client
// believes is before that read. This service is that moment, named.
//
// A table is named as the client names it, with or without its folder:
// "DBFilesClient\\Spell.dbc" and "Spell.dbc" are the same table, and case does not matter.

#ifndef WXL_SERVICE_DATA_H
#define WXL_SERVICE_DATA_H

#include "wxl/abi.h"

#define WXL_DATA_SERVICE         "wxl.data"
#define WXL_DATA_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wxl_data_api
{
    uint32_t struct_size;

    /**
     * @brief Serves a client database from @p path instead of the client's own file.
     *
     * The file must be a .dbc the client can read: same columns, same order, same string
     * block. Adding rows is what this is for; changing the shape is not, and the client will
     * fail to start rather than guess.
     *
     * Must be called before the client reads the table, which happens early in the boot. The
     * safe places are the load call and WXL_STAGE_STORAGE; later is too late, and @ref
     * is_loaded says which it was.
     *
     * @return WXL_OK, WXL_E_FAILED when another mod already serves that table (the runtime's
     *         log names it), or WXL_E_NOT_FOUND when @p path cannot be read.
     */
    wxl_result(WXL_CALL* serve_table)(wxl_str table, wxl_str path);

    /// Stops serving a table, so the client's own file is used again. Only the mod that
    /// claimed it may give it back.
    wxl_result(WXL_CALL* restore_table)(wxl_str table);

    /// The path a table is being served from, empty when the client's own file is used.
    wxl_str(WXL_CALL* served_from)(wxl_str table);

    /**
     * @brief Whether the client has already read that table.
     *
     * True means an override of it will not be seen this session. There is no way to make the
     * client read one again: it loads each exactly once and refuses a second load.
     */
    wxl_bool(WXL_CALL* is_loaded)(wxl_str table);
} wxl_data_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_DATA_H
