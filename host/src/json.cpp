// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/json.hpp"

#include <cerrno>
#include <cstdlib>

namespace wxl::host::json
{
    const char* TypeName(Type type)
    {
        switch (type)
        {
        case Type::Null: return "null";
        case Type::Bool: return "a boolean";
        case Type::Number: return "a number";
        case Type::String: return "a string";
        case Type::Array: return "an array";
        case Type::Object: return "an object";
        }
        return "a value";
    }

    const Value* Value::Find(std::string_view key) const
    {
        for (const Member& member : object)
            if (member.key == key) return &member.value;
        return nullptr;
    }

    std::string Error::Text() const
    {
        return "line " + std::to_string(line) + ", column " + std::to_string(column) + ": " +
               message;
    }

    namespace
    {
        constexpr int kMaxDepth = 64;

        class Parser
        {
        public:
            Parser(std::string_view text, Error& error) : text_(text), error_(error) {}

            bool ParseDocument(Value& out)
            {
                // A byte order mark is not JSON, but editors write one; skip it.
                if (text_.size() >= 3 && text_[0] == '\xEF' && text_[1] == '\xBB' &&
                    text_[2] == '\xBF')
                    pos_ = 3;

                SkipWhitespace();
                if (AtEnd()) return Fail("the document is empty");
                if (!ParseValue(out, 0)) return false;
                SkipWhitespace();
                if (!AtEnd()) return Fail("unexpected text after the document's value");
                return true;
            }

        private:
            bool AtEnd() const { return pos_ >= text_.size(); }
            char Peek() const { return text_[pos_]; }
            bool PeekDigit() const { return !AtEnd() && Peek() >= '0' && Peek() <= '9'; }

            void Advance()
            {
                if (text_[pos_] == '\n')
                {
                    ++line_;
                    column_ = 0;
                }
                ++pos_;
                ++column_;
            }

            bool Fail(const std::string& message)
            {
                error_.line    = line_;
                error_.column  = column_;
                error_.message = message;
                return false;
            }

            void SkipWhitespace()
            {
                while (!AtEnd())
                {
                    const char c = Peek();
                    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') return;
                    Advance();
                }
            }

            bool ParseValue(Value& out, int depth)
            {
                if (depth > kMaxDepth) return Fail("nesting deeper than 64 levels");
                if (AtEnd()) return Fail("expected a value but the document ended");

                out.line = line_;
                switch (Peek())
                {
                case '{': return ParseObject(out, depth);
                case '[': return ParseArray(out, depth);
                case '"':
                    out.type = Type::String;
                    return ParseString(out.string);
                case 't': return ParseLiteral("true", out, Type::Bool, true);
                case 'f': return ParseLiteral("false", out, Type::Bool, false);
                case 'n': return ParseLiteral("null", out, Type::Null, false);
                default:
                    if (Peek() == '-' || PeekDigit()) return ParseNumber(out);
                    return Fail(std::string("unexpected character '") + Peek() + "'");
                }
            }

            bool ParseLiteral(const char* word, Value& out, Type type, bool truth)
            {
                for (const char* p = word; *p; ++p)
                {
                    if (AtEnd() || Peek() != *p) return Fail(std::string("expected '") + word + "'");
                    Advance();
                }
                out.type    = type;
                out.boolean = truth;
                return true;
            }

            bool ParseObject(Value& out, int depth)
            {
                out.type = Type::Object;
                Advance(); // '{'
                SkipWhitespace();
                if (!AtEnd() && Peek() == '}')
                {
                    Advance();
                    return true;
                }

                for (;;)
                {
                    SkipWhitespace();
                    if (AtEnd() || Peek() != '"') return Fail("expected a quoted object key");

                    Member    member;
                    const int keyLine   = line_;
                    const int keyColumn = column_;
                    if (!ParseString(member.key)) return false;
                    if (out.Find(member.key))
                    {
                        line_   = keyLine;
                        column_ = keyColumn;
                        return Fail("duplicate key \"" + member.key + "\"");
                    }

                    SkipWhitespace();
                    if (AtEnd() || Peek() != ':') return Fail("expected ':' after an object key");
                    Advance();
                    SkipWhitespace();
                    if (!ParseValue(member.value, depth + 1)) return false;
                    out.object.push_back(std::move(member));

                    SkipWhitespace();
                    if (AtEnd()) return Fail("expected ',' or '}' but the document ended");
                    if (Peek() == ',')
                    {
                        Advance();
                        continue;
                    }
                    if (Peek() == '}')
                    {
                        Advance();
                        return true;
                    }
                    return Fail("expected ',' or '}' after an object member");
                }
            }

