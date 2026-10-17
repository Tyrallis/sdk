// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Watchers.h"

#include <cstdio>

namespace scout
{
    void WorldWatch::OnUpdate(const wxl_update_args& args)
    {
        // Called once per tick of the client's scheduler, with the seconds since the last one.
        // This is the only clock Scout has, and the only work it does every frame.
        clock_.Advance(args.delta);
    }

    void WorldWatch::OnWorldEnter(const wxl_world_args& args)
    {
        mapId_ = args.map_id;
        journal_.Clear();

        char line[96];
        std::snprintf(line, sizeof line, "watching map %u from %.0f, %.0f", args.map_id,
                      args.position.x, args.position.y);
        wxl::Log(WXL_LOG_INFO, kTag, line);
    }

    void WorldWatch::OnWorldLeave(const wxl_world_args& args)
    {
        char line[96];
        std::snprintf(line, sizeof line, "leaving map %u after %u note(s)", args.map_id,
                      journal_.Written());
        wxl::Log(WXL_LOG_INFO, kTag, line);

        mapId_ = 0;
    }
} // namespace scout
