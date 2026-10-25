// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::field: the update fields of wxl/update_fields.def, as typed constants and as a table.

#ifndef WXL_OBJECTS_FIELDS_HPP
#define WXL_OBJECTS_FIELDS_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "wxl/abi.h"
#include "wxl/events.h"

namespace wxl::field
{
    /// What one element of a field holds: the value column of update_fields.def.
    enum class Value : uint8_t
    {
        u32,
        i32,
        f32,
        guid,  ///< an object's guid, two dwords
        u64,   ///< another 64-bit value, two dwords
        bytes, ///< four bytes, low first
        u16x2, ///< two 16-bit halves, low first
        i16x2,
    };

    /// Whether the client sees a change of the field: the watch column (wxl_object_api's
    /// watch_field accepts any and local, and the elements of some that the WATCH rows cover).
    enum class Watch : uint8_t
    {
        none,
        any,   ///< on every object of the kind
        local, ///< on the active player only
        some,  ///< on some elements of it
    };

    /// Who the server sends a field to: the flags column, as the client's descriptors carry it.
    enum Visibility : uint32_t
    {
        kPublic      = 0x001, ///< everyone who sees the object
        kPrivate     = 0x002, ///< its owner's client only
        kOwner       = 0x004, ///< its owner
        kItemOwner   = 0x010, ///< an item's owner
        kSpecialInfo = 0x020,
        kPartyMember = 0x040, ///< the owner's party
        kDynamic     = 0x100, ///< worked out for each viewer
    };

    /// The dwords one element of a value takes.
    constexpr uint32_t DwordsOf(Value value)
    {
        return value == Value::guid || value == Value::u64 ? 2u : 1u;
    }

    /// The C++ type of one element: what Object::Field answers for it.
    template <Value V>
    struct ValueType;
    template <> struct ValueType<Value::u32>   { using type = uint32_t; };
    template <> struct ValueType<Value::i32>   { using type = int32_t; };
    template <> struct ValueType<Value::f32>   { using type = float; };
    template <> struct ValueType<Value::guid>  { using type = wxl_guid; };
    template <> struct ValueType<Value::u64>   { using type = uint64_t; };
    template <> struct ValueType<Value::bytes> { using type = std::array<uint8_t, 4>; };
    template <> struct ValueType<Value::u16x2> { using type = std::array<uint16_t, 2>; };
    template <> struct ValueType<Value::i16x2> { using type = std::array<int16_t, 2>; };

    template <Value V>
    using ValueOf = typename ValueType<V>::type;

    /**
     * @brief One field of update_fields.def, typed by what its elements hold.
     *
     *     wxl::Expected<int32_t> health = unit.Field(wxl::field::Unit::Health);
     *
     * A field of several elements -- UNIT_FIELD_RESISTANCES holds seven -- is read one element
     * at a time (Object::Field's second argument) or whole (Object::Fields).
     */
    template <class T>
    struct Field
    {
        wxl_object_type kind;  ///< the kind that owns it; WXL_OBJECT_UNKNOWN for every object's
        uint32_t        index; ///< its first dword
        uint32_t        size;  ///< its dwords
        Value           value;
        const char*     name;  ///< the client's: "UNIT_FIELD_HEALTH"

        /// The dwords of one element: 2 for a guid or another 64-bit value, 1 otherwise.
        constexpr uint32_t Dwords() const { return DwordsOf(value); }
        constexpr uint32_t Elements() const { return size / DwordsOf(value); }
        /// The dword index of an element.
        constexpr uint32_t IndexOf(uint32_t element) const { return index + element * DwordsOf(value); }
    };

    /// An element read out of its dwords, which hold it low first.
    template <class T>
    T Decode(const uint32_t* dwords)
    {
        static_assert(sizeof(T) % 4 == 0 && sizeof(T) <= 8, "an element is one or two dwords");
        T value;
        std::memcpy(&value, dwords, sizeof value);
        return value;
    }

    /// An element as the dwords the client holds it in.
    template <class T>
    void Encode(const T& value, uint32_t* dwords)
    {
        static_assert(sizeof(T) % 4 == 0 && sizeof(T) <= 8, "an element is one or two dwords");
        std::memcpy(dwords, &value, sizeof value);
    }

    namespace detail
    {
        /// Object's parent: none, which the table reads as itself.
        inline constexpr wxl_object_type kTypeOfNone = WXL_OBJECT_UNKNOWN;
#define WXL_UPDATE_FIELD(kind, NAME, Name, index, size, value, flags, watch)
#define WXL_UPDATE_FIELD_KIND(kind, type, parent, first, end, others_end) \
    inline constexpr wxl_object_type kTypeOf##kind = type;
#include "wxl/update_fields.def"
#undef WXL_UPDATE_FIELD_KIND
#undef WXL_UPDATE_FIELD
    } // namespace detail

