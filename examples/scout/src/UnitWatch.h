// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#ifndef SCOUT_UNIT_WATCH_H
#define SCOUT_UNIT_WATCH_H

#include "wxl/objects.hpp"
#include "wxl/sdk.hpp"

#include "Journal.h"

namespace scout
{
    /// Writes down the units that appear, leave and die near the player, and each new target.
    class UnitWatch final : public wxl::ObjectScript
    {
    public:
        /// How close, in yards on the ground, a unit has to be for Scout to note it.
        static constexpr float kRange = 60.0f;

        UnitWatch(const Clock& clock, Journal& journal) : clock_(clock), journal_(journal) {}

        void OnObjectCreate(wxl::Object object, wxl_guid guid, uint32_t type) override;
        void OnObjectDestroy(wxl::Object object, wxl_guid guid, uint32_t type) override;
        void OnUnitDeath(wxl::Object unit) override;
        void OnTargetChanged(wxl::Object previous, wxl::Object current) override;

    private:
        /// Adds a line about @p unit to the journal.
        void Note(What what, const wxl::Unit& unit);

        const Clock& clock_;
        Journal&     journal_;
    };
} // namespace scout

#endif // SCOUT_UNIT_WATCH_H