            bool ParseArray(Value& out, int depth)
            {
                out.type = Type::Array;
                Advance(); // '['
                SkipWhitespace();
                if (!AtEnd() && Peek() == ']')
                {
                    Advance();
                    return true;
                }

                for (;;)
                {
                    SkipWhitespace();
                    Value element;
                    if (!ParseValue(element, depth + 1)) return false;
                    out.array.push_back(std::move(element));

                    SkipWhitespace();
                    if (AtEnd()) return Fail("expected ',' or ']' but the document ended");
                    if (Peek() == ',')
                    {
                        Advance();
                        continue;
                    }
                    if (Peek() == ']')
                    {
                        Advance();
                        return true;
                    }
                    return Fail("expected ',' or ']' after an array element");
                }
            }

            bool ParseHex4(uint32_t& out)
            {
                out = 0;
                for (int i = 0; i < 4; ++i)
                {
                    if (AtEnd()) return Fail("\\u escape cut short");
                    const char c = Peek();
                    uint32_t   digit;
                    if (c >= '0' && c <= '9') digit = static_cast<uint32_t>(c - '0');
                    else if (c >= 'a' && c <= 'f') digit = static_cast<uint32_t>(c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') digit = static_cast<uint32_t>(c - 'A' + 10);
                    else return Fail("\\u escape needs four hex digits");
                    out = (out << 4) | digit;
                    Advance();
                }
                return true;
            }

            static void AppendUtf8(std::string& out, uint32_t code)
            {
                if (code < 0x80)
                {
                    out.push_back(static_cast<char>(code));
                }
                else if (code < 0x800)
                {
                    out.push_back(static_cast<char>(0xC0 | (code >> 6)));
                    out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                }
                else if (code < 0x10000)
                {
                    out.push_back(static_cast<char>(0xE0 | (code >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                }
                else
                {
                    out.push_back(static_cast<char>(0xF0 | (code >> 18)));
                    out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                }
            }

            bool ParseString(std::string& out)
            {
                Advance(); // opening quote
                for (;;)
                {
                    if (AtEnd()) return Fail("unterminated string");
                    const char c = Peek();
                    if (c == '"')
                    {
                        Advance();
                        return true;
                    }
                    if (static_cast<unsigned char>(c) < 0x20)
                        return Fail("control character in a string; escape it");
                    if (c != '\\')
                    {
                        out.push_back(c);
                        Advance();
                        continue;
                    }

                    Advance(); // backslash
                    if (AtEnd()) return Fail("unterminated escape");
                    const char e = Peek();
                    Advance();
                    switch (e)
                    {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u':
                    {
                        uint32_t code;
                        if (!ParseHex4(code)) return false;
                        if (code >= 0xD800 && code <= 0xDBFF)
                        {
                            // A high surrogate must be followed by an escaped low one.
                            if (AtEnd() || Peek() != '\\') return Fail("lone high surrogate");
                            Advance();
                            if (AtEnd() || Peek() != 'u') return Fail("lone high surrogate");
                            Advance();
                            uint32_t low;
                            if (!ParseHex4(low)) return false;
                            if (low < 0xDC00 || low > 0xDFFF) return Fail("lone high surrogate");
                            code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                        }
                        else if (code >= 0xDC00 && code <= 0xDFFF)
                        {
                            return Fail("lone low surrogate");
                        }
                        AppendUtf8(out, code);
                        break;
                    }
                    default: return Fail(std::string("unknown escape '\\") + e + "'");
                    }
                }
            }

            bool ParseNumber(Value& out)
            {
                const size_t start    = pos_;
                bool         integral = true;

                if (Peek() == '-') Advance();
                if (!PeekDigit()) return Fail("expected a digit");
                if (Peek() == '0')
                {
                    Advance();
                    if (PeekDigit()) return Fail("a number may not start with a zero");
                }
                else
                {
                    while (PeekDigit()) Advance();
                }

                if (!AtEnd() && Peek() == '.')
                {
                    integral = false;
                    Advance();
                    if (!PeekDigit()) return Fail("expected a digit after '.'");
                    while (PeekDigit()) Advance();
                }

                if (!AtEnd() && (Peek() == 'e' || Peek() == 'E'))
                {
                    integral = false;
                    Advance();
                    if (!AtEnd() && (Peek() == '+' || Peek() == '-')) Advance();
                    if (!PeekDigit()) return Fail("expected a digit in the exponent");
                    while (PeekDigit()) Advance();
                }

                const std::string lexeme(text_.substr(start, pos_ - start));
                out.type     = Type::Number;
                out.integral = integral;
                out.number   = std::strtod(lexeme.c_str(), nullptr);
                if (integral)
                {
                    errno       = 0;
                    out.integer = std::strtoll(lexeme.c_str(), nullptr, 10);
                    if (errno == ERANGE) return Fail("integer out of range");
                }
                return true;
            }

            std::string_view text_;
            Error&           error_;
            size_t           pos_    = 0;
            int              line_   = 1;
            int              column_ = 1;
        };
    } // namespace

    bool Parse(std::string_view text, Value& out, Error& error)
    {
        out = Value{};
        Parser parser(text, error);
        return parser.ParseDocument(out);
    }
} // namespace wxl::host::json
