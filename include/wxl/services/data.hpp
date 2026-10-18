// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.data: the client's databases, served from a mod's own files.

#ifndef WXL_SERVICE_DATA_HPP
#define WXL_SERVICE_DATA_HPP

#include <string>
#include <string_view>

#include "wxl/sdk.hpp"
#include "wxl/services/data.h"

namespace wxl
{
    /**
     * @brief The typed facade over wxl.data.
     *
     * The client reads each of its databases once, early, and never looks at the file again.
     * So a mod that wants to change one says so before that read -- in OnLoad or
     * OnStorageReady -- and `Loaded()` is how it finds out it was too late.
     *
     *     const wxl::DataService data;
     *     if (data.Serve("Spell.dbc", "Mods\\mymod\\Spell.dbc") != WXL_OK)
     *         wxl::Log(WXL_LOG_WARN, "mymod", "another mod is already serving Spell.dbc");
     */
    class DataService : public detail::Facade<DataService, wxl_data_api>
    {
    public:
        static constexpr const char* kName    = WXL_DATA_SERVICE;
        static constexpr uint32_t    kVersion = WXL_DATA_SERVICE_VERSION;

        /// Serves @p table from @p path instead of the client's own file.
        wxl_result Serve(std::string_view table, std::string_view path) const
        {
            const wxl_data_api* api = Raw();
            if (!detail::Carries(api, &wxl_data_api::serve_table)) return WXL_E_UNSUPPORTED;
            return api->serve_table(Str(table), Str(path));
        }

        /// Gives a table back, so the client's own file is used again.
        wxl_result Restore(std::string_view table) const
        {
            const wxl_data_api* api = Raw();
            if (!detail::Carries(api, &wxl_data_api::restore_table)) return WXL_E_UNSUPPORTED;
            return api->restore_table(Str(table));
        }

        /// What a table is being served from, empty when it is the client's own file.
        std::string ServedFrom(std::string_view table) const
        {
            const wxl_data_api* api = Raw();
            if (!detail::Carries(api, &wxl_data_api::served_from)) return std::string();
            return std::string(View(api->served_from(Str(table))));
        }

        /// Whether the client has already read it, in which case an override comes too late.
        bool Loaded(std::string_view table) const
        {
            const wxl_data_api* api = Raw();
            if (!detail::Carries(api, &wxl_data_api::is_loaded)) return false;
            return api->is_loaded(Str(table)) == WXL_TRUE;
        }
    };
} // namespace wxl

#endif // WXL_SERVICE_DATA_HPP
