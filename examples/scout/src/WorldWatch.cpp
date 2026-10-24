// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "WorldWatch.h"

#include <string>

namespace scout
{
    void WorldWatch::OnUpdate(float delta)
    {
        clock_.Advance(delta);
    }

    void WorldWatch::OnWorldEnter(uint32_t map_id, wxl_vec3 position)
    {
        journal_.Clear();
        wxl::Log(WXL_LOG_INFO, kTag,
                 "entered map " + std::to_string(map_id) + " at " +
                     std::to_string(static_cast<int>(position.x)) + ", " +
                     std::to_string(static_cast<int>(position.y)));
    }

    void WorldWatch::OnWorldLeave(uint32_t map_id, wxl_vec3)
    {
        wxl::Log(WXL_LOG_INFO, kTag,
                 "leaving map " + std::to_string(map_id) + " with " +
                     std::to_string(journal_.Size()) + " line(s) in the journal");
    }
} // namespace scout
