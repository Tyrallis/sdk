// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The mod itself: it owns the state, hands it to the watchers, and registers them. Everything
// that happens afterwards happens in one of the files beside this one.

#include <cstdio>

#include "wxl/sdk.hpp"

#include "Journal.h"
#include "ScriptApi.h"
#include "Watchers.h"

namespace
{
    class Scout final : public wxl::Mod
    {
    public:
        /**
         * @brief Set-up. Nothing the client brings up exists yet -- not the world, not the
         *        interface, not a single file.
         *
         * So the only thing done here is registering: saying what to call later. Anything that
         * needs the client to be further along waits for the stage that brings it up, or for
         * the event that announces it.
         */
        wxl_result OnLoad() override
        {
            journal_.OnChange([this] { script_.Announce(); });

            wxl::ScriptMgr::Add(&world_);
            wxl::ScriptMgr::Add(&objects_);
            wxl::ScriptMgr::Add(&net_);
            wxl::ScriptMgr::Add(&script_);

            wxl::Log(WXL_LOG_INFO, scout::kTag, "watching");
            return WXL_OK;
        }

        /// The client is up. Say once what this runtime can and cannot do, so the log explains
        /// a quiet mod instead of leaving someone guessing.
        void OnReady() override
        {
            if (!wxl::ScriptService::Available())
                wxl::Log(WXL_LOG_WARN, scout::kTag,
                         "no wxl.script here: the journal is kept but no interface can read it");
        }

        void OnShutdown() override
        {
            char line[96];
            std::snprintf(line, sizeof line, "%u note(s) this session", journal_.Written());
            wxl::Log(WXL_LOG_INFO, scout::kTag, line);
        }

    private:
        // Declaration order is construction order: the state first, then the watchers that
        // borrow it. A watcher holds a reference, never a copy, so there is one journal.
        scout::Clock   clock_;
        scout::Journal journal_;

        scout::WorldWatch  world_{clock_, journal_};
        scout::ObjectWatch objects_{clock_, journal_};
        scout::NetWatch    net_;
        scout::ScriptApi   script_{journal_, net_};
    };
} // namespace

// The four entry points a runtime looks for, written for you. The id must be the one in
// mod.json, to the letter.
WXL_MOD(Scout, "wxl.scout", "Scout", 1)
