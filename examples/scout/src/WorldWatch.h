// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#ifndef SCOUT_WORLD_WATCH_H
#define SCOUT_WORLD_WATCH_H

#include "wxl/sdk.hpp"

#include "Journal.h"

namespace scout
{
    /// Keeps the clock and starts a new journal on every world Scout enters.
    class WorldWatch final : public wxl::WorldScript
    {
    public:
        WorldWatch(Clock& clock, Journal& journal) : clock_(clock), journal_(journal) {}

        void OnUpdate(float delta) override;
        void OnWorldEnter(uint32_t map_id, wxl_vec3 position) override;
        void OnWorldLeave(uint32_t map_id, wxl_vec3 position) override;

    private:
        Clock&   clock_;
        Journal& journal_;
    };
} // namespace scout

#endif // SCOUT_WORLD_WATCH_H
