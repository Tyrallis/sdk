// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// A strict JSON reader, sized for the manifest: objects, arrays, strings, integers, booleans and
// null. Anything malformed is refused with a line number. No dependency.

#ifndef WXL_HOST_JSON_HPP
#define WXL_HOST_JSON_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wxl::host::json
{
    enum class Type
    {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    /// The name of a type, for messages: "an object", "a string".
    const char* TypeName(Type type);

    struct Member;

    /**
     * @brief One JSON value, with the line it started on so a schema error can point at it.
     *
     * Object members keep document order, so a report lists problems in the order they are
     * written. Duplicate keys are a parse error.
     */
    struct Value
    {
        Type    type     = Type::Null;
        int     line     = 0;
        bool    boolean  = false;
        bool    integral = false; ///< the number had no fraction and no exponent
        int64_t integer  = 0;     ///< valid when integral
        double  number   = 0.0;   ///< always valid for a Number
        std::string         string;
        std::vector<Value>  array;
        std::vector<Member> object;

        bool Is(Type expected) const { return type == expected; }

        /// A member by key, or nullptr. Only meaningful on an object.
        const Value* Find(std::string_view key) const;
    };

    struct Member
    {
        std::string key;
        Value       value;
    };

    /// Where a parse stopped and why.
    struct Error
    {
        int         line   = 0;
        int         column = 0;
        std::string message;

        /// "line 3, column 14: expected ':' after an object key"
        std::string Text() const;
    };

    /**
     * @brief Parses a whole document.
     *
     * The document is one value followed by nothing but whitespace. A leading UTF-8 byte order
     * mark is skipped. Nesting deeper than 64 levels is refused.
     * @return false with @p error filled in when the text is not JSON.
     */
    bool Parse(std::string_view text, Value& out, Error& error);
} // namespace wxl::host::json

#endif // WXL_HOST_JSON_HPP
