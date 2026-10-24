// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Watchers.h"

#include <cstdio>

namespace scout
{
    void WorldWatch::OnUpdate(float delta)
    {
        // Called once per tick of the client's scheduler, with the seconds since the last one.
        // This is the only clock Scout has, and the only work it does every frame.
        clock_.Advance(delta);
    }

    void WorldWatch::OnWorldEnter(uint32_t map_id, wxl_vec3 position)
    {
        mapId_ = map_id;
        journal_.Clear();

        char line[96];
        std::snprintf(line, sizeof line, "watching map %u from %.0f, %.0f", map_id, position.x,
                      position.y);
        wxl::Log(WXL_LOG_INFO, kTag, line);
    }

    void WorldWatch::OnWorldLeave(uint32_t map_id, wxl_vec3)
    {
        char line[96];
        std::snprintf(line, sizeof line, "leaving map %u after %u note(s)", map_id,
                      journal_.Written());
        wxl::Log(WXL_LOG_INFO, kTag, line);

        mapId_ = 0;
    }
} // namespace scout
