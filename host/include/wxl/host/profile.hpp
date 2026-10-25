// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// What the host asks of a runtime's profiler: a zone around each event it dispatches and around
// each handler it calls, named after the event and the mod. The host carries no profiler itself;
// a runtime that has one hands it these three functions.

#ifndef WXL_HOST_PROFILE_HPP
#define WXL_HOST_PROFILE_HPP

#include <cstdint>
#include <string_view>

namespace wxl::host
{
    /**
     * @brief A runtime's profiler, as the event bus uses it.
     *
     * Every function is called only while the runtime says a viewer is watching (see
     * EventBus::SetProfiling), except `location`, which is called when a subscription is made
     * and when the profiler is set.
     */
    struct Profiler
    {
        void* user = nullptr;

        /// A place zones are named after, kept for the process's life. @p color is 0xRRGGBB, 0
        /// for the viewer's own. Null when the profiler cannot make one.
        const void* (*location)(void* user, std::string_view name, std::string_view function,
                                uint32_t color) = nullptr;

        /// Opens a zone at a location on the calling thread: 0 when nothing is recorded.
        uint64_t (*begin)(void* user, const void* location) = nullptr;

        /// Closes what begin answered, on the same thread, innermost first.
        void (*end)(void* user, uint64_t zone) = nullptr;
    };

    /// The colour of the zone of an event, and of a mod's handlers: one per mod, from its id.
    constexpr uint32_t kEventZoneColor = 0x4A6FA5;
    inline uint32_t ModZoneColor(std::string_view id)
    {
        // FNV-1a over the id, onto a few readable hues: a mod keeps its colour from run to run.
        constexpr uint32_t kPalette[] = {0xC0504D, 0x9BBB59, 0x8064A2, 0x4BACC6, 0xF79646,
                                         0xD4A017, 0x2E8B57, 0xB5651D, 0x5F9EA0, 0xCD5C5C};
        uint32_t hash = 2166136261u;
        for (const char c : id) hash = (hash ^ static_cast<uint8_t>(c)) * 16777619u;
        return kPalette[hash % (sizeof kPalette / sizeof kPalette[0])];
    }
} // namespace wxl::host

#endif // WXL_HOST_PROFILE_HPP
