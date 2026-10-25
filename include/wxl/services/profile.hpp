// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.profile: zones, graphs and messages on the profiler's timeline.

#ifndef WXL_SERVICE_PROFILE_HPP
#define WXL_SERVICE_PROFILE_HPP

#include <atomic>
#include <cstdint>
#include <string_view>

#include "wxl/sdk.hpp"
#include "wxl/services/profile.h"
#include "wxl/services/rows.hpp"

namespace wxl::api::facade
{
    template <>
    struct Methods<wxl_profile_api>
    {
#define WXL_API_TABLE "wxl/services/profile.def"
#include "wxl/services/methods.inc"
    };
} // namespace wxl::api::facade

namespace wxl
{
    namespace profile
    {
        /**
         * @brief The runtime's table, looked up once.
         *
         * The runtime publishes wxl.profile before any mod loads, or not at all, so an answer
         * of null is kept too: a zone in a runtime built without the profiler costs one load.
         * Null until the mod is loaded.
         */
        inline const wxl_profile_api* Api()
        {
            static std::atomic<const wxl_profile_api*> table{nullptr};
            static std::atomic<bool>                   looked{false};
            if (looked.load(std::memory_order_acquire)) return table.load(std::memory_order_relaxed);
            const wxl_host* host = Host();
            if (!host) return nullptr;
            const auto* found =
                static_cast<const wxl_profile_api*>(host->get_service(WXL_PROFILE_SERVICE, WXL_PROFILE_SERVICE_VERSION));
            if (found && !detail::Carries(found, &wxl_profile_api::zone_end)) found = nullptr;
            table.store(found, std::memory_order_relaxed);
            looked.store(true, std::memory_order_release);
            return found;
        }

        /**
         * @brief A place in the mod's source that zones are named after.
         *
         * Constant: it holds the strings it was given and asks the runtime for its location the
         * first time a zone opens there. WXL_PROFILE_ZONE declares one per line.
         */
        class Site
        {
        public:
            constexpr Site(const char* name, const char* function, const char* file, uint32_t line,
                           uint32_t color = 0)
                : name_(name), function_(function), file_(file), line_(line), color_(color)
            {
            }

            Site(const Site&)            = delete;
            Site& operator=(const Site&) = delete;

            /// The runtime's location for this place, or 0 without the service.
            uintptr_t Location() const
            {
                uintptr_t location = location_.load(std::memory_order_acquire);
                if (location) return location;
                const wxl_profile_api* api = Api();
                if (!detail::Carries(api, &wxl_profile_api::location)) return 0;
                location = api->location(Str(name_), Str(function_), Str(file_), line_, color_);
                location_.store(location, std::memory_order_release);
                return location;
            }

        private:
            const char*                    name_;
            const char*                    function_;
            const char*                    file_;
            uint32_t                       line_;
            uint32_t                       color_;
            mutable std::atomic<uintptr_t> location_{0};
        };

        /**
         * @brief A zone from construction to destruction, on the thread that made it.
         *
         * Records nothing while no viewer is connected, or when the runtime has no profiler.
         */
        class Zone
        {
        public:
            explicit Zone(const Site& site)
            {
                const wxl_profile_api* api = Api();
                if (!api) return;
                zone_ = api->zone_begin(site.Location());
                if (zone_) api_ = api;
            }

            ~Zone()
            {
                if (zone_) api_->zone_end(zone_);
            }

            Zone(const Zone&)            = delete;
            Zone& operator=(const Zone&) = delete;

            /// Whether this zone is being recorded: a viewer was connected when it opened.
            bool Active() const { return zone_ != 0; }

            /// Adds a line of text to the zone: the item, the unit, the file it worked on.
            void Text(std::string_view text) const
            {
                if (zone_ && api_->zone_text) api_->zone_text(zone_, Str(text));
            }

        private:
            const wxl_profile_api* api_  = nullptr;
            uintptr_t              zone_ = 0;
        };
    } // namespace profile

    /**
     * @brief The typed facade over wxl.profile.
     *
     *     void Journal::Flush()
     *     {
     *         WXL_PROFILE_ZONE("Journal::Flush");      // a zone until the end of the scope
     *         ...
     *         wxl::ProfileService::Plot("journal lines", static_cast<double>(lines_.size()));
     *     }
     *
     * The plain methods are rows of profile.def, and answer as profile.h says:
     *
     *   Connected()         bool  whether a viewer is connected
     *   Plot(name, value)         a value on the graph of that name
     *   Message(text)             a line of text on the timeline, now
     *   FrameMark(name)           ends a frame of a frame set of the mod's own
     *
     * Zones are wxl::profile::Zone, made by the macros below.
     */
    class ProfileService : public detail::Facade<ProfileService, wxl_profile_api>,
                           public api::facade::Methods<wxl_profile_api>
    {
    public:
        static constexpr const char* kName    = WXL_PROFILE_SERVICE;
        static constexpr uint32_t    kVersion = WXL_PROFILE_SERVICE_VERSION;
    };
} // namespace wxl

#define WXL_PROFILE_CONCAT_(a, b) a##b
#define WXL_PROFILE_CONCAT(a, b)  WXL_PROFILE_CONCAT_(a, b)

/// A zone named @p name from here to the end of the scope, in the viewer's own colour.
#define WXL_PROFILE_ZONE(name) WXL_PROFILE_ZONE_COLOR(name, 0)

/// The same, drawn in @p color (0xRRGGBB).
#define WXL_PROFILE_ZONE_COLOR(name, color)                                                         \
    static ::wxl::profile::Site WXL_PROFILE_CONCAT(wxl_profile_site_, __LINE__){name, __func__,    \
                                                                                __FILE__, __LINE__, \
                                                                                color};             \
    const ::wxl::profile::Zone  WXL_PROFILE_CONCAT(wxl_profile_zone_, __LINE__)(                    \
        WXL_PROFILE_CONCAT(wxl_profile_site_, __LINE__))

/// A zone held in a variable called @p var, for var.Text(...).
#define WXL_PROFILE_ZONE_NAMED(var, name)                                                          \
    static ::wxl::profile::Site WXL_PROFILE_CONCAT(wxl_profile_site_, __LINE__){name, __func__,    \
                                                                                __FILE__, __LINE__, \
                                                                                0};                 \
    const ::wxl::profile::Zone  var(WXL_PROFILE_CONCAT(wxl_profile_site_, __LINE__))

#endif // WXL_SERVICE_PROFILE_HPP
