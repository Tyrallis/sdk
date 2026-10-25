// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The event contract as data: every event with its family, name and args, the version of the
// contract that brought it, every member of its args with what it carries, and the names of the
// enumerations those members hold. Generated from events.def, fields.def and enums.def. A handler's parameters are the members IsExtent leaves, in this
// order.

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

    /// One value of an enumeration of enums.def.
    struct EnumValue
    {
        const char* type;     ///< the enumeration's C type, "wxl_script_context"
        uint32_t    value;    ///< its number
        const char* constant; ///< its C constant, "WXL_SCRIPT_CONTEXT_GAME"
        const char* name;     ///< what a script sees, "game"
    };

    /// An enumeration of enums.def.
    struct Enum
    {
        const char*      type;   ///< the C type, "wxl_script_context"
        const EnumValue* values; ///< in table order
        uint32_t         count;
    };

    /// One member of an args struct.
    struct Field
    {
        const char* args;   ///< the struct's name
        const char* name;   ///< the member's name
        uint32_t    offset; ///< offsetof the member
        Kind        kind;   ///< u32 for a member that holds an enumeration's values
        /// The member holding a bytes field's length or a buffer field's capacity; null otherwise.
        const char* extent;
        uint32_t    extent_offset; ///< offsetof that member, 0 when there is none
        /// The enumeration a u32 member holds the values of (a WXL_ARGS_ENUM row); null otherwise.
        const Enum* enumeration = nullptr;
        /// Whether a u64 member holds the dwords of an update field (a WXL_ARGS_VALUE row): the
        /// field whose dword index is at `value_field_offset`, of an object whose
        /// wxl_object_type is at `value_type_offset`. update_fields.def says what it holds.
        bool     value              = false;
        uint32_t value_type_offset  = 0;
        uint32_t value_field_offset = 0;
    };

    /// A version of the contract, major.minor.feature, as abi.h numbers it.
    struct Version
    {
        uint32_t major;
        uint32_t minor;
        uint32_t feature;
    };

    /// A WXL_EVENTS_SINCE row of events.def: the version that brought the events from `first` on.
    struct Since
    {
        Version  version;
        uint32_t first; ///< the id of the first event it brought
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

        constexpr bool Same(const char* a, const char* b)
        {
            while (*a && *a == *b) ++a, ++b;
            return *a == *b;
        }

        inline constexpr EnumValue kEnumValues[] = {
#define WXL_ENUM_VALUE(type, constant, value, name) { #type, uint32_t(value), #constant, #name },
#include "wxl/enums.def"
#undef WXL_ENUM_VALUE
        };

        /// The first value row of an enumeration; the rows of one enumeration are contiguous.
        constexpr uint32_t FirstValueOf(const char* type)
        {
            uint32_t i = 0;
            while (i < sizeof kEnumValues / sizeof kEnumValues[0] && !Same(kEnumValues[i].type, type))
                ++i;
            return i;
        }

        constexpr uint32_t ValueCountOf(const char* type)
        {
            uint32_t i = FirstValueOf(type), n = 0;
            while (i + n < sizeof kEnumValues / sizeof kEnumValues[0] &&
                   Same(kEnumValues[i + n].type, type))
                ++n;
            return n;
        }

        inline constexpr Enum kEnums[] = {
#define WXL_ENUM(type) { #type, kEnumValues + FirstValueOf(#type), ValueCountOf(#type) },
#define WXL_ENUM_VALUE(type, constant, value, name)
#include "wxl/enums.def"
#undef WXL_ENUM_VALUE
#undef WXL_ENUM
        };

        /// Not constexpr: reaching it in EnumNamed stops the build at the row that named a type
        /// enums.def does not declare.
        inline const Enum* EnumerationMissingFromEnumsDef() { return nullptr; }

        /// The enumeration of a WXL_ARGS_ENUM row.
        constexpr const Enum* EnumNamed(const char* type)
        {
            for (const Enum& e : kEnums)
                if (Same(e.type, type)) return &e;
            return EnumerationMissingFromEnumsDef();
        }

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
#define WXL_ARGS_ENUM(args, member, type)                                               \
    { #args, #member, uint32_t(offsetof(args, member)), Kind::u32, nullptr, 0,           \
      detail::EnumNamed(#type) },
#define WXL_ARGS_VALUE(args, member, type, field)                                       \
    { #args, #member, uint32_t(offsetof(args, member)), Kind::u64, nullptr, 0, nullptr,  \
      true, uint32_t(offsetof(args, type)), uint32_t(offsetof(args, field)) },
#include "wxl/fields.def"
#undef WXL_ARGS_VALUE
#undef WXL_ARGS_ENUM
#undef WXL_ARGS_BUFFER
#undef WXL_ARGS_ANSWER
#undef WXL_ARGS_BYTES
#undef WXL_ARGS_FIELD
        };

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

        inline constexpr Since kSince[] = {
#define WXL_EVENTS_SINCE(major, minor, feature, first) { { major, minor, feature }, first },
#include "wxl/events.def"
#undef WXL_EVENTS_SINCE
        };
    }

    /// Every member row of fields.def, in table order.
    inline const Field* Fields(uint32_t* count)
    {
        *count = uint32_t(sizeof detail::kFields / sizeof detail::kFields[0]);
        return detail::kFields;
    }

    /// Every enumeration of enums.def, in table order.
    inline const Enum* Enums(uint32_t* count)
    {
        *count = uint32_t(sizeof detail::kEnums / sizeof detail::kEnums[0]);
        return detail::kEnums;
    }

    /// The enumeration of this C type -- "wxl_object_type" -- or null.
    inline const Enum* FindEnum(const char* type)
    {
        if (!type) return nullptr;
        for (const Enum& e : detail::kEnums)
            if (std::strcmp(e.type, type) == 0) return &e;
        return nullptr;
    }

    /// The name a script sees for a value, or null for a number the enumeration does not name.
    inline const char* NameOf(const Enum& enumeration, uint32_t value)
    {
        for (uint32_t i = 0; i < enumeration.count; ++i)
            if (enumeration.values[i].value == value) return enumeration.values[i].name;
        return nullptr;
    }

    /// Every event of events.def, in table order (not id order).
    inline const Event* Events(uint32_t* count)
    {
        *count = uint32_t(sizeof detail::kEvents / sizeof detail::kEvents[0]);
        return detail::kEvents;
    }

    /// Every WXL_EVENTS_SINCE row of events.def, in table order, which is ascending.
    inline const Since* Sinces(uint32_t* count)
    {
        *count = uint32_t(sizeof detail::kSince / sizeof detail::kSince[0]);
        return detail::kSince;
    }

    /// The version of the contract that brought an event: the row of events.def with the
    /// highest first id at or below this one.
    constexpr Version SinceOf(wxl_event id)
    {
        Version version = detail::kSince[0].version;
        for (const Since& since : detail::kSince)
            if (since.first <= uint32_t(id)) version = since.version;
        return version;
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

    /**
     * @brief Whether a member only holds another's length or capacity.
     *
     * A bytes or a buffer member of the same struct names it as its extent. A handler is not
     * given it as a parameter of its own: the bytes or the buffer carry it.
     */
    inline bool IsExtent(const Event& event, const Field& field)
    {
        for (uint32_t i = 0; i < event.field_count; ++i)
            if (event.fields[i].extent && event.fields[i].extent_offset == field.offset) return true;
        return false;
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
