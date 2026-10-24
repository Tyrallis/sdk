// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The mod: it owns Scout's state and its scripts, and registers the scripts when it loads.

#include <string>

#include "wxl/sdk.hpp"
#include "wxl/services/script.hpp"

#include "Journal.h"
#include "ScriptApi.h"
#include "Traffic.h"
#include "UnitWatch.h"
#include "WorldWatch.h"

namespace
{
    class Scout final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            journal_.OnAdd([this](const scout::Entry& entry) { script_.Announce(entry); });

            wxl::ScriptMgr::Add(&world_);
            wxl::ScriptMgr::Add(&units_);
            wxl::ScriptMgr::Add(&traffic_);
            wxl::ScriptMgr::Add(&script_);

            const std::string_view runtime = wxl::View(wxl::Host()->runtime()->name);
            wxl::Log(WXL_LOG_INFO, scout::kTag, "watching, on " + std::string(runtime));
            return WXL_OK;
        }

        void OnReady() override
        {
            if (!wxl::ScriptService::Available())
                wxl::Log(WXL_LOG_WARN, scout::kTag,
                         "this runtime has no wxl.script: the journal stays out of the interface");
        }

        void OnShutdown() override
        {
            wxl::Log(WXL_LOG_INFO, scout::kTag,
                     std::to_string(journal_.Written()) + " line(s) written this session");
        }

    private:
        // Members are built in this order and destroyed in the reverse one: the state first,
        // then the scripts that hold references to it.
        scout::Clock   clock_;
        scout::Journal journal_;

        scout::WorldWatch world_{clock_, journal_};
        scout::UnitWatch  units_{clock_, journal_};
        scout::Traffic    traffic_;
        scout::ScriptApi  script_{clock_, journal_, traffic_};
    };
} // namespace

// The id must be the manifest's, letter for letter.
WXL_MOD(Scout, "wxl.scout", "Scout", 1)
