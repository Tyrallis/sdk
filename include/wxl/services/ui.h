// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The overlay service: a developer panel, drawn over the game.
//
// The runtime owns the interface library and never lends it out. Handing a mod the library's own
// context would tie both sides to one build of it and to one C++ ABI, which is the guarantee this
// whole contract exists to keep. So what crosses is this: named controls, primitive types, and a
// callback invoked at the point where drawing them is legal.
//
// It stays immediate mode -- a panel may loop, branch, and draw something different every frame.
// This is for tools and diagnostics, not for game interface; game interface is FrameXML's job.

#ifndef WXL_SERVICE_UI_H
#define WXL_SERVICE_UI_H

#include "wxl/abi.h"

#define WXL_UI_SERVICE         "wxl.ui"
#define WXL_UI_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A panel body, called while the overlay is open.
 *
 * Runs inside an already-open window, so it draws by calling the functions below and nothing
 * else. It is not called on frames where the overlay is shut, so it must not be where a mod does
 * work it needs done.
 */
typedef void(WXL_CALL* wxl_panel_fn)(void* user);

typedef struct wxl_ui_api
{
    uint32_t struct_size;

    /**
     * @brief Registers a panel, drawn whenever the overlay is open.
     * @param title  the window title, and its identity -- unique and stable.
     */
    wxl_result(WXL_CALL* add_panel)(const char* title, wxl_panel_fn fn, void* user);

    /// Non-zero while the overlay is open, for a mod that wants to know without owning a panel.
    wxl_bool(WXL_CALL* is_open)(void);

    // --- controls -----------------------------------------------------------------------------
    //
    // Every function below is valid only inside a wxl_panel_fn and does nothing anywhere else.
    // A control bound to a value reads it for the current state and writes it when it changes,
    // and answers WXL_TRUE on the frame it changed.
    //
    // Formatting is the caller's: no varargs cross the boundary.

    void(WXL_CALL* text)(wxl_str text);
    void(WXL_CALL* separator)(void);
    void(WXL_CALL* same_line)(void);

    wxl_bool(WXL_CALL* button)(const char* label);

    /// int32, not bool: bool has no guaranteed representation across a compiler boundary.
    wxl_bool(WXL_CALL* checkbox)(const char* label, wxl_bool* value);

    wxl_bool(WXL_CALL* slider_float)(const char* label, float* value, float min, float max);
    wxl_bool(WXL_CALL* slider_int)(const char* label, int32_t* value, int32_t min, int32_t max);

    /// Four floats in 0..1, red first.
    wxl_bool(WXL_CALL* color_edit)(const char* label, float rgba[4]);

    /**
     * @brief A dropdown over a list of labels.
     * @param index  the entry shown, written when another is picked. An index left over from a
     *               longer list is clamped rather than refused: keeping a selection across a
     *               change of subject is the ordinary case, not a mistake.
     * @param items  @p count NUL-terminated labels, read for the duration of the call only.
     */
    wxl_bool(WXL_CALL* combo)(const char* label, int32_t* index, const char* const* items,
                              int32_t count);

    /// A section header the reader can fold away. True while open, which is when to draw its body.
    wxl_bool(WXL_CALL* collapsing_header)(const char* label);

    /**
     * @brief A single-line text field bound to the caller's buffer.
     * @param buffer    read for the current text, written in place, always left NUL-terminated.
     * @param capacity  bytes available, terminator included.
     */
    wxl_bool(WXL_CALL* input_text)(const char* label, char* buffer, uint32_t capacity);
} wxl_ui_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_UI_H
