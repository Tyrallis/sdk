// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The event contract as data: every event with its family, name and args, and every member of
// its args with what it carries. Generated from events.def and fields.def.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "wxl/abi.h"
#include "wxl/events.h"

namespace wxl::reflect
{
    /// What a member carries: the kind column of fields.def.
    enum class Kind : uint8_t
    {
        u32,
        i32,
        u64,
        i64,
        f32,
        boolean, ///< a wxl_bool
        handle,  ///< a wxl_handle
        guid,    ///< a wxl_guid
        vec3,    ///< a wxl_vec3
        str,     ///< a wxl_str, valid for the call
        ptr,     ///< an address only native code can use
        bytes,   ///< `const void*` to the number of bytes in the extent member
        answer,  ///< `wxl_bool*` a handler writes WXL_TRUE through
        buffer,  ///< `char*` a handler writes a string into, the extent member's capacity
    };

    /// One member of an args struct.
    struct Field
    {
        const char* args;   ///< the struct's name
        const char* name;   ///< the member's name
        uint32_t    offset; ///< offsetof the member
        Kind        kind;
        /// The member holding a bytes field's length or a buffer field's capacity; null otherwise.
        const char* extent;
        uint32_t    extent_offset; ///< offsetof that member, 0 when there is none
    };

    /// One event of the contract.
    struct Event
    {
        wxl_event    id;
        const char*  family;      ///< the C++ script class's stem: "World" for WorldScript
        const char*  name;        ///< "Update" for WXL_EV_Update and OnUpdate
        const char*  args;        ///< the struct's name
        uint32_t     args_size;   ///< sizeof the struct
        const Field* fields;      ///< its members, in declaration order
        uint32_t     field_count;
    };

    namespace detail
    {
        // Kinds spelled as the field table writes them.
        constexpr Kind kind_u32     = Kind::u32;
        constexpr Kind kind_i32     = Kind::i32;
        constexpr Kind kind_u64     = Kind::u64;
        constexpr Kind kind_i64     = Kind::i64;
        constexpr Kind kind_f32     = Kind::f32;
        constexpr Kind kind_bool    = Kind::boolean;
        constexpr Kind kind_handle  = Kind::handle;
        constexpr Kind kind_guid    = Kind::guid;
        constexpr Kind kind_vec3    = Kind::vec3;
        constexpr Kind kind_str     = Kind::str;
        constexpr Kind kind_ptr     = Kind::ptr;
        constexpr Kind kind_bytes   = Kind::bytes;
        constexpr Kind kind_answer  = Kind::answer;
        constexpr Kind kind_buffer  = Kind::buffer;

        inline constexpr Field kFields[] = {
#define WXL_ARGS_FIELD(args, member, kind) \
    { #args, #member, uint32_t(offsetof(args, member)), detail::kind_##kind, nullptr, 0 },
#define WXL_ARGS_BYTES(args, member, size)                                              \
    { #args, #member, uint32_t(offsetof(args, member)), Kind::bytes, #size,              \
      uint32_t(offsetof(args, size)) },
#define WXL_ARGS_ANSWER(args, member) \
    { #args, #member, uint32_t(offsetof(args, member)), Kind::answer, nullptr, 0 },
#define WXL_ARGS_BUFFER(args, member, capacity)                                         \
    { #args, #member, uint32_t(offsetof(args, member)), Kind::buffer, #capacity,         \
      uint32_t(offsetof(args, capacity)) },
#include "wxl/fields.def"
#undef WXL_ARGS_BUFFER
#undef WXL_ARGS_ANSWER
#undef WXL_ARGS_BYTES
#undef WXL_ARGS_FIELD
        };

        constexpr bool Same(const char* a, const char* b)
        {
            while (*a && *a == *b) ++a, ++b;
            return *a == *b;
        }

        /// The first row of a struct's members; the rows of one struct are contiguous.
        constexpr uint32_t FirstOf(const char* args)
        {
            uint32_t i = 0;
            while (i < sizeof kFields / sizeof kFields[0] && !Same(kFields[i].args, args)) ++i;
            return i;
        }

        constexpr uint32_t CountOf(const char* args)
        {
            uint32_t i = FirstOf(args), n = 0;
            while (i + n < sizeof kFields / sizeof kFields[0] && Same(kFields[i + n].args, args))
                ++n;
            return n;
        }

        inline constexpr Event kEvents[] = {
#define WXL_EVENT(family, name, id, args)                                                \
    { WXL_EV_##name, #family, #name, #args, uint32_t(sizeof(args)),                      \
      kFields + FirstOf(#args), CountOf(#args) },
#include "wxl/events.def"
#undef WXL_EVENT
        };
    }

    /// Every member row of fields.def, in table order.
    inline const Field* Fields(uint32_t* count)
    {
        *count = uint32_t(sizeof detail::kFields / sizeof detail::kFields[0]);
        return detail::kFields;
    }

    /// Every event of events.def, in table order (not id order).
    inline const Event* Events(uint32_t* count)
    {
        *count = uint32_t(sizeof detail::kEvents / sizeof detail::kEvents[0]);
        return detail::kEvents;
    }

    /// The event with this id, or null.
    inline const Event* Find(wxl_event id)
    {
        for (const Event& event : detail::kEvents)
            if (event.id == id) return &event;
        return nullptr;
    }

    /// The event of this family and name -- ("Object", "UnitDeath") -- or null.
    inline const Event* Find(const char* family, const char* name)
    {
        if (!family || !name) return nullptr;
        for (const Event& event : detail::kEvents)
            if (std::strcmp(event.family, family) == 0 && std::strcmp(event.name, name) == 0)
                return &event;
        return nullptr;
    }

    /// The member of an event's args with this name, or null.
    inline const Field* FieldOf(const Event& event, const char* name)
    {
        if (!name) return nullptr;
        for (uint32_t i = 0; i < event.field_count; ++i)
            if (std::strcmp(event.fields[i].name, name) == 0) return &event.fields[i];
        return nullptr;
    }
}
