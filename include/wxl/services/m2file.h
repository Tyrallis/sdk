// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The model-file service: what a mod needs to reshape a model file the client has parsed, for a
// mod that teaches the client a model format it was not written for.
//
// The loader's stages reach a mod as events of the Asset family, each with the model's file (the
// client's CM2Shared, opaque) and its parsed header:
//
//   ModelLoad          the file read whole, nothing parsed: its bytes can be replaced (an `m2`
//                      transform of wxl.storage converts them, to any size)
//   ModelParsed        the header parsed, nothing made from it yet: arrays can be pointed at
//                      other memory, ahead of the skin profile's choice and the textures
//   SkinProfileLoad    the skin profile read, before it is parsed
//   SkinProfileParsed  the skin profile parsed, before the client sizes its draw state from it:
//                      batches, submeshes and the model's lookups can still be replaced
//   ModelSequenceLoad  an external sequence (.anim) read, before it is parsed against the model
//
// An array of a parsed model is two 32-bit words, a count and where its elements are. Where they
// are depends on the runtime: the stock client keeps their address, the recompiled client (whose
// addresses are 64-bit) their offset from the array itself. array_data and set_array read and
// write an array the same way on both, and alloc_near gives memory a 64-bit client's array can
// reach.
//
// Raw: everything here is the client's own memory, written as it lies. Calls are made on
// the client's main thread, from the handlers of those events.

#ifndef WXL_SERVICE_M2FILE_H
#define WXL_SERVICE_M2FILE_H

#include "wxl/abi.h"

#define WXL_M2FILE_SERVICE         "wxl.m2file"
#define WXL_M2FILE_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wxl_m2file_api
{
    uint32_t struct_size;

    /**
     * @brief @p size zeroed bytes, 16-aligned, that an array lying at @p anchor can be pointed
     *        at with set_array. They live for the session: the client never frees a model's
     *        arrays one by one, and neither is this. Null when no memory is left in reach.
     *
     * On the recompiled client the bytes lie within 1 GB of the anchor, well inside a 32-bit
     * offset; on the stock client any address is in reach.
     */
    void*(WXL_CALL* alloc_near)(const void* anchor, uint32_t size);

    /**
     * @brief Has the client animate every bone of @p file's models when it draws their shadows.
     *
     * The client skips the bone pass of a model none of whose bones carries one of the flags it
     * counts at load (billboards, a transform), and draws the shadow from the model's transform
     * alone. A file whose bones are animated but carry none of those flags then casts a shadow
     * that does not move with it. From ModelParsed on.
     */
    void(WXL_CALL* animate_bones)(void* file);

    /// Where the elements of the parsed array at @p array are, as this runtime keeps it; null
    /// for an empty array.
    void*(WXL_CALL* array_data)(const void* array);

    /**
     * @brief Points the parsed array at @p array at @p count elements at @p data, as this
     *        runtime keeps it.
     *
     * WXL_E_INVALID when @p data lies out of a 32-bit offset's reach of the array, which only
     * happens on the recompiled client: alloc_near(array, size) gives memory that is in reach.
     */
    wxl_result(WXL_CALL* set_array)(void* array, const void* data, uint32_t count);
} wxl_m2file_api;

#ifdef __cplusplus
}
#endif

#endif
