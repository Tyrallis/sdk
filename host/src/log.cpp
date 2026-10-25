// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/log.hpp"

namespace wxl::host
{
    const char* LevelName(wxl_log_level level)
    {
        switch (level)
        {
        case WXL_LOG_TRACE: return "TRACE";
        case WXL_LOG_DEBUG: return "DEBUG";
        case WXL_LOG_INFO: return "INFO ";
        case WXL_LOG_WARN: return "WARN ";
        case WXL_LOG_ERROR: return "ERROR";
        }
        return "?????";
    }

    std::string FormatLine(wxl_log_level level, std::string_view tag, std::string_view message)
    {
        std::string line;
        line.reserve(tag.size() + message.size() + 12);
        line += '[';
        line += LevelName(level);
        line += "] [";
        line += tag;
        line += "] ";
        line += message;
        return line;
    }

    namespace
    {
        /// A control character a log line keeps: tab, line feed, carriage return.
        bool KeptControl(unsigned char byte) { return byte == '\t' || byte == '\n' || byte == '\r'; }

        /// The length of the text at @p at: a printable ASCII byte or a kept control is 1, a
        /// well-formed UTF-8 sequence its length (RFC 3629: shortest form, no surrogates, at most
        /// U+10FFFF); 0 when the byte there is neither.
        size_t SequenceAt(std::string_view text, size_t at)
        {
            const auto byte = [&](size_t i) { return static_cast<unsigned char>(text[i]); };
            const unsigned char lead = byte(at);
            if (lead < 0x20 || lead == 0x7F) return KeptControl(lead) ? 1 : 0;
            if (lead < 0x80) return 1;
            size_t length;
            unsigned char low = 0x80, high = 0xBF; // the range of the second byte
            if (lead >= 0xC2 && lead <= 0xDF) length = 2;
            else if (lead == 0xE0) length = 3, low = 0xA0;
            else if (lead == 0xED) length = 3, high = 0x9F;
            else if (lead >= 0xE1 && lead <= 0xEF) length = 3;
            else if (lead == 0xF0) length = 4, low = 0x90;
            else if (lead == 0xF4) length = 4, high = 0x8F;
            else if (lead >= 0xF1 && lead <= 0xF3) length = 4;
            else return 0;
            if (text.size() - at < length) return 0;
            if (byte(at + 1) < low || byte(at + 1) > high) return 0;
            for (size_t i = 2; i < length; ++i)
                if (byte(at + i) < 0x80 || byte(at + i) > 0xBF) return 0;
            return length;
        }
    } // namespace

    bool IsLogText(std::string_view text)
    {
        for (size_t at = 0; at < text.size();)
        {
            const auto byte = static_cast<unsigned char>(text[at]);
            if (byte >= 0x20 && byte < 0x7F)
            {
                ++at;
                continue;
            }
            const size_t length = SequenceAt(text, at);
            if (length == 0) return false;
            at += length;
        }
        return true;
    }

    std::string EscapeForLog(std::string_view text)
    {
        static const char kHex[] = "0123456789ABCDEF";
        std::string out;
        out.reserve(text.size() + 16);
        for (size_t at = 0; at < text.size();)
        {
            const size_t length = SequenceAt(text, at);
            if (length > 0)
            {
                out.append(text.data() + at, length);
                at += length;
                continue;
            }
            const auto value = static_cast<unsigned char>(text[at++]);
            out += "\\x";
            out += kHex[value >> 4];
            out += kHex[value & 0xF];
        }
        return out;
    }
} // namespace wxl::host
