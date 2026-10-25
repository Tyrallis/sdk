// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.footprint: where the client's memory goes.

#ifndef WXL_SERVICE_FOOTPRINT_HPP
#define WXL_SERVICE_FOOTPRINT_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "wxl/sdk.hpp"
#include "wxl/services/footprint.h"
#include "wxl/services/rows.hpp"

namespace wxl::api::facade
{
    template <>
    struct Methods<wxl_footprint_api>
    {
#define WXL_API_TABLE "wxl/services/footprint.def"
#include "wxl/services/methods.inc"
    };
} // namespace wxl::api::facade

namespace wxl
{
    namespace footprint
    {
        /// One consumer of memory (wxl_footprint_category).
        struct Category
        {
            std::string name;
            std::string owner;
            uint64_t    bytes = 0;
            uint64_t    count = 0;
            uint64_t    peak  = 0;
        };

        /// Storm's allocations of one source file (wxl_footprint_source).
        struct Source
        {
            std::string file;
            uint64_t    bytes       = 0;
            uint64_t    count       = 0;
            uint64_t    peak        = 0;
            uint64_t    allocations = 0;
        };

        /// One mounted archive (wxl_footprint_archive).
        struct Archive
        {
            std::string path;
            int32_t     priority      = 0;
            uint32_t    hash_entries  = 0;
            uint32_t    block_entries = 0;
            uint64_t    bytes         = 0;
        };

        /// One client database (wxl_footprint_table).
        struct Table
        {
            std::string name;
            uint32_t    records     = 0;
            uint32_t    record_size = 0;
            uint64_t    bytes       = 0;
        };

        /// One kind of the address space (wxl_footprint_region).
        struct Region
        {
            std::string state;
            std::string type;
            uint64_t    bytes   = 0;
            uint32_t    regions = 0;
            uint64_t    largest = 0;
        };
    } // namespace footprint

    /**
     * @brief The typed facade over wxl.footprint.
     *
     *     const wxl::FootprintService footprint;
     *     footprint.Start();                       // at load: count Storm's allocations
     *     ...
     *     footprint.Snapshot();                    // on the main thread
     *     for (const wxl::footprint::Source& s : footprint.Sources())
     *         wxl::Log(WXL_LOG_INFO, "mymod", "%s %llu", s.file.c_str(), (unsigned long long)s.bytes);
     *
     * The plain methods are rows of footprint.def, and answer as footprint.h says:
     *
     *   Start()                      wxl_result  counts Storm's allocations from zero
     *   Running()                    bool        whether it counts
     *   Snapshot()                   wxl_result  takes the picture the lists read
     *   Report(name, bytes, count)   wxl_result  a consumer of the mod's own, under its id
     *   WatchPressure(bytes)         wxl_result  arms MemoryPressure under this largest free block
     *
     * The lists are written here, as vectors that own their strings: each reads the last
     * Snapshot(), and is empty without the service.
     */
    class FootprintService : public detail::Facade<FootprintService, wxl_footprint_api>,
                             public api::facade::Methods<wxl_footprint_api>
    {
    public:
        static constexpr const char* kName    = WXL_FOOTPRINT_SERVICE;
        static constexpr uint32_t    kVersion = WXL_FOOTPRINT_SERVICE_VERSION;

        /// Stops counting; the last counts stay in the next Snapshot().
        void Stop() const
        {
            const wxl_footprint_api* api = Raw();
            if (detail::Carries(api, &wxl_footprint_api::stop)) api->stop();
        }

        std::vector<footprint::Category> Categories() const
        {
            return List<wxl_footprint_category, footprint::Category>(&wxl_footprint_api::categories,
                                                                     [](const wxl_footprint_category& c) {
                                                                         return footprint::Category{
                                                                             std::string(View(c.name)),
                                                                             std::string(View(c.owner)), c.bytes,
                                                                             c.count, c.peak};
                                                                     });
        }

        std::vector<footprint::Source> Sources() const
        {
            return List<wxl_footprint_source, footprint::Source>(&wxl_footprint_api::sources,
                                                                 [](const wxl_footprint_source& s) {
                                                                     return footprint::Source{
                                                                         std::string(View(s.file)), s.bytes, s.count,
                                                                         s.peak, s.allocations};
                                                                 });
        }

        std::vector<footprint::Archive> Archives() const
        {
            return List<wxl_footprint_archive, footprint::Archive>(&wxl_footprint_api::archives,
                                                                   [](const wxl_footprint_archive& a) {
                                                                       return footprint::Archive{
                                                                           std::string(View(a.path)), a.priority,
                                                                           a.hash_entries, a.block_entries, a.bytes};
                                                                   });
        }

        std::vector<footprint::Table> Tables() const
        {
            return List<wxl_footprint_table, footprint::Table>(&wxl_footprint_api::tables,
                                                               [](const wxl_footprint_table& t) {
                                                                   return footprint::Table{std::string(View(t.name)),
                                                                                           t.records, t.record_size,
                                                                                           t.bytes};
                                                               });
        }

        std::vector<footprint::Region> Regions() const
        {
            return List<wxl_footprint_region, footprint::Region>(&wxl_footprint_api::regions,
                                                                 [](const wxl_footprint_region& r) {
                                                                     return footprint::Region{
                                                                         std::string(View(r.state)),
                                                                         std::string(View(r.type)), r.bytes,
                                                                         r.regions, r.largest};
                                                                 });
        }

    private:
        /// Counted, then filled, then copied into what owns its strings.
        template <class C, class T, class F>
        static std::vector<T> List(uint32_t(WXL_CALL* wxl_footprint_api::*member)(C*, uint32_t), F&& convert)
        {
            std::vector<T>           out;
            const wxl_footprint_api* api = Raw();
            if (!detail::Carries(api, member)) return out;
            const uint32_t count = (api->*member)(nullptr, 0);
            if (!count) return out;
            std::vector<C> items(count);
            for (C& item : items) item.struct_size = sizeof(C);
            const uint32_t written = (api->*member)(items.data(), count);
            const uint32_t n       = written < count ? written : count;
            out.reserve(n);
            for (uint32_t i = 0; i < n; ++i) out.push_back(convert(items[i]));
            return out;
        }
    };
} // namespace wxl

#endif // WXL_SERVICE_FOOTPRINT_HPP
