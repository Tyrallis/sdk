// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// A mod built with the SDK, for the host tests to load for real. It reports what happens to it
// through the host's log and publishes one service, so the test can see both sides work.

#include <cstdio>

#include "wxl/sdk.hpp"

namespace
{
    constexpr const char* kTag = "fixture";

    /// The service the fixture publishes: one call, one answer.
    struct FixtureApi
    {
        uint32_t struct_size;
        int32_t(WXL_CALL* answer)(void);
    };

    int32_t WXL_CALL Answer() { return 42; }

    FixtureApi g_api{sizeof(FixtureApi), &Answer};

    void WXL_CALL OnUpdate(void* user, const void* args)
    {
        const wxl_update_args* update = static_cast<const wxl_update_args*>(args);
        char                   line[64];
        std::snprintf(line, sizeof(line), "update %.3f", static_cast<double>(update->delta));
        wxl::Log(WXL_LOG_INFO, kTag, line);
        (void)user;
    }

    class FixtureMod final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            wxl::Log(WXL_LOG_INFO, kTag, "load");
            if (wxl::Host()->publish_service("fixture.api", 1, &g_api) != WXL_OK) return WXL_E_FAILED;
            if (wxl::Host()->subscribe(WXL_EV_Update, &OnUpdate, nullptr, WXL_PRIORITY_DEFAULT) ==
                WXL_SUBSCRIPTION_NONE)
                return WXL_E_FAILED;
            return WXL_OK;
        }

        void OnStage(wxl_stage stage) override
        {
            char line[32];
            std::snprintf(line, sizeof(line), "stage %d", static_cast<int>(stage));
            wxl::Log(WXL_LOG_INFO, kTag, line);
        }

        void OnUnload() override { wxl::Log(WXL_LOG_INFO, kTag, "unload"); }
    };
} // namespace

WXL_MOD(FixtureMod, "wxl.fixture", "Fixture", 7)
