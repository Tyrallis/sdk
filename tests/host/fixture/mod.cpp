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

    /// Logs every tick it sees.
    struct TickLogger
    {
        void operator()(float delta) const
        {
            char line[64];
            std::snprintf(line, sizeof(line), "update %.3f", static_cast<double>(delta));
            wxl::Log(WXL_LOG_INFO, kTag, line);
        }
    };

    class FixtureMod final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            wxl::Log(WXL_LOG_INFO, kTag, "load");
            if (wxl::Host()->publish_service("fixture.api", 1, &g_api) != WXL_OK) return WXL_E_FAILED;
            tick_ = wxl::Subscribe<WXL_EV_Update>(&tickLogger_);
            if (!tick_) return WXL_E_FAILED;
            return WXL_OK;
        }

        /// Every stage, by number, so the test can see each one arrive.
        void OnStage(wxl_stage stage) override
        {
            char line[32];
            std::snprintf(line, sizeof(line), "stage %d", static_cast<int>(stage));
            wxl::Log(WXL_LOG_INFO, kTag, line);
        }

        void OnUnload() override { wxl::Log(WXL_LOG_INFO, kTag, "unload"); }

    private:
        TickLogger        tickLogger_;
        wxl::Subscription tick_;
    };
} // namespace

WXL_MOD(FixtureMod, "wxl.fixture", "Fixture")
