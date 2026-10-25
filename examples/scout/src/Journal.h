// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// What Scout remembers: a clock and a bounded journal. Plain C++, with nothing of the SDK in it.

#ifndef SCOUT_JOURNAL_H
#define SCOUT_JOURNAL_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>

namespace scout
{
    /// The tag on every line Scout writes to the log.
    inline constexpr const char* kTag = "scout";

    /// Seconds since the mod started, advanced by the client's tick.
    class Clock
    {
    public:
        void  Advance(float seconds) { now_ += seconds; }
        float Now() const { return now_; }

    private:
        float now_ = 0.0f;
    };

    /// The kinds of thing Scout writes down.
    enum class What
    {
        Appeared,
        Left,
        Died,
        Targeted
    };

    /// The word for a kind, as the log and the interface show it.
    const char* Describe(What what);

    /// One line of the journal.
    struct Entry
    {
        What        what  = What::Appeared;
        std::string who   = "?";  ///< the unit's name, or "?" when it could not be read
        uint32_t    level = 0;    ///< 0 when it could not be read
        float       at    = 0.0f; ///< Clock::Now() when it happened
    };

    /// The last kLimit lines, oldest first. A line added at the limit drops the oldest.
    class Journal
    {
    public:
        static constexpr std::size_t kLimit = 64;

        /// Appends a line, then calls the listener with it.
        void Add(Entry entry);
        void Clear() { entries_.clear(); }

        std::size_t Size() const { return entries_.size(); }

        /// The line @p back steps from the newest, 0 being the newest; null past the oldest.
        const Entry* FromNewest(std::size_t back) const;

        /// How many lines were ever added, the dropped ones included.
        uint32_t Written() const { return written_; }

        /// Sets the function Add calls after each new line.
        void OnAdd(std::function<void(const Entry&)> listener) { listener_ = std::move(listener); }

    private:
        std::deque<Entry>                 entries_;
        uint32_t                          written_ = 0;
        std::function<void(const Entry&)> listener_;
    };
} // namespace scout

#endif // SCOUT_JOURNAL_H
