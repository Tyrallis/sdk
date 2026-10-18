// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The event bus: subscriptions in priority order, safe to change from inside a handler and from
// any thread, each handler called under the crash guard.

#ifndef WXL_HOST_EVENT_BUS_HPP
#define WXL_HOST_EVENT_BUS_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
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
     * A dispatch walks the list as it was when the dispatch began: a subscription made meanwhile
     * runs from the next dispatch on, and one dropped meanwhile is skipped for the rest of it.
     * An exception leaving a handler is caught here, logged, and the dispatch continues: nothing
     * crosses back into C. A handler that faults is caught by the crash guard, reported to the
     * fault handler, and the dispatch continues past it.
     *
     * Every subscription carries an owner, an opaque token the host assigns to the mod that
     * made it (0 for nobody); @ref DropOwner drops them all at once, during a dispatch included.
     *
     * Any thread may emit, subscribe and unsubscribe. A dispatch takes no lock: each event's list
     * is an immutable snapshot that a change replaces, and a replaced snapshot is freed once no
     * dispatch can still be reading it. Changes are serialised by one lock that no dispatch
     * holds, so a handler may change subscriptions without deadlocking.
     */
    class EventBus
    {
    public:
        /// Told when a handler faulted: whose it was, which event, and what happened.
        using FaultFn = void (*)(void* user, uint32_t owner, wxl_event event, const Fault& fault);

        /// Sets the owner whose code runs on this thread and answers the previous one. Called
        /// around each handler so that what the handler does is attributed to its owner.
        using OwnerFn = uint32_t (*)(void* user, uint32_t owner);

        /// Nothing can be raised until @ref SetRaisable says so.
        EventBus();

        /// @param raisable  the events this runtime raises; a subscribe to any other is refused.
        EventBus(const std::vector<wxl_event>& raisable, const Logger* logger);

        ~EventBus();

        EventBus(const EventBus&)            = delete;
        EventBus& operator=(const EventBus&) = delete;

        /// Configuration, set before the first emit.
        void SetLogger(const Logger* logger) { logger_ = logger; }
        void SetRaisable(wxl_event event, bool raisable);
        bool CanRaise(wxl_event event) const;

        void SetOwnerTracker(OwnerFn swap, void* user)
        {
            ownerFn_   = swap;
            ownerUser_ = user;
        }

        /// Called after a handler faulted, before the dispatch goes on, on the faulting thread.
        /// Null drops the report.
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

        /**
         * @brief Waits until no other thread is inside a dispatch.
         *
         * What makes it safe to free the code of an owner just dropped: a dispatch on another
         * thread may have read the owner's handler before the drop. Dispatches of the calling
         * thread do not count, so it may be called from inside a handler.
         * @return false when the wait ran out; another thread is still dispatching.
         */
        bool WaitForOtherDispatches(std::chrono::milliseconds limit) const;

        /// Live subscriptions to one event, or to all of them.
        size_t LiveCount(wxl_event event) const;
        size_t LiveCount() const;

    private:
        struct Entry
        {
            wxl_subscription  id;
            wxl_event_fn      handler;
            void*             user;
            int32_t           priority;
            uint32_t          owner;
            std::atomic<bool> live{true};
        };

        /// One event's subscriptions, sorted by priority then by id. Never changed once published.
        using Snapshot = std::vector<Entry*>;

        static bool Known(wxl_event event)
        {
            return static_cast<uint32_t>(event) < static_cast<uint32_t>(WXL_EV_COUNT);
        }

        /// Replaces one event's snapshot and frees what no dispatch can still read. Holds
        /// writeLock_.
        void Publish(wxl_event event, std::unique_ptr<Snapshot> next);
        void Reclaim();

        /// How many dispatches of this bus the calling thread is inside.
        uint32_t DispatchesOnThisThread() const;

        std::unique_ptr<std::atomic<const Snapshot*>[]> lists_;
        std::vector<bool>                               raisable_;

        mutable std::mutex                     writeLock_;
        std::vector<std::unique_ptr<Entry>>    entries_;         ///< every entry ever subscribed and not yet freed
        std::vector<std::unique_ptr<Snapshot>> current_;         ///< owns what lists_ points at
        std::vector<std::unique_ptr<Snapshot>> retiredSnapshots_;
        std::vector<std::unique_ptr<Entry>>    retiredEntries_;
        wxl_subscription                       next_ = WXL_SUBSCRIPTION_NONE;

        mutable std::atomic<uint32_t> inFlight_{0}; ///< dispatches running, on every thread

        const Logger* logger_    = nullptr;
        OwnerFn       ownerFn_   = nullptr;
        void*         ownerUser_ = nullptr;
        FaultFn       faultFn_   = nullptr;
        void*         faultUser_ = nullptr;
    };
} // namespace wxl::host

#endif // WXL_HOST_EVENT_BUS_HPP
