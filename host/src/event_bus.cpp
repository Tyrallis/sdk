// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/event_bus.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <thread>

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

        /// The dispatches the calling thread is inside, innermost first, across every bus.
        struct DispatchFrame
        {
            const void*    bus;
            DispatchFrame* up;
        };

        thread_local DispatchFrame* t_dispatches = nullptr;
    } // namespace

    EventBus::EventBus()
        : lists_(new std::atomic<const Snapshot*>[WXL_EV_COUNT]), raisable_(WXL_EV_COUNT, false),
          current_(WXL_EV_COUNT)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(WXL_EV_COUNT); ++i) lists_[i].store(nullptr);
    }

    EventBus::EventBus(const std::vector<wxl_event>& raisable, const Logger* logger) : EventBus()
    {
        logger_ = logger;
        for (const wxl_event event : raisable) SetRaisable(event, true);
    }

    EventBus::~EventBus() = default;

    void EventBus::SetRaisable(wxl_event event, bool raisable)
    {
        if (Known(event)) raisable_[event] = raisable;
    }

    bool EventBus::CanRaise(wxl_event event) const
    {
        return Known(event) && raisable_[event];
    }

    void EventBus::Publish(wxl_event event, std::unique_ptr<Snapshot> next)
    {
        if (next && next->empty()) next.reset();

        std::unique_ptr<Snapshot> old = std::move(current_[event]);
        current_[event]               = std::move(next);
        lists_[event].store(current_[event].get(), std::memory_order_seq_cst);
        if (old) retiredSnapshots_.push_back(std::move(old));
        Reclaim();
    }

    void EventBus::Reclaim()
    {
        // A dispatch counts itself in before it reads a snapshot, and every publish lands before
        // this load: with nobody counted in, nobody can be holding anything retired so far.
        if (inFlight_.load(std::memory_order_seq_cst) != 0) return;
        retiredSnapshots_.clear();
        retiredEntries_.clear();
    }

    wxl_subscription EventBus::Subscribe(wxl_event event, wxl_event_fn handler, void* user,
                                         int32_t priority, uint32_t owner)
    {
        if (!handler || !CanRaise(event)) return WXL_SUBSCRIPTION_NONE;

        std::lock_guard<std::mutex> lock(writeLock_);

        std::unique_ptr<Entry> entry(new Entry{++next_, handler, user, priority, owner});
        Entry* const           added = entry.get();
        entries_.push_back(std::move(entry));

        std::unique_ptr<Snapshot> next(current_[event] ? new Snapshot(*current_[event]) : new Snapshot);
        // After every entry of the same priority: ties keep subscription order.
        const auto position = std::upper_bound(
            next->begin(), next->end(), added,
            [](const Entry* a, const Entry* b) { return a->priority < b->priority; });
        next->insert(position, added);
        Publish(event, std::move(next));
        return added->id;
    }

    void EventBus::Unsubscribe(wxl_subscription subscription)
    {
        if (subscription == WXL_SUBSCRIPTION_NONE) return;

        std::lock_guard<std::mutex> lock(writeLock_);
        for (uint32_t i = 0; i < static_cast<uint32_t>(WXL_EV_COUNT); ++i)
        {
            const Snapshot* list = current_[i].get();
            if (!list) continue;

            const auto found = std::find_if(list->begin(), list->end(), [subscription](const Entry* e) {
                return e->id == subscription;
            });
            if (found == list->end()) continue;

            Entry* const victim = *found;
            victim->live.store(false, std::memory_order_release);

            std::unique_ptr<Snapshot> next(new Snapshot(*list));
            next->erase(next->begin() + (found - list->begin()));
            for (std::unique_ptr<Entry>& owned : entries_)
            {
                if (owned.get() != victim) continue;
                retiredEntries_.push_back(std::move(owned));
                owned = std::move(entries_.back());
                entries_.pop_back();
                break;
            }
            Publish(static_cast<wxl_event>(i), std::move(next));
            return;
        }
    }

    void EventBus::DropOwner(uint32_t owner)
    {
        if (owner == 0) return;

        std::lock_guard<std::mutex> lock(writeLock_);
        for (uint32_t i = 0; i < static_cast<uint32_t>(WXL_EV_COUNT); ++i)
        {
            const Snapshot* list = current_[i].get();
            if (!list) continue;

            std::unique_ptr<Snapshot> next(new Snapshot);
            for (Entry* entry : *list)
            {
                if (entry->owner == owner) entry->live.store(false, std::memory_order_release);
                else next->push_back(entry);
            }
            if (next->size() != list->size()) Publish(static_cast<wxl_event>(i), std::move(next));
        }

        for (size_t i = 0; i < entries_.size();)
        {
            if (entries_[i]->owner != owner)
            {
                ++i;
                continue;
            }
            retiredEntries_.push_back(std::move(entries_[i]));
            entries_[i] = std::move(entries_.back());
            entries_.pop_back();
        }
        Reclaim();
    }

    void EventBus::Emit(wxl_event event, const void* args)
    {
        if (!Known(event)) return;

        std::atomic<const Snapshot*>& slot = lists_[event];
        if (!slot.load(std::memory_order_relaxed)) return; // nobody listens: no count, no frame

        struct Dispatch
        {
            const EventBus& bus;
            DispatchFrame   frame;

            explicit Dispatch(const EventBus& b) : bus(b), frame{&b, t_dispatches}
            {
                bus.inFlight_.fetch_add(1, std::memory_order_seq_cst);
                t_dispatches = &frame;
            }
            ~Dispatch()
            {
                t_dispatches = frame.up;
                bus.inFlight_.fetch_sub(1, std::memory_order_seq_cst);
            }
        };

        {
            const Dispatch  dispatch(*this);
            const Snapshot* list = slot.load(std::memory_order_seq_cst);
            if (list)
            {
                for (Entry* entry : *list)
                {
                    if (!entry->live.load(std::memory_order_acquire)) continue;

                    HandlerCall    call{entry->handler, entry->user, args, event, logger_};
                    const uint32_t owner = entry->owner;

                    const uint32_t saved = ownerFn_ ? ownerFn_(ownerUser_, owner) : 0;
                    Fault          fault;
                    const bool     returned = CallGuarded(&InvokeHandler, &call, &fault);
                    if (ownerFn_) ownerFn_(ownerUser_, saved);

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
            }
        }

        // The last dispatch out frees what changes made while it ran left behind, unless a
        // change is being made right now, which frees it itself.
        if (inFlight_.load(std::memory_order_relaxed) == 0)
        {
            std::unique_lock<std::mutex> lock(writeLock_, std::try_to_lock);
            if (lock && (!retiredSnapshots_.empty() || !retiredEntries_.empty())) Reclaim();
        }
    }

    uint32_t EventBus::DispatchesOnThisThread() const
    {
        uint32_t count = 0;
        for (const DispatchFrame* frame = t_dispatches; frame; frame = frame->up)
            if (frame->bus == this) ++count;
        return count;
    }

    bool EventBus::WaitForOtherDispatches(std::chrono::milliseconds limit) const
    {
        const uint32_t mine     = DispatchesOnThisThread();
        const auto     deadline = std::chrono::steady_clock::now() + limit;
        while (inFlight_.load(std::memory_order_seq_cst) > mine)
        {
            if (std::chrono::steady_clock::now() >= deadline) return false;
            std::this_thread::yield();
        }
        return true;
    }

    size_t EventBus::LiveCount(wxl_event event) const
    {
        if (!Known(event)) return 0;
        std::lock_guard<std::mutex> lock(writeLock_);
        return current_[event] ? current_[event]->size() : 0;
    }

    size_t EventBus::LiveCount() const
    {
        size_t count = 0;
        for (uint32_t i = 0; i < static_cast<uint32_t>(WXL_EV_COUNT); ++i)
            count += LiveCount(static_cast<wxl_event>(i));
        return count;
    }
} // namespace wxl::host
