// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "UnitWatch.h"

#include <cmath>

namespace
{
    /// The distance between two points on the ground, height left out.
    float GroundDistance(const wxl_vec3& a, const wxl_vec3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    /// How far @p unit is from the player, or why that cannot be read.
    wxl::Expected<float> DistanceToPlayer(const wxl::Unit& unit)
    {
        const wxl::Expected<wxl_vec3> mine = wxl::Player::Local().Position();
        if (!mine) return wxl::Unexpected{mine.Error()};

        const wxl::Expected<wxl_vec3> theirs = unit.Position();
        if (!theirs) return wxl::Unexpected{theirs.Error()};

        return GroundDistance(*mine, *theirs);
    }

    /// Whether @p unit is known to be within range. A distance that cannot be read is not near.
    bool IsNear(const wxl::Unit& unit)
    {
        const wxl::Expected<float> distance = DistanceToPlayer(unit);
        return distance && *distance <= scout::UnitWatch::kRange;
    }
} // namespace

namespace scout
{
    void UnitWatch::OnObjectCreate(wxl::Object object, wxl_guid, wxl_object_type type)
    {
        if (!wxl::Unit::Accepts(type)) return;

        const wxl::Unit unit(object);
        if (unit == wxl::Player::Local()) return;
        if (IsNear(unit)) Note(What::Appeared, unit);
    }

    void UnitWatch::OnObjectDestroy(wxl::Object object, wxl_guid, wxl_object_type type)
    {
        if (!wxl::Unit::Accepts(type)) return;

        const wxl::Unit unit(object);
        if (IsNear(unit)) Note(What::Left, unit);
    }

    void UnitWatch::OnUnitDeath(wxl::Object unit)
    {
        if (IsNear(wxl::Unit(unit))) Note(What::Died, wxl::Unit(unit));
    }

    void UnitWatch::OnTargetChanged(wxl::Object, wxl::Object current)
    {
        if (!current.HasHandle()) return;
        Note(What::Targeted, wxl::Unit(current));
    }

    void UnitWatch::Note(What what, const wxl::Unit& unit)
    {
        Entry entry;
        entry.what  = what;
        entry.who   = unit.Name().ValueOr("?");
        entry.level = unit.Level().ValueOr(0u);
        entry.at    = clock_.Now();
        journal_.Add(std::move(entry));
    }
} // namespace scout
