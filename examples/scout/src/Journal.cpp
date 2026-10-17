// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Journal.h"

namespace scout
{
    void Journal::Add(Entry entry)
    {
        entries_.push_back(std::move(entry));
        while (entries_.size() > kLimit) entries_.pop_front();
        ++written_;
        if (listener_) listener_();
    }

    const char* Journal::Describe(What what)
    {
        switch (what)
        {
        case What::Appeared: return "appeared";
        case What::LeftView: return "left";
        case What::Died:     return "died";
        case What::Targeted: return "targeted";
        }
        return "?";
    }
} // namespace scout
