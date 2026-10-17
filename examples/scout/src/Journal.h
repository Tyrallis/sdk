// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// What Scout remembers. There is not one line of WarcraftXL in this file on purpose: a mod is
// ordinary C++, and the part of it that holds your own data should not know the SDK exists.

#ifndef SCOUT_JOURNAL_H
#define SCOUT_JOURNAL_H

#include <cstdint>
#include <deque>
#include <functional>
#include <string>

namespace scout
{
    /// Seconds since the mod started, advanced one frame at a time.
    class Clock
    {
    public:
        void  Advance(float seconds) { now_ += seconds; }
        float Now() const { return now_; }

    private:
        float now_ = 0.0f;
    };

    /// The kinds of thing worth writing down.
    enum class What
    {
        Appeared,
        LeftView,
        Died,
        Targeted
    };

    /// One line of the journal.
    struct Entry
    {
        What        what  = What::Appeared;
        std::string who   = "?"; ///< the unit's name, or "?" when it could not be read
        uint32_t    level = 0;
        float       at    = 0.0f; ///< Clock::Now() when it happened
    };

    /**
     * @brief The last few things that happened, oldest first.
     *
     * Bounded: a long session in a busy zone would otherwise grow without end, and nobody reads
     * the thousandth entry. The oldest line falls off when a new one arrives.
     */
    class Journal
    {
    public:
        static constexpr size_t kLimit = 64;

        void Add(Entry entry);
        void Clear() { entries_.clear(); }

        size_t Size() const { return entries_.size(); }
        bool   Empty() const { return entries_.empty(); }

        /// The most recent line, or nullptr while nothing has happened.
        const Entry* Last() const { return entries_.empty() ? nullptr : &entries_.back(); }

        const std::deque<Entry>& Entries() const { return entries_; }

        /// How many lines have ever been written, including the ones that fell off.
        uint32_t Written() const { return written_; }

        /// A word for a kind, for the log and for script.
        static const char* Describe(What what);

        /// Called after every Add. The journal does not know who listens, and does not care.
        void OnChange(std::function<void()> listener) { listener_ = std::move(listener); }

    private:
        std::deque<Entry>     entries_;
        uint32_t              written_ = 0;
        std::function<void()> listener_;
    };
} // namespace scout

#endif // SCOUT_JOURNAL_H
