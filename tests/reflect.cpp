// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The field table against the structs it describes: every member listed, in order, of its kind.

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

#include "wxl/reflect.hpp"
#include "wxl/sdk.hpp"

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
#define WXL_ARGS_ENUM(args, member, type)                                                  \
    static_assert(std::is_same_v<decltype(args::member), uint32_t>,                         \
                  #args "." #member " holds an enumeration's values in a uint32_t");        \
    static_assert(std::is_enum_v<type>, #args "." #member ": " #type " is an enum of enums.def");
#include "wxl/fields.def"
#undef WXL_ARGS_ENUM
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

    // A handler's parameters, as the C++ layer derives them from the rows and as a runtime reading
    // the field table does, one letter per parameter: a view or a value of the kind it is.
    template <class T>
    constexpr char Letter()
    {
        if constexpr (std::is_same_v<T, wxl::Bytes>) return 'B';
        else if constexpr (std::is_same_v<T, wxl::Answer>) return 'A';
        else if constexpr (std::is_same_v<T, wxl::Buffer>) return 'F';
        else if constexpr (std::is_same_v<T, std::string_view>) return 's';
        else if constexpr (std::is_same_v<T, bool>) return 'b';
        else if constexpr (std::is_same_v<T, wxl::Object>) return 'o';
        else return 'v';
    }

    template <class Signature>
    struct Letters;
    template <class... P>
    struct Letters<void(P...)>
    {
        static constexpr char text[] = {Letter<P>()..., '\0'};
    };

    /// Per event, in table order: the parameters of its C++ handler.
    constexpr const char* kHandlerLetters[] = {
#define WXL_EVENT(family, name, id, args) Letters<wxl::HandlerOf<WXL_EV_##name>>::text,
#include "wxl/events.def"
#undef WXL_EVENT
    };

    /// events.def in table order, both shapes: a WXL_EVENTS_SINCE row as its first id, an event
    /// as its id, told apart by `since`.
    struct Row
    {
        bool     since;
        uint32_t id;
    };

    constexpr Row kRows[] = {
#define WXL_EVENT(family, name, id, args)              { false, id },
#define WXL_EVENTS_SINCE(major, minor, feature, first) { true, first },
#include "wxl/events.def"
#undef WXL_EVENTS_SINCE
#undef WXL_EVENT
    };

    /// a < b, in the order versions follow each other.
    bool Before(const wxl::reflect::Version& a, const wxl::reflect::Version& b)
    {
        if (a.major != b.major) return a.major < b.major;
        if (a.minor != b.minor) return a.minor < b.minor;
        return a.feature < b.feature;
    }

    char LetterOf(Kind kind)
    {
        switch (kind)
        {
            case Kind::bytes: return 'B';
            case Kind::answer: return 'A';
            case Kind::buffer: return 'F';
            case Kind::str: return 's';
            case Kind::boolean: return 'b';
            case Kind::handle: return 'o';
            default: return 'v';
        }
    }
}

int main()
{
    // Unbuffered, so the last line printed before a crash is never held back in a buffer.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

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

    std::printf("the C++ handlers take the parameters the rows give\n");
    Check(eventCount == sizeof kHandlerLetters / sizeof kHandlerLetters[0], "one handler per event");
    for (uint32_t e = 0; e < eventCount && e < sizeof kHandlerLetters / sizeof kHandlerLetters[0]; ++e)
    {
        const wxl::reflect::Event& event = events[e];
        std::string                fromRows;
        for (uint32_t i = 0; i < event.field_count; ++i)
            if (!wxl::reflect::IsExtent(event, event.fields[i])) fromRows += LetterOf(event.fields[i].kind);
        char what[200];
        std::snprintf(what, sizeof what, "On%s takes %s, the rows give %s", event.name,
                      kHandlerLetters[e], fromRows.c_str());
        Check(fromRows == kHandlerLetters[e], what);
    }
    const wxl::reflect::Event* cast = wxl::reflect::Find(WXL_EV_SpellCastGo);
    const wxl::reflect::Field* size = cast ? wxl::reflect::FieldOf(*cast, "hits_size") : nullptr;
    const wxl::reflect::Field* hits = cast ? wxl::reflect::FieldOf(*cast, "hit_count") : nullptr;
    Check(size && hits && wxl::reflect::IsExtent(*cast, *size) && !wxl::reflect::IsExtent(*cast, *hits),
          "a member another one measures is an extent, a count nobody measures is not");

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

    std::printf("each event knows the version that brought it\n");
    {
        uint32_t                   sinceCount = 0;
        const wxl::reflect::Since* sinces     = wxl::reflect::Sinces(&sinceCount);
        Check(sinceCount > 0 && sinces[0].first == 0, "a WXL_EVENTS_SINCE row covers id 0");
        for (uint32_t i = 1; i < sinceCount; ++i)
        {
            char what[200];
            std::snprintf(what, sizeof what, "WXL_EVENTS_SINCE(%u, %u, %u, %u) comes after the row before it, "
                          "in version and in id", sinces[i].version.major, sinces[i].version.minor,
                          sinces[i].version.feature, sinces[i].first);
            Check(Before(sinces[i - 1].version, sinces[i].version) && sinces[i - 1].first < sinces[i].first, what);
        }
        const wxl::reflect::Version abi{WXL_ABI_VERSION_MAJOR, WXL_ABI_VERSION_MINOR, WXL_ABI_VERSION_FEATURE};
        Check(sinceCount > 0 && !Before(abi, sinces[sinceCount - 1].version),
              "no WXL_EVENTS_SINCE row is newer than abi.h");

        constexpr size_t rowCount = sizeof kRows / sizeof kRows[0];
        uint32_t         seen     = 0;
        for (size_t i = 0; i < rowCount; ++i)
        {
            if (!kRows[i].since) continue;
            char what[200];
            std::snprintf(what, sizeof what, "WXL_EVENTS_SINCE(..., %u) sits right above the event of id %u",
                          kRows[i].id, kRows[i].id);
            Check(i + 1 < rowCount && !kRows[i + 1].since && kRows[i + 1].id == kRows[i].id, what);
            ++seen;
        }
        Check(seen == sinceCount, "one Since per WXL_EVENTS_SINCE row");

        // Below each row, every event has an id at or past its first: an event appended above
        // the row of its version would be counted to an older one.
        uint32_t floor = 0;
        for (size_t i = 0; i < rowCount; ++i)
        {
            if (kRows[i].since)
            {
                floor = kRows[i].id;
                continue;
            }
            char what[200];
            std::snprintf(what, sizeof what, "event %u sits below WXL_EVENTS_SINCE(..., %u), so its id "
                          "is at least that", kRows[i].id, floor);
            Check(kRows[i].id >= floor, what);
        }

        constexpr wxl::reflect::Version first = wxl::reflect::SinceOf(WXL_EV_Update);
        static_assert(first.major == 0 && first.minor == 1 && first.feature == 5,
                      "Update came with the first contract");
        Check(wxl::reflect::SinceOf(WXL_EV_CombatMiss).feature == 5 &&
                  wxl::reflect::SinceOf(WXL_EV_ZoneChanged).feature == 6 &&
                  wxl::reflect::SinceOf(WXL_EV_ChatMessage).feature == 6 &&
                  wxl::reflect::SinceOf(WXL_EV_ChatAddonMessage).feature == 7,
              "CombatMiss came with 0.1.5, ZoneChanged and ChatMessage with 0.1.6, ChatAddonMessage with 0.1.7");
    }

    std::printf("the enumerations name every value once\n");
    {
        uint32_t                  enumCount = 0;
        const wxl::reflect::Enum* enums     = wxl::reflect::Enums(&enumCount);
        Check(enumCount > 0, "enums.def declares enumerations");
        for (uint32_t e = 0; e < enumCount; ++e)
        {
            const wxl::reflect::Enum& one = enums[e];
            char                      what[200];
            std::snprintf(what, sizeof what, "%s has values", one.type);
            Check(one.count > 0, what);
            for (uint32_t i = 0; i < one.count; ++i)
                for (uint32_t j = i + 1; j < one.count; ++j)
                {
                    std::snprintf(what, sizeof what, "%s: %s and %s differ in number and in name",
                                  one.type, one.values[i].constant, one.values[j].constant);
                    Check(one.values[i].value != one.values[j].value &&
                              std::strcmp(one.values[i].name, one.values[j].name) != 0,
                          what);
                }
            std::snprintf(what, sizeof what, "%s is found by its type", one.type);
            Check(wxl::reflect::FindEnum(one.type) == &one, what);
        }
        for (uint32_t i = 0; i < fieldCount; ++i)
        {
            if (!fields[i].enumeration) continue;
            char what[200];
            std::snprintf(what, sizeof what, "%s.%s holds %s in a u32", fields[i].args,
                          fields[i].name, fields[i].enumeration->type);
            Check(fields[i].kind == Kind::u32, what);
        }
        const wxl::reflect::Event* ready   = wxl::reflect::Find(WXL_EV_ScriptVmReady);
        const wxl::reflect::Field* context = ready ? wxl::reflect::FieldOf(*ready, "context") : nullptr;
        Check(context && context->enumeration &&
                  std::strcmp(context->enumeration->type, "wxl_script_context") == 0 &&
                  std::strcmp(wxl::reflect::NameOf(*context->enumeration, WXL_SCRIPT_CONTEXT_GAME),
                              "game") == 0 &&
                  wxl::reflect::NameOf(*context->enumeration, 9) == nullptr,
              "a member knows its enumeration, which names a value and not a number it lacks");
        const wxl::reflect::Event* create = wxl::reflect::Find(WXL_EV_ObjectCreate);
        const wxl::reflect::Field* guid   = create ? wxl::reflect::FieldOf(*create, "guid") : nullptr;
        Check(guid && !guid->enumeration, "a member of another kind has none");
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