    // The constants, one namespace per kind: wxl::field::Unit::Health, wxl::field::Player::Xp.
    // A name is the client's with the kind's prefix dropped and the words joined, as the Name
    // column of update_fields.def writes it.
#define WXL_UPDATE_FIELD(kind, NAME, Name, index, size, value, flags, watch)                     \
    namespace kind                                                                           \
    {                                                                                        \
        inline constexpr ::wxl::field::Field<::wxl::field::ValueOf<::wxl::field::Value::value>> \
            Name{::wxl::field::detail::kTypeOf##kind, index, size, ::wxl::field::Value::value, #NAME}; \
    }
#include "wxl/update_fields.def"
#undef WXL_UPDATE_FIELD

    // --- the table, for code that names fields at run time ------------------------------------

    /// A kind of object and its block (a WXL_UPDATE_FIELD_KIND row).
    struct Kind
    {
        const char*     name;       ///< "Unit"
        wxl_object_type type;
        wxl_object_type parent;     ///< the kind its fields come after; itself for Object
        uint32_t        first;      ///< its own fields' first dword
        uint32_t        end;        ///< past its last
        uint32_t        others_end; ///< where the block of an object of the kind ends: `end`,
                                    ///< except a player other than the active one's
    };

    /// One field (a WXL_UPDATE_FIELD row).
    struct Row
    {
        wxl_object_type kind;
        const char*     name; ///< "UNIT_FIELD_HEALTH"
        const char*     code; ///< "Health"
        uint32_t        index;
        uint32_t        size;
        Value           value;
        uint32_t        flags;
        Watch           watch;

        constexpr uint32_t Dwords() const { return DwordsOf(value); }
        constexpr uint32_t Elements() const { return size / DwordsOf(value); }
    };

    /// Dwords the client keeps a copy of (a WXL_UPDATE_FIELD_WATCH row).
    struct WatchRun
    {
        wxl_object_type kind;
        uint32_t        first;
        uint32_t        end;
        bool            local; ///< on the active player only
    };

