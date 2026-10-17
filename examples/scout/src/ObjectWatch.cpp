// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Watchers.h"

#include <cmath>

namespace
{
    /// How far apart two points are, on the ground: height is noise for "is this near me".
    float GroundDistance(const wxl_vec3& a, const wxl_vec3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    /**
     * @brief How far a unit is from the player, or nothing when that cannot be answered.
     *
     * Three things can go wrong and they are not the same thing: there is no object service on
     * this runtime, the player is not in the world, or the unit has gone. Expected<T> keeps
     * them apart; a function returning 0.0f for all three would be a lie that reads as "right
     * on top of you".
     */
    wxl::Expected<float> DistanceToPlayer(const wxl::Unit& unit)
    {
        const wxl::Player me = wxl::ObjectService().LocalPlayer();

        const wxl::Expected<wxl_vec3> mine = me.Position();
        if (!mine) return wxl::Unexpected{mine.Error()};

        const wxl::Expected<wxl_vec3> theirs = unit.Position();
        if (!theirs) return wxl::Unexpected{theirs.Error()};

        return GroundDistance(*mine, *theirs);
    }
} // namespace

namespace scout
{
    void ObjectWatch::Note(What what, wxl::Unit unit)
    {
        // Out of range is the common case in a city, so it is checked first and nothing else is
        // read. A distance that cannot be answered is not a reason to drop the line: being out
        // of the world yourself still makes a death near you worth noting.
        const wxl::Expected<float> distance = DistanceToPlayer(unit);
        if (distance && *distance > kRange) return;

        Entry entry;
        entry.what = what;
        entry.at   = clock_.Now();

        // ValueOr is the right tool here and the wrong one above: a name we could not read is
        // honestly "?", while a distance we could not read is not zero.
        entry.who   = unit.Name().ValueOr(std::string("?"));
        entry.level = unit.Level().ValueOr(0u);

        journal_.Add(std::move(entry));
    }

    void ObjectWatch::OnObjectCreate(const wxl_object_args& args)
    {
        // Everything the client tracks comes through here: items in your bags, corpses, the
        // doodads of the zone. Scout only wants things that are alive.
        if (args.type != WXL_OBJECT_UNIT && args.type != WXL_OBJECT_PLAYER) return;
        Note(What::Appeared, wxl::Unit(args.object));
    }

    void ObjectWatch::OnObjectDestroy(const wxl_object_args& args)
    {
        if (args.type != WXL_OBJECT_UNIT && args.type != WXL_OBJECT_PLAYER) return;
        Note(What::LeftView, wxl::Unit(args.object));
    }

    void ObjectWatch::OnUnitDeath(const wxl_unit_args& args)
    {
        Note(What::Died, wxl::Unit(args.unit));
    }

    void ObjectWatch::OnTargetChanged(const wxl_target_args& args)
    {
        // Clearing the target is a change too, and there is nothing to read then.
        if (args.current == WXL_HANDLE_NONE) return;
        Note(What::Targeted, wxl::Unit(args.current));
    }
} // namespace scout
