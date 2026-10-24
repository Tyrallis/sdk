// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// What every C++ header of the SDK rests on: the runtime table, strings across the boundary, the
// log, and the services resolved through the runtime.
//
// Kept apart from sdk.hpp so that the object classes, which an event handler takes as parameters,
// can be defined before sdk.hpp declares the handlers.

#ifndef WXL_BASE_HPP
#define WXL_BASE_HPP

#include <cstddef>
#include <cstring>
#include <string_view>

#include "wxl/abi.h"
#include "wxl/host.h"

namespace wxl
{
    namespace detail
    {
        /// The runtime table, set once by WXL_MOD before any of the mod's own code runs.
        inline const wxl_host*& HostRef()
        {
            static const wxl_host* host = nullptr;
            return host;
        }
    } // namespace detail

    /// The runtime that loaded this mod.
    inline const wxl_host* Host() { return detail::HostRef(); }

    /// Wraps a NUL-terminated string for the boundary.
    inline wxl_str Str(const char* text)
    {
        return wxl_str{text, text ? static_cast<uint32_t>(std::strlen(text)) : 0u};
    }

    /// Wraps a view for the boundary. The view's storage must outlive the call it is given to.
    inline wxl_str Str(std::string_view text)
    {
        return wxl_str{text.data(), static_cast<uint32_t>(text.size())};
    }

    /// The view over a boundary string. Empty when the runtime answered with nothing.
    inline std::string_view View(wxl_str text)
    {
        return text.data ? std::string_view(text.data, text.size) : std::string_view();
    }

    /// Writes one line to the runtime's log.
    inline void Log(wxl_log_level level, const char* tag, std::string_view message)
    {
        if (const wxl_host* host = Host()) host->log(level, tag, Str(message));
    }

    /// The same for a NUL-terminated string; a null message logs an empty line.
    inline void Log(wxl_log_level level, const char* tag, const char* message)
    {
        if (const wxl_host* host = Host()) host->log(level, tag, Str(message));
    }

    // --- Services ------------------------------------------------------------------------

    /**
     * @brief A raw service table, resolved on first use.
     *
     * Load order is not something a mod can rely on, so nothing is looked up until it is needed,
     * and a lookup that finds nothing is retried on the next use. The typed facades in
     * wxl/services/<service>.hpp wrap one of these with the name and version sealed in; this is for a
     * table those do not cover, or for reaching a raw table directly.
     *
     *     static wxl::Service<my_api> g_mine("my.service", 1);
     *     if (g_mine) g_mine->call();
     */
    template <class T>
    class Service
    {
    public:
        constexpr Service(const char* name, uint32_t version) : name_(name), version_(version) {}

        /// The table, or nullptr while nobody has published it under that name and version.
        const T* Get() const
        {
            if (!iface_)
            {
                if (const wxl_host* host = Host())
                    iface_ = static_cast<const T*>(host->get_service(name_, version_));
            }
            return iface_;
        }

        const T* operator->() const { return Get(); }
        explicit operator bool() const { return Get() != nullptr; }

        constexpr const char* Name() const { return name_; }
        constexpr uint32_t    Version() const { return version_; }

    private:
        const char*      name_;
        uint32_t         version_;
        mutable const T* iface_ = nullptr;
    };

    namespace detail
    {
        /**
         * @brief Shared shape of every typed facade: a stateless object over one cached table.
         *
         * Derived names the table type and carries `kName` and `kVersion`. The table is resolved
         * on first use and shared by every instance, so a facade can be a temporary.
         */
        template <class Derived, class Api>
        class Facade
        {
        public:
            using api_type = Api;

            /// The raw table, or nullptr while the runtime does not publish the service.
            static const Api* Raw()
            {
                static Service<Api> service(Derived::kName, Derived::kVersion);
                return service.Get();
            }

            /// Whether the runtime publishes the service right now.
            static bool Available() { return Raw() != nullptr; }

            explicit operator bool() const { return Available(); }
        };
    } // namespace detail

    namespace detail
    {
        /**
         * @brief Whether a service table really answers for one of its members.
         *
         * Two ways it may not. The runtime's table can be older than this header, and stop
         * short of the member: struct_size says how far it reaches. Or the runtime can reach
         * the member and have nothing true to say through it, and leave it null -- which the
         * contract allows, and which is how a runtime tells a mod that one row of a service is
         * beyond it without refusing the whole service.
         *
         * Either way the answer is the same to a mod: unsupported.
         */
        template <class Api, class Member>
        bool Carries(const Api* api, Member Api::*member)
        {
            if (!api) return false;
            const char* base = reinterpret_cast<const char*>(api);
            const char* end  = reinterpret_cast<const char*>(&(api->*member)) + sizeof(Member);
            if (static_cast<size_t>(end - base) > api->struct_size) return false;
            return (api->*member) != nullptr;
        }
    } // namespace detail
} // namespace wxl

#endif // WXL_BASE_HPP
