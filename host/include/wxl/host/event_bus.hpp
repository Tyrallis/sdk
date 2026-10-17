// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The event bus: subscriptions in priority order, safe to change from inside a handler, each
// handler called under the crash guard.

#ifndef WXL_HOST_EVENT_BUS_HPP
#define WXL_HOST_EVENT_BUS_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include "wxl/events.h"
#include "wxl/host.h"
#include "wxl/host/guard.hpp"
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
     * dispatch continues: nothing crosses back into C. A handler that faults is caught by the
     * crash guard, reported to the fault handler, and the dispatch continues past it.
     *
     * Every subscription carries an owner, an opaque token the host assigns to the mod that
     * made it (0 for nobody); @ref DropOwner drops them all at once, during a dispatch included.
     *
     * Single-threaded: dispatch and subscription changes happen on the thread that raises.
     */
    class EventBus
    {
    public:
        /// Told when a handler faulted: whose it was, which event, and what happened.
        using FaultFn = void (*)(void* user, uint32_t owner, wxl_event event, const Fault& fault);

        /// Nothing can be raised until @ref SetRaisable says so.
        EventBus();

        /// @param raisable  the events this runtime raises; a subscribe to any other is refused.
        EventBus(const std::vector<wxl_event>& raisable, const Logger* logger);

        void SetLogger(const Logger* logger) { logger_ = logger; }
        void SetRaisable(wxl_event event, bool raisable);
        bool CanRaise(wxl_event event) const;

        /// Where the host keeps the owner whose code is running: set to each handler's owner
        /// around its call and put back after, so a subscribe made inside is attributed right.
        void SetOwnerSlot(uint32_t* slot) { ownerSlot_ = slot; }

        /// Called after a handler faulted, before the dispatch goes on. Null drops the report.
        void SetFaultHandler(FaultFn handler, void* user)
        {
            faultFn_   = handler;
            faultUser_ = user;
        }

        /// @return WXL_SUBSCRIPTION_NONE for a null handler, an unknown event, or one the
        ///         runtime does not raise.
        wxl_subscription Subscribe(wxl_event event, wxl_event_fn handler, void* user,
                                   int32_t priority, uint32_t owner = 0);

        /// A token that is not live is ignored.
        void Unsubscribe(wxl_subscription subscription);

        /// Drops every live subscription of an owner. Safe inside a dispatch: the rest of it
        /// skips them.
        void DropOwner(uint32_t owner);

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
            uint32_t         owner;
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
        const Logger*     logger_    = nullptr;
        uint32_t*         ownerSlot_ = nullptr;
        FaultFn           faultFn_   = nullptr;
        void*             faultUser_ = nullptr;
        wxl_subscription  next_      = WXL_SUBSCRIPTION_NONE;
    };
} // namespace wxl::host

#endif // WXL_HOST_EVENT_BUS_HPP
