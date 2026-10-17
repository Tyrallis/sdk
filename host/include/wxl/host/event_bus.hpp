// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The event bus: subscriptions in priority order, safe to change from inside a handler.

#ifndef WXL_HOST_EVENT_BUS_HPP
#define WXL_HOST_EVENT_BUS_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include "wxl/events.h"
#include "wxl/host.h"
#include "wxl/host/log.hpp"

namespace wxl::host
{
    /// The name of an event as events.def spells it, or "?" for a value outside the table.
    const char* EventName(wxl_event event);

    /// Every event in the table, for a runtime that raises all of them.
    std::vector<wxl_event> AllEvents();

    /**
     * @brief Subscriptions per event, dispatched in ascending priority, ties in subscription
     *        order.
     *
     * A subscription made from inside a dispatch of the same event is held back until that
     * dispatch ends. A subscription dropped from inside a dispatch is skipped for the rest of it
     * and erased when it ends. An exception leaving a handler is caught here, logged, and the
     * dispatch continues: nothing crosses back into C.
     *
     * Single-threaded: dispatch and subscription changes happen on the thread that raises.
     */
    class EventBus
    {
    public:
        /// Nothing can be raised until @ref SetRaisable says so.
        EventBus();

        /// @param raisable  the events this runtime raises; a subscribe to any other is refused.
        EventBus(const std::vector<wxl_event>& raisable, const Logger* logger);

        void SetLogger(const Logger* logger) { logger_ = logger; }
        void SetRaisable(wxl_event event, bool raisable);
        bool CanRaise(wxl_event event) const;

        /// @return WXL_SUBSCRIPTION_NONE for a null handler, an unknown event, or one the
        ///         runtime does not raise.
        wxl_subscription Subscribe(wxl_event event, wxl_event_fn handler, void* user,
                                   int32_t priority);

        /// A token that is not live is ignored.
        void Unsubscribe(wxl_subscription subscription);

        void Emit(wxl_event event, const void* args);

        /// Live subscriptions to one event, or to all of them.
        size_t LiveCount(wxl_event event) const;
        size_t LiveCount() const;

    private:
        struct Entry
        {
            wxl_subscription id;
            wxl_event_fn     handler;
            void*            user;
            int32_t          priority;
            bool             live;
        };

        struct List
        {
            std::vector<Entry> entries; ///< sorted by priority, then by id
            std::vector<Entry> pending; ///< subscribed during a dispatch; merged when it ends
            int                depth = 0; ///< nested dispatches in progress
            bool               dead  = false; ///< an entry was dropped during a dispatch
        };

        static bool Known(wxl_event event)
        {
            return static_cast<uint32_t>(event) < static_cast<uint32_t>(WXL_EV_COUNT);
        }

        static void Insert(List& list, Entry entry);
        static void Settle(List& list);

        std::vector<List> lists_;
        std::vector<bool> raisable_;
        const Logger*     logger_ = nullptr;
        wxl_subscription  next_   = WXL_SUBSCRIPTION_NONE;
    };
} // namespace wxl::host

#endif // WXL_HOST_EVENT_BUS_HPP
