// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "WorldWatch.h"

namespace scout
{
    void WorldWatch::OnUpdate(float delta)
    {
        clock_.Advance(delta);
    }

    void WorldWatch::OnWorldEnter(uint32_t map_id, wxl_vec3 position)
    {
        journal_.Clear();
        wxl::Log(WXL_LOG_INFO, kTag, "entered map {} at {:.0f}, {:.0f}", map_id, position.x,
                 position.y);
    }

    void WorldWatch::OnWorldLeave(uint32_t map_id, wxl_vec3)
    {
        wxl::Log(WXL_LOG_INFO, kTag, "leaving map {} with {} line(s) in the journal", map_id,
                 journal_.Size());
    }
} // namespace scout
