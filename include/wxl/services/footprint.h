// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The footprint service: where the client's memory goes.
//
// Three kinds of answer, all read from one picture taken by snapshot():
//
//   what the client allocates through Storm, its own allocator, by the source file that asked
//   for the memory -- counted only while accounting runs (start .. stop), since it costs a
//   little on every allocation;
//
//   what the big consumers outside Storm hold, read from the client when the picture is taken:
//   the graphics device's textures and buffers by pool, the sound engine's heap, the client's
//   script heap, the tables of each mounted archive, the client databases; and what mods report
//   of their own (report);
//
//   the process's address space: how much is committed, reserved and free, by kind of memory,
//   and the largest free block. A 32-bit client fails an allocation when no free block is large
//   enough, long before the total runs out, so the largest free block is the number to watch.
//
// A picture is taken on the calling thread and reads the client's own lists: take it on the
// main thread, where mods run. Every list call reads the last picture; its strings stay valid
// until the next snapshot().

#ifndef WXL_SERVICE_FOOTPRINT_H
#define WXL_SERVICE_FOOTPRINT_H

#include "wxl/abi.h"

#define WXL_FOOTPRINT_SERVICE         "wxl.footprint"
#define WXL_FOOTPRINT_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief One consumer of memory, as categories() lists them.
 *
 * Built-in names, owner "client":
 *
 *   storm                Storm's allocations live since start(): what the source files add up to
 *   textures.managed     textures of the managed pool: in video memory and, as a copy, in the
 *                        process's own memory, where they cost address space
 *   textures.default     textures of the default pool (render targets): video memory only
 *   textures.systemmem   textures in system memory only
 *   buffers.managed, buffers.default, buffers.systemmem   the vertex and index buffers, the same way
 *   sound                the sound engine's heap (count: none)
 *   script               the client's own Lua heap, the interface's (count: none)
 *   archives             the tables of the mounted archives (count: archives)
 *   tables               the client databases (count: tables loaded)
 *   accounting           what the accounting itself holds while it runs
 *
 * A mod's own, from report(), carry its id as owner. The categories overlap: the script heap,
 * the databases and the archives' tables are allocated through Storm too, so they are also in
 * `storm` and its source files. Texture and buffer sizes are what their formats and dimensions
 * take, not what a driver rounds them up to.
 */
typedef struct wxl_footprint_category
{
    uint32_t struct_size;
    wxl_str  name;
    wxl_str  owner; ///< "client", or the id of the mod that reported it
    uint64_t bytes;
    uint64_t count; ///< how many blocks, objects or entries; 0 where it means nothing
    uint64_t peak;  ///< the most bytes seen since start(), where the runtime follows it; else bytes
} wxl_footprint_category;

/// One source file of the client, as Storm was told it by the code that allocated.
typedef struct wxl_footprint_source
{
    uint32_t struct_size;
    wxl_str  file;        ///< as the client spells it: ".\\Client.cpp", "new", a full build path
    uint64_t bytes;       ///< live now, requested sizes rounded up to 8 as Storm does
    uint64_t count;       ///< live blocks
    uint64_t peak;        ///< the most bytes live at once since start()
    uint64_t allocations; ///< every allocation since start(), freed or not
} wxl_footprint_source;

/// One mounted archive.
typedef struct wxl_footprint_archive
{
    uint32_t struct_size;
    wxl_str  path;           ///< the path it was opened by
    int32_t  priority;
    uint32_t hash_entries;   ///< its hash table's entries, as its header says
    uint32_t block_entries;  ///< its block table's entries
    uint64_t bytes;          ///< what its tables and listfile hold in memory now
} wxl_footprint_archive;

/// One client database.
typedef struct wxl_footprint_table
{
    uint32_t struct_size;
    wxl_str  name;        ///< its file: "DBFilesClient\\Spell.dbc"
    uint32_t records;
    uint32_t record_size; ///< bytes per record in memory
    uint64_t bytes;       ///< records, strings and the index by id
} wxl_footprint_table;

/**
 * @brief One kind of the process's address space.
 *
 * `state` is "committed", "reserved" or "free"; `type` is "image" (executables and DLLs),
 * "mapped" (mapped files and sections) or "private" (heaps, stacks, the rest), empty for free.
 */
typedef struct wxl_footprint_region
{
    uint32_t struct_size;
    wxl_str  state;
    wxl_str  type;
    uint64_t bytes;
    uint32_t regions; ///< how many separate regions
    uint64_t largest; ///< the largest one
} wxl_footprint_region;

typedef struct wxl_footprint_api
{
    uint32_t struct_size;

    // --- accounting ---------------------------------------------------------------------------

    /**
     * @brief Starts counting Storm's allocations, from zero.
     *
     * Only what is allocated from here on is followed: a block allocated before is not counted,
     * and its free is not either. Starting while it runs starts again from zero.
     */
    wxl_result(WXL_CALL* start)(void);

    /// Stops counting. The last counts stay in the next picture; nothing is followed any more.
    void(WXL_CALL* stop)(void);

    /// Whether accounting runs.
    wxl_bool(WXL_CALL* running)(void);

    // --- the picture --------------------------------------------------------------------------

    /// Takes the picture the lists below read: counts, the client's consumers, the address space.
    wxl_result(WXL_CALL* snapshot)(void);

    /**
     * @brief The lists of the last picture: up to @p capacity entries into @p out, which may be
     *        NULL to count only. Each answers how many there are, which may exceed @p capacity.
     *
     * categories and sources come largest first, archives in the client's order, tables by
     * name, regions committed, then reserved, then free.
     */
    uint32_t(WXL_CALL* categories)(wxl_footprint_category* out, uint32_t capacity);
    uint32_t(WXL_CALL* sources)(wxl_footprint_source* out, uint32_t capacity);
    uint32_t(WXL_CALL* archives)(wxl_footprint_archive* out, uint32_t capacity);
    uint32_t(WXL_CALL* tables)(wxl_footprint_table* out, uint32_t capacity);
    uint32_t(WXL_CALL* regions)(wxl_footprint_region* out, uint32_t capacity);

    // --- what a mod adds ----------------------------------------------------------------------

    /**
     * @brief States one consumer of the calling mod's own, shown in categories() under its id.
     *
     * Kept until the mod reports it again or goes; a report of 0 bytes and 0 count removes it.
     * WXL_E_INVALID for an empty name.
     */
    wxl_result(WXL_CALL* report)(wxl_str name, uint64_t bytes, uint64_t count);

    /**
     * @brief Arms MemoryPressure: it is raised when the largest free block of the address space
     *        falls under @p bytes, and again only once it has risen back above.
     *
     * The runtime looks about once a second. 0 disarms it. Each mod holds its own threshold and
     * the highest one counts, so no mod is told later than it asked; the event carries the
     * largest free block, which a mod compares with its own. A mod's threshold goes with it.
     */
    wxl_result(WXL_CALL* watch_pressure)(uint64_t bytes);
} wxl_footprint_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_FOOTPRINT_H
