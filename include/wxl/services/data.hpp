// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.data: the client's databases, served from a mod's own files.

#ifndef WXL_SERVICE_DATA_HPP
#define WXL_SERVICE_DATA_HPP

#include "wxl/sdk.hpp"
#include "wxl/services/data.h"
#include "wxl/services/rows.hpp"

namespace wxl::api::facade
{
    template <>
    struct Methods<wxl_data_api>
    {
#define WXL_API_TABLE "wxl/services/data.def"
#include "wxl/services/methods.inc"
    };
} // namespace wxl::api::facade

namespace wxl
{
    /**
     * @brief The typed facade over wxl.data.
     *
     * The client reads each of its databases once, early, and never looks at the file again.
     * So a mod that wants to change one says so before that read -- in OnLoad or
     * OnStorageReady -- and `IsLoaded()` is how it finds out it was too late.
     *
     *     const wxl::DataService data;
     *     if (data.ServeTable("Spell.dbc", "Mods\\mymod\\Spell.dbc") != WXL_OK)
     *         wxl::Log(WXL_LOG_WARN, "mymod", "another mod is already serving Spell.dbc");
     *
     * Every method is a row of data.def, and answers as data.h says:
     *
     *   ServeTable(table, path)   wxl_result   serves @p table from @p path instead of the
     *                                          client's own file
     *   RestoreTable(table)       wxl_result   gives it back, so the client's own file is used
     *   ServedFrom(table)         std::string  what it is served from, empty for the client's own
     *   IsLoaded(table)           bool         whether the client read it already, in which case
     *                                          an override comes too late
     *
     * Without the service a wxl_result is WXL_E_UNSUPPORTED, a string empty, a bool false.
     */
    class DataService : public detail::Facade<DataService, wxl_data_api>,
                        public api::facade::Methods<wxl_data_api>
    {
    public:
        static constexpr const char* kName    = WXL_DATA_SERVICE;
        static constexpr uint32_t    kVersion = WXL_DATA_SERVICE_VERSION;
    };
} // namespace wxl

#endif // WXL_SERVICE_DATA_HPP
