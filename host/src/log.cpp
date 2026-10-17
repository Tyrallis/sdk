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
} // namespace wxl::host
