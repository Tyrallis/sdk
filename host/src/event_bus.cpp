// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/event_bus.hpp"

#include <algorithm>
#include <exception>
#include <string>

namespace wxl::host
{
    const char* EventName(wxl_event event)
    {
        switch (event)
        {
#define WXL_EVENT(family, name, id, args) \
    case WXL_EV_##name: return #name;
#include "wxl/events.def"
#undef WXL_EVENT
        default: return "?";
        }
    }

    std::vector<wxl_event> AllEvents()
    {
        std::vector<wxl_event> events;
        events.reserve(WXL_EV_COUNT);
        for (uint32_t i = 0; i < static_cast<uint32_t>(WXL_EV_COUNT); ++i)
            events.push_back(static_cast<wxl_event>(i));
        return events;
    }

    namespace
    {
        /// One handler call as the guard runs it: the C call, with C++ exceptions caught here.
        struct HandlerCall
        {
            wxl_event_fn  handler;
            void*         user;
            const void*   args;
            wxl_event     event;
            const Logger* logger;
        };

        void InvokeHandler(void* context)
        {
            HandlerCall* call = static_cast<HandlerCall*>(context);
            try
            {
                call->handler(call->user, call->args);
            }
            catch (const std::exception& e)
            {
                if (call->logger)
                    call->logger->Error(kHostTag, std::string("a handler for ") +
                                                      EventName(call->event) + " threw: " + e.what());
            }
            catch (...)
            {
                if (call->logger)
                    call->logger->Error(kHostTag, std::string("a handler for ") +
                                                      EventName(call->event) +
                                                      " threw something that is not a std::exception");
            }
        }
    } // namespace

    EventBus::EventBus() : lists_(WXL_EV_COUNT), raisable_(WXL_EV_COUNT, false) {}

    EventBus::EventBus(const std::vector<wxl_event>& raisable, const Logger* logger) : EventBus()
    {
        logger_ = logger;
        for (const wxl_event event : raisable) SetRaisable(event, true);
    }

    void EventBus::SetRaisable(wxl_event event, bool raisable)
    {
        if (Known(event)) raisable_[event] = raisable;
    }

    bool EventBus::CanRaise(wxl_event event) const
    {
        return Known(event) && raisable_[event];
    }

    void EventBus::Insert(List& list, Entry entry)
    {
        // After every entry of the same priority: ties keep subscription order.
        const auto position =
            std::upper_bound(list.entries.begin(), list.entries.end(), entry,
                             [](const Entry& a, const Entry& b) { return a.priority < b.priority; });
        list.entries.insert(position, entry);
    }

    void EventBus::Settle(List& list)
    {
        if (list.dead)
        {
            list.entries.erase(std::remove_if(list.entries.begin(), list.entries.end(),
                                              [](const Entry& e) { return !e.live; }),
                               list.entries.end());
            list.dead = false;
        }
        for (const Entry& entry : list.pending)
            if (entry.live) Insert(list, entry);
        list.pending.clear();
    }

    wxl_subscription EventBus::Subscribe(wxl_event event, wxl_event_fn handler, void* user,
                                         int32_t priority, uint32_t owner)
    {
        if (!handler || !CanRaise(event)) return WXL_SUBSCRIPTION_NONE;

        List&       list  = lists_[event];
        const Entry entry{++next_, handler, user, priority, owner, true};
        if (list.depth > 0) list.pending.push_back(entry);
        else Insert(list, entry);
        return entry.id;
    }

    void EventBus::Unsubscribe(wxl_subscription subscription)
    {
        if (subscription == WXL_SUBSCRIPTION_NONE) return;

        for (List& list : lists_)
        {
            for (Entry& entry : list.pending)
                if (entry.id == subscription) entry.live = false;

            for (size_t i = 0; i < list.entries.size(); ++i)
            {
                if (list.entries[i].id != subscription) continue;
                if (list.depth > 0)
                {
                    // The dispatch walks this vector by index: mark now, erase when it ends.
                    list.entries[i].live = false;
                    list.dead            = true;
                }
                else
                {
                    list.entries.erase(list.entries.begin() + static_cast<std::ptrdiff_t>(i));
                }
                return;
            }
        }
    }

    void EventBus::DropOwner(uint32_t owner)
    {
        if (owner == 0) return;

        for (List& list : lists_)
        {
            for (Entry& entry : list.pending)
                if (entry.owner == owner) entry.live = false;

            if (list.depth > 0)
            {
                for (Entry& entry : list.entries)
                {
                    if (entry.owner != owner || !entry.live) continue;
                    entry.live = false;
                    list.dead  = true;
                }
            }
            else
            {
                list.entries.erase(std::remove_if(list.entries.begin(), list.entries.end(),
                                                  [owner](const Entry& e) { return e.owner == owner; }),
                                   list.entries.end());
            }
        }
    }

    void EventBus::Emit(wxl_event event, const void* args)
    {
        if (!Known(event)) return;

        List& list = lists_[event];
        ++list.depth;
        const size_t count = list.entries.size(); // entries added meanwhile wait in pending
        for (size_t i = 0; i < count; ++i)
        {
            const Entry& entry = list.entries[i];
            if (!entry.live) continue;

            HandlerCall call{entry.handler, entry.user, args, event, logger_};
            const uint32_t owner = entry.owner;

            uint32_t saved = 0;
            if (ownerSlot_)
            {
                saved       = *ownerSlot_;
                *ownerSlot_ = owner;
            }
            Fault      fault;
            const bool returned = CallGuarded(&InvokeHandler, &call, &fault);
            if (ownerSlot_) *ownerSlot_ = saved;

            if (returned) continue;

            if (logger_)
            {
                char description[96];
                logger_->Error(kHostTag, std::string("a handler for ") + EventName(event) +
                                             " faulted: " +
                                             DescribeFault(fault, description, sizeof description));
            }
            if (faultFn_) faultFn_(faultUser_, owner, event, fault);
        }
        if (--list.depth == 0) Settle(list);
    }

    size_t EventBus::LiveCount(wxl_event event) const
    {
        if (!Known(event)) return 0;
        size_t count = 0;
        for (const Entry& entry : lists_[event].entries)
            if (entry.live) ++count;
        for (const Entry& entry : lists_[event].pending)
            if (entry.live) ++count;
        return count;
    }

    size_t EventBus::LiveCount() const
    {
        size_t count = 0;
        for (uint32_t i = 0; i < static_cast<uint32_t>(WXL_EV_COUNT); ++i)
            count += LiveCount(static_cast<wxl_event>(i));
        return count;
    }
} // namespace wxl::host