    namespace detail
    {
        inline constexpr Kind kKinds[] = {
#define WXL_UPDATE_FIELD(kind, NAME, Name, index, size, value, flags, watch)
#define WXL_UPDATE_FIELD_KIND(kind, type, parent, first, end, others_end) \
    {#kind, type, kTypeOf##parent, first, end, others_end},
#include "wxl/update_fields.def"
#undef WXL_UPDATE_FIELD_KIND
#undef WXL_UPDATE_FIELD
        };

        inline constexpr Row kRows[] = {
#define WXL_UPDATE_FIELD(kind, NAME, Name, index, size, value, flags, watch) \
    {kTypeOf##kind, #NAME, #Name, index, size, Value::value, flags, Watch::watch},
#include "wxl/update_fields.def"
#undef WXL_UPDATE_FIELD
        };

        inline constexpr WatchRun kWatchRuns[] = {
#define WXL_UPDATE_FIELD(kind, NAME, Name, index, size, value, flags, watch)
#define WXL_UPDATE_FIELD_WATCH(kind, first, end, scope) \
    {kTypeOf##kind, first, end, Watch::scope == Watch::local},
#include "wxl/update_fields.def"
#undef WXL_UPDATE_FIELD_WATCH
#undef WXL_UPDATE_FIELD
        };

        constexpr bool Same(std::string_view a, const char* b) { return a == std::string_view(b); }
    } // namespace detail

    inline const Kind* Kinds(uint32_t* count)
    {
        *count = uint32_t(sizeof detail::kKinds / sizeof detail::kKinds[0]);
        return detail::kKinds;
    }

    inline const Row* Rows(uint32_t* count)
    {
        *count = uint32_t(sizeof detail::kRows / sizeof detail::kRows[0]);
        return detail::kRows;
    }

    inline const WatchRun* WatchRuns(uint32_t* count)
    {
        *count = uint32_t(sizeof detail::kWatchRuns / sizeof detail::kWatchRuns[0]);
        return detail::kWatchRuns;
    }

    /// The kind of a wxl_object_type, or null for a number past the client's kinds.
    constexpr const Kind* KindOf(uint32_t type)
    {
        for (const Kind& kind : detail::kKinds)
            if (static_cast<uint32_t>(kind.type) == type) return &kind;
        return nullptr;
    }

    /// The kind by its name, "Unit", or null.
    constexpr const Kind* KindNamed(std::string_view name)
    {
        for (const Kind& kind : detail::kKinds)
            if (detail::Same(name, kind.name)) return &kind;
        return nullptr;
    }

    /// Whether an object of type `type` carries the fields of kind `kind`: a player carries the
    /// unit's and the object's.
    constexpr bool Carries(uint32_t type, uint32_t kind)
    {
        for (const Kind* k = KindOf(type); k; k = k->type == k->parent ? nullptr : KindOf(k->parent))
            if (static_cast<uint32_t>(k->type) == kind) return true;
        return false;
    }

    /// The kind of an object of type `type` whose fields hold dword `index`, or null past its
    /// block.
    constexpr const Kind* OwnerOf(uint32_t type, uint32_t index)
    {
        for (const Kind* k = KindOf(type); k; k = k->type == k->parent ? nullptr : KindOf(k->parent))
            if (index >= k->first && index < k->end) return k;
        return nullptr;
    }

    /// The field of an object of type `type` that holds dword `index`, or null.
    constexpr const Row* RowAt(uint32_t type, uint32_t index)
    {
        const Kind* owner = OwnerOf(type, index);
        if (!owner) return nullptr;
        for (const Row& row : detail::kRows)
            if (row.kind == owner->type && index >= row.index && index < row.index + row.size)
                return &row;
        return nullptr;
    }

    /// Where an object's block ends: its kind's end, or a player's other than the active one's.
    constexpr uint32_t BlockEnd(uint32_t type, bool active_player)
    {
        const Kind* kind = KindOf(type);
        if (!kind) return 0;
        return active_player ? kind->end : kind->others_end;
    }

    /// Whether the client sees a change of dword `index` of kind `kind`: none, any or local.
    constexpr Watch WatchOf(uint32_t kind, uint32_t index)
    {
        for (const WatchRun& run : detail::kWatchRuns)
            if (static_cast<uint32_t>(run.kind) == kind && index >= run.first && index < run.end)
                return run.local ? Watch::local : Watch::any;
        return Watch::none;
    }

    /**
     * @brief A field of an object of type `type` by name, or null.
     *
     * The client's name ("UNIT_FIELD_HEALTH"), the code name qualified by its kind
     * ("Unit.Health"), or the code name alone ("Health") when only one kind along the object's
     * own has it. `ambiguous`, when given, is set when a bare code name is two kinds' --
     * Unit.Flags and Player.Flags on a player -- and null is answered.
     */
    constexpr const Row* Find(uint32_t type, std::string_view name, bool* ambiguous = nullptr)
    {
        if (ambiguous) *ambiguous = false;
        const size_t dot  = name.find('.');
        const Row*   seen = nullptr;
        for (const Row& row : detail::kRows)
        {
            if (!Carries(type, row.kind)) continue;
            if (detail::Same(name, row.name)) return &row;
            if (dot != std::string_view::npos)
            {
                const Kind* kind = KindOf(row.kind);
                if (kind && detail::Same(name.substr(0, dot), kind->name) &&
                    detail::Same(name.substr(dot + 1), row.code))
                    return &row;
                continue;
            }
            if (!detail::Same(name, row.code)) continue;
            if (seen)
            {
                if (ambiguous) *ambiguous = true;
                return nullptr;
            }
            seen = &row;
        }
        return seen;
    }
} // namespace wxl::field

namespace wxl
{
    /**
     * @brief An element of an update field as an event carries it: its dwords, low first.
     *
     * ObjectFieldChanged hands the old and the new value as this; As() reads it as the field
     * says:
     *
     *     void OnObjectFieldChanged(wxl::Object object, wxl_object_type, uint32_t field,
     *                               uint32_t, wxl::FieldValue was, wxl::FieldValue now) override
     *     {
     *         if (field == wxl::field::Unit::Health.index)
     *             Log("%d -> %d", was.As(wxl::field::Unit::Health), now.As(wxl::field::Unit::Health));
     *     }
     */
    class FieldValue
    {
    public:
        constexpr FieldValue() = default;
        constexpr explicit FieldValue(uint64_t raw) : raw_(raw) {}

        /// The dwords as one number, the first in the low half.
        constexpr uint64_t Raw() const { return raw_; }

        /// The value as an element of that field holds it.
        template <class T>
        T As(const field::Field<T>&) const
        {
            T value;
            std::memcpy(&value, &raw_, sizeof value);
            return value;
        }

    private:
        uint64_t raw_ = 0;
    };
} // namespace wxl

#endif // WXL_OBJECTS_FIELDS_HPP
