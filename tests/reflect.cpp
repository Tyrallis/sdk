// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The field table against the structs it describes: every member listed, in order, of its kind.

#include <cstdio>
#include <cstring>
#include <type_traits>

#include "wxl/reflect.hpp"

namespace
{
    int g_failures = 0;

    void Check(bool condition, const char* what)
    {
        if (condition) return;
        std::printf("  FAIL  %s\n", what);
        ++g_failures;
    }

    using wxl::reflect::Kind;

    /// Whether a member's C type is what its kind says.
    template <Kind K, class T>
    constexpr bool Fits()
    {
        switch (K)
        {
            case Kind::u32: return std::is_same_v<T, uint32_t>;
            case Kind::i32: return std::is_same_v<T, int32_t>;
            case Kind::u64: return std::is_same_v<T, uint64_t>;
            case Kind::i64: return std::is_same_v<T, int64_t>;
            case Kind::f32: return std::is_same_v<T, float>;
            case Kind::boolean: return std::is_same_v<T, wxl_bool>;
            case Kind::handle: return std::is_same_v<T, wxl_handle>;
            case Kind::guid: return std::is_same_v<T, wxl_guid>;
            case Kind::vec3: return std::is_same_v<T, wxl_vec3>;
            case Kind::str: return std::is_same_v<T, wxl_str>;
            case Kind::ptr: return std::is_pointer_v<T>;
            case Kind::bytes: return std::is_same_v<T, const void*>;
            case Kind::answer: return std::is_same_v<T, wxl_bool*>;
            case Kind::buffer: return std::is_same_v<T, char*>;
        }
        return false;
    }

#define WXL_ARGS_FIELD(args, member, kind)                                                       \
    static_assert(Fits<wxl::reflect::detail::kind_##kind, decltype(args::member)>(),        \
                  #args "." #member " is not of kind " #kind);
#define WXL_ARGS_BYTES(args, member, size)                                                 \
    static_assert(Fits<Kind::bytes, decltype(args::member)>(), #args "." #member);          \
    static_assert(std::is_same_v<decltype(args::size), uint32_t>, #args "." #size);
#define WXL_ARGS_BUFFER(args, member, capacity)                                            \
    static_assert(Fits<Kind::buffer, decltype(args::member)>(), #args "." #member);         \
    static_assert(std::is_same_v<decltype(args::capacity), uint32_t>, #args "." #capacity);
#include "wxl/fields.def"
#undef WXL_ARGS_BUFFER
#undef WXL_ARGS_BYTES
#undef WXL_ARGS_FIELD

    /// The size and alignment of each row's member, in the order of the field table.
    struct Shape
    {
        uint32_t size;
        uint32_t align;
    };

    constexpr Shape kShapes[] = {
#define WXL_ARGS_FIELD(args, member, kind) \
    { uint32_t(sizeof(decltype(args::member))), uint32_t(alignof(decltype(args::member))) },
#include "wxl/fields.def"
#undef WXL_ARGS_FIELD
    };

    uint32_t AlignUp(uint32_t value, uint32_t align) { return (value + align - 1) / align * align; }
}

int main()
{
    uint32_t fieldCount = 0, eventCount = 0;
    const wxl::reflect::Field* fields = wxl::reflect::Fields(&fieldCount);
    const wxl::reflect::Event* events = wxl::reflect::Events(&eventCount);
    Check(fieldCount == sizeof kShapes / sizeof kShapes[0], "one shape per field row");

    std::printf("every args struct is fully described\n");
    for (uint32_t e = 0; e < eventCount; ++e)
    {
        const wxl::reflect::Event& event = events[e];
        char what[200];

        std::snprintf(what, sizeof what, "%s has rows in fields.def", event.args);
        Check(event.field_count > 0, what);
        if (event.field_count == 0) continue;

        // Rebuilt from the rows with C's own rule: each member at the next offset its alignment
        // allows. A member left out, swapped or mistyped puts the next one somewhere else.
        const uint32_t first = uint32_t(event.fields - fields);
        uint32_t       end = 0, align = 1;
        bool           laidOut = true;
        for (uint32_t i = 0; i < event.field_count; ++i)
        {
            const Shape& shape = kShapes[first + i];
            if (event.fields[i].offset != AlignUp(end, shape.align)) laidOut = false;
            end   = event.fields[i].offset + shape.size;
            align = shape.align > align ? shape.align : align;
        }
        std::snprintf(what, sizeof what, "%s: the rows are its members, in order", event.args);
        Check(laidOut && AlignUp(end, align) == event.args_size, what);

        for (uint32_t i = 0; i < event.field_count; ++i)
        {
            const wxl::reflect::Field& field = event.fields[i];
            const bool sized = field.kind == Kind::bytes || field.kind == Kind::buffer;
            std::snprintf(what, sizeof what, "%s.%s names its extent exactly when it needs one",
                          event.args, field.name);
            Check(sized == (field.extent != nullptr), what);
        }
    }

    std::printf("the rows of a struct are contiguous\n");
    for (uint32_t i = 0; i < fieldCount; ++i)
        for (uint32_t j = i + 2; j < fieldCount; ++j)
            if (std::strcmp(fields[i].args, fields[j].args) == 0 &&
                std::strcmp(fields[i].args, fields[j - 1].args) != 0)
            {
                char what[200];
                std::snprintf(what, sizeof what, "%s's rows are split", fields[i].args);
                Check(false, what);
            }

    std::printf("lookups\n");
    const wxl::reflect::Event* death = wxl::reflect::Find("Object", "UnitDeath");
    Check(death && death->id == WXL_EV_UnitDeath, "an event is found by family and name");
    Check(wxl::reflect::Find(WXL_EV_MessageSent) &&
              std::strcmp(wxl::reflect::Find(WXL_EV_MessageSent)->family, "Net") == 0,
          "and by id");
    Check(wxl::reflect::Find("Object", "Update") == nullptr, "a name of another family is not");
    const wxl::reflect::Event* sent = wxl::reflect::Find(WXL_EV_MessageSent);
    const wxl::reflect::Field* body = sent ? wxl::reflect::FieldOf(*sent, "data") : nullptr;
    Check(body && body->kind == Kind::bytes && std::strcmp(body->extent, "size") == 0 &&
              body->extent_offset == offsetof(wxl_message_args, size),
          "a bytes field knows where its length is");
    const wxl::reflect::Field* flag = sent ? wxl::reflect::FieldOf(*sent, "suppressed") : nullptr;
    Check(flag && flag->kind == Kind::answer, "and an answer is told apart from a value");

    if (g_failures == 0)
    {
        std::printf("\nall checks passed\n");
        return 0;
    }
    std::printf("\n%d check(s) failed\n", g_failures);
    return 1;
}
