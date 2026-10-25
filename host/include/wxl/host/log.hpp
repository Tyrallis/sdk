// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// Logging: the library formats and routes, the runtime decides where a line ends up.

#ifndef WXL_HOST_LOG_HPP
#define WXL_HOST_LOG_HPP

#include <string>
#include <string_view>

#include "wxl/host.h"

namespace wxl::host
{
    /// The tag the library logs its own lines under.
    constexpr const char* kHostTag = "host";

    /**
     * @brief Where lines go. A runtime implements this once: a file, a console, the client's own
     *        log.
     *
     * Write is called from any thread, at the same time from two of them when a handler logs off
     * the main thread: the sink keeps each line whole (wxl_host.log promises it to mods).
     */
    class LogSink
    {
    public:
        virtual ~LogSink() = default;

        /// One line, already filtered by level. @p tag and @p message are UTF-8 and not
        /// NUL-terminated.
        virtual void Write(wxl_log_level level, std::string_view tag, std::string_view message) = 0;
    };

    /// A fixed-width name for a level: "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR".
    const char* LevelName(wxl_log_level level);

    /// "[INFO ] [tag] message" -- the shape a sink may use when it has no format of its own.
    std::string FormatLine(wxl_log_level level, std::string_view tag, std::string_view message);

    /// Whether @p text is text a log file can hold as it is: well-formed UTF-8 (no stray
    /// continuation byte, no cut or overlong sequence, no surrogate, nothing past U+10FFFF) with
    /// no control character other than a tab, a line feed or a carriage return.
    bool IsLogText(std::string_view text);

    /// @p text with every byte IsLogText refuses written as `\xNN`: a line a mod hands over keeps
    /// the log file valid UTF-8 text and still says which bytes it held.
    std::string EscapeForLog(std::string_view text);

    /**
     * @brief Routes lines to a sink, dropping those below a minimum level.
     *
     * A null sink drops everything. The sink is not owned.
     */
    class Logger
    {
    public:
        Logger() = default;
        Logger(LogSink* sink, wxl_log_level minimum) : sink_(sink), minimum_(minimum) {}

        void SetSink(LogSink* sink) { sink_ = sink; }
        void SetMinimum(wxl_log_level minimum) { minimum_ = minimum; }

        LogSink*      Sink() const { return sink_; }
        wxl_log_level Minimum() const { return minimum_; }

        bool Enabled(wxl_log_level level) const { return sink_ && level >= minimum_; }

        /// Hands the line to the sink, with any byte of @p tag or @p message that is not text
        /// escaped (EscapeForLog): a sink only ever receives UTF-8 text.
        void Write(wxl_log_level level, std::string_view tag, std::string_view message) const
        {
            if (!Enabled(level)) return;
            if (IsLogText(tag) && IsLogText(message))
                sink_->Write(level, tag, message);
            else
                sink_->Write(level, EscapeForLog(tag), EscapeForLog(message));
        }

        void Debug(std::string_view tag, std::string_view message) const
        {
            Write(WXL_LOG_DEBUG, tag, message);
        }
        void Info(std::string_view tag, std::string_view message) const
        {
            Write(WXL_LOG_INFO, tag, message);
        }
        void Warn(std::string_view tag, std::string_view message) const
        {
            Write(WXL_LOG_WARN, tag, message);
        }
        void Error(std::string_view tag, std::string_view message) const
        {
            Write(WXL_LOG_ERROR, tag, message);
        }

    private:
        LogSink*      sink_    = nullptr;
        wxl_log_level minimum_ = WXL_LOG_TRACE;
    };
} // namespace wxl::host

#endif // WXL_HOST_LOG_HPP
