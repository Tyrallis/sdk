// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Traffic.h"

#include "Journal.h"

namespace scout
{
    void Traffic::OnMessageReceived(uint32_t opcode, wxl::Bytes data, wxl::Answer suppressed)
    {
        Record(Direction::Received, opcode, data.size());

        // Setting the answer keeps the message from the client: its own handler never runs.
        if (dropped_.count(opcode) != 0) suppressed.Set();
    }

    void Traffic::OnMessageSent(uint32_t opcode, wxl::Bytes data, wxl::Answer)
    {
        Record(Direction::Sent, opcode, data.size());
    }

    void Traffic::Record(Direction direction, uint32_t opcode, uint32_t size)
    {
        const std::lock_guard<std::mutex> hold(lock_);
        Tally& tally = (direction == Direction::Received ? received_ : sent_)[opcode];
        ++tally.messages;
        tally.bytes += size;
    }

    Tally Traffic::Count(uint32_t opcode, Direction direction) const
    {
        const std::lock_guard<std::mutex> hold(lock_);
        const std::map<uint32_t, Tally>&  side  = direction == Direction::Received ? received_ : sent_;
        const auto                        found = side.find(opcode);
        return found == side.end() ? Tally{} : found->second;
    }

    bool Traffic::Drop(uint32_t opcode, bool drop)
    {
        const bool before = dropped_.count(opcode) != 0;
        if (drop == before) return before;

        if (drop)
            dropped_.insert(opcode);
        else
            dropped_.erase(opcode);

        wxl::Log(drop ? WXL_LOG_WARN : WXL_LOG_INFO, kTag, "opcode 0x{:04X} is {}", opcode,
                 drop ? "now dropped before the client sees it" : "delivered again");
        return before;
    }
} // namespace scout
