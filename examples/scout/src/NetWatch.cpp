// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Watchers.h"

#include <cstdio>

namespace scout
{
    void NetWatch::OnMessageReceived(const wxl_message_args& args)
    {
        ++incoming_[args.opcode];

        // suppressed is a pointer, not a value: the runtime is asking, not telling. Writing
        // WXL_TRUE through it means the client never sees this message at all -- its own
        // handler does not run. That is how a mod serves an opcode the client does not know;
        // it is also how a mod breaks the client, so it only happens on request.
        if (!IsMuted(args.opcode) || !args.suppressed) return;
        *args.suppressed = WXL_TRUE;
    }

    void NetWatch::OnMessageSent(const wxl_message_args& args)
    {
        // Counted, never muted. Dropping something the client chose to say leaves the server
        // with a different idea of the world than the client has, and nothing says which.
        ++outgoing_[args.opcode];
    }

    uint32_t NetWatch::Count(uint32_t opcode, bool incoming) const
    {
        const std::map<uint32_t, uint32_t>& side = incoming ? incoming_ : outgoing_;
        const auto                          at   = side.find(opcode);
        return at == side.end() ? 0u : at->second;
    }

    bool NetWatch::Mute(uint32_t opcode, bool muted)
    {
        const bool before = IsMuted(opcode);
        if (muted)
            muted_.insert(opcode);
        else
            muted_.erase(opcode);

        if (before != muted)
        {
            char line[96];
            std::snprintf(line, sizeof line, "opcode 0x%04X is now %s", opcode,
                          muted ? "dropped before the client sees it" : "delivered again");
            wxl::Log(muted ? WXL_LOG_WARN : WXL_LOG_INFO, kTag, line);
        }
        return before;
    }
} // namespace scout
