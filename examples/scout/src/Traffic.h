// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#ifndef SCOUT_TRAFFIC_H
#define SCOUT_TRAFFIC_H

#include <cstdint>
#include <map>
#include <mutex>
#include <set>

#include "wxl/sdk.hpp"

namespace scout
{
    /// Which way a message went.
    enum class Direction
    {
        Received,
        Sent
    };

    /// How much of one opcode went one way.
    struct Tally
    {
        uint32_t messages = 0;
        uint64_t bytes    = 0;
    };

    /**
     * @brief Counts the messages between the client and the server, by opcode and direction,
     *        and drops the incoming opcodes it is asked to.
     *
     * OnMessageSent may run on the connection's own thread, so the tallies are read and written
     * under a lock. The dropped set is only touched on the main thread: by OnMessageReceived
     * and by Drop.
     */
    class Traffic final : public wxl::NetScript
    {
    public:
        void OnMessageReceived(uint32_t opcode, wxl::Bytes data, wxl::Answer suppressed) override;
        void OnMessageSent(uint32_t opcode, wxl::Bytes data, wxl::Answer suppressed) override;

        /// What has crossed so far for that opcode, that way.
        Tally Count(uint32_t opcode, Direction direction) const;

        /// Starts or stops dropping an incoming opcode. Answers whether it was dropped before.
        bool Drop(uint32_t opcode, bool drop);

    private:
        void Record(Direction direction, uint32_t opcode, uint32_t size);

        mutable std::mutex        lock_;
        std::map<uint32_t, Tally> received_; ///< under lock_
        std::map<uint32_t, Tally> sent_;     ///< under lock_
        std::set<uint32_t>        dropped_;  ///< main thread only
    };
} // namespace scout

#endif // SCOUT_TRAFFIC_H
