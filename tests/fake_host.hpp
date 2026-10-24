// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// A runtime that does nothing but keep its promises: enough of wxl_host to exercise the contract
// without a client. The conformance suite runs the same checks against it and against each real
// runtime, so a divergence shows up as a failing test rather than as a dialect.

#ifndef WXL_TEST_FAKE_HOST_HPP
#define WXL_TEST_FAKE_HOST_HPP

#include <cstdio>
#include <string>
#include <vector>

#include "wxl/host.h"

namespace wxltest
{
    class FakeHost
    {
    public:
        struct Subscription
        {
            wxl_event        event;
            wxl_event_fn     handler;
            void*            user;
            int32_t          priority;
            wxl_subscription id;
            bool             live;
        };

        struct Published
        {
            std::string name;
            uint32_t    version;
            const void* iface;
        };

        FakeHost()
        {
            current_ = this;

            info_.struct_size = sizeof(info_);
            info_.id          = wxl_str{"fake", 4};
            info_.name        = wxl_str{"Fake runtime", 12};
            info_.version     = 1;
            info_.abi_major   = WXL_ABI_VERSION_MAJOR;
            info_.abi_minor   = WXL_ABI_VERSION_MINOR;
            info_.abi_feature = WXL_ABI_VERSION_FEATURE;

            table_.struct_size     = sizeof(table_);
            table_.abi_major       = WXL_ABI_VERSION_MAJOR;
            table_.abi_minor       = WXL_ABI_VERSION_MINOR;
            table_.log             = &FakeHost::Log;
            table_.runtime         = &FakeHost::Runtime;
            table_.has_service     = &FakeHost::HasService;
            table_.get_service     = &FakeHost::GetService;
            table_.publish_service = &FakeHost::PublishService;
            table_.subscribe       = &FakeHost::Subscribe;
            table_.unsubscribe     = &FakeHost::Unsubscribe;
            table_.emit            = &FakeHost::Emit;
            table_.abi_feature     = WXL_ABI_VERSION_FEATURE;
        }

        const wxl_host* Table() const { return &table_; }

        /// Events this runtime refuses to raise, to exercise the "service is missing" path.
        void Withhold(wxl_event event) { withheld_.push_back(event); }

        /// Raises an event, in priority order, to every live subscription.
        void Raise(wxl_event event, const void* args)
        {
            std::vector<Subscription*> ordered;
            for (Subscription& sub : subscriptions_)
                if (sub.live && sub.event == event) ordered.push_back(&sub);

            for (size_t i = 1; i < ordered.size(); ++i)
                for (size_t j = i; j > 0 && ordered[j - 1]->priority > ordered[j]->priority; --j)
                    std::swap(ordered[j - 1], ordered[j]);

            for (Subscription* sub : ordered) sub->handler(sub->user, args);
        }

        const std::vector<Subscription>& Subscriptions() const { return subscriptions_; }
        const std::vector<std::string>&  LogLines() const { return log_; }

        /// How many times unsubscribe was called, whether or not the token was live.
        size_t UnsubscribeCalls() const { return unsubscribeCalls_; }

        size_t LiveSubscriptions() const
        {
            size_t count = 0;
            for (const Subscription& sub : subscriptions_)
                if (sub.live) ++count;
            return count;
        }

    private:
        static FakeHost* current_;

        static void WXL_CALL Log(wxl_log_level level, const char* tag, wxl_str message)
        {
            current_->log_.push_back(std::string("[") + tag + "] " +
                                     std::string(message.data ? message.data : "", message.size));
            (void)level;
        }

        static const wxl_runtime_info* WXL_CALL Runtime() { return &current_->info_; }

        static wxl_bool WXL_CALL HasService(const char* name, uint32_t version)
        {
            return GetService(name, version) ? WXL_TRUE : WXL_FALSE;
        }

        static const void* WXL_CALL GetService(const char* name, uint32_t version)
        {
            for (const Published& entry : current_->services_)
                if (entry.name == name && entry.version == version) return entry.iface;
            return nullptr;
        }

        static wxl_result WXL_CALL PublishService(const char* name, uint32_t version,
                                                  const void* iface)
        {
            if (!name || !iface) return WXL_E_INVALID;
            if (GetService(name, version)) return WXL_E_INVALID;
            current_->services_.push_back(Published{name, version, iface});
            return WXL_OK;
        }

        static wxl_subscription WXL_CALL Subscribe(wxl_event event, wxl_event_fn handler,
                                                   void* user, int32_t priority)
        {
            if (!handler) return WXL_SUBSCRIPTION_NONE;
            for (wxl_event withheld : current_->withheld_)
                if (withheld == event) return WXL_SUBSCRIPTION_NONE;

            const wxl_subscription id = ++current_->next_id_;
            current_->subscriptions_.push_back(
                Subscription{event, handler, user, priority, id, true});
            return id;
        }

        static void WXL_CALL Unsubscribe(wxl_subscription id)
        {
            ++current_->unsubscribeCalls_;
            for (Subscription& sub : current_->subscriptions_)
                if (sub.id == id) sub.live = false;
        }

        static void WXL_CALL Emit(wxl_event event, const void* args)
        {
            current_->Raise(event, args);
        }

        wxl_host                  table_{};
        wxl_runtime_info          info_{};
        std::vector<Subscription> subscriptions_;
        std::vector<Published>    services_;
        std::vector<std::string>  log_;
        std::vector<wxl_event>    withheld_;
        wxl_subscription          next_id_ = 0;
        size_t                    unsubscribeCalls_ = 0;
    };

    inline FakeHost* FakeHost::current_ = nullptr;
} // namespace wxltest

#endif // WXL_TEST_FAKE_HOST_HPP
