// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Journal.h"

namespace scout
{
    const char* Describe(What what)
    {
        switch (what)
        {
        case What::Appeared: return "appeared";
        case What::Left:     return "left";
        case What::Died:     return "died";
        case What::Targeted: return "targeted";
        }
        return "?";
    }

    void Journal::Add(Entry entry)
    {
        entries_.push_back(std::move(entry));
        if (entries_.size() > kLimit) entries_.pop_front();
        ++written_;
        if (listener_) listener_(entries_.back());
    }

    const Entry* Journal::FromNewest(std::size_t back) const
    {
        if (back >= entries_.size()) return nullptr;
        return &entries_[entries_.size() - 1 - back];
    }
} // namespace scout
