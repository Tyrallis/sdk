// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::Unit: something alive, or recently so.

#ifndef WXL_OBJECTS_UNIT_HPP
#define WXL_OBJECTS_UNIT_HPP

#include "wxl/objects/object.hpp"

namespace wxl
{
    /// The powers a unit may run on, as the index unit_power takes.
    enum class Power : uint32_t
    {
        Mana       = 0,
        Rage       = 1,
        Focus      = 2,
        Energy     = 3,
        Happiness  = 4,
        Runes      = 5,
        RunicPower = 6
    };

    /// Something alive: a creature, an NPC, a player. The client's ID_UNIT and the base of Player.
    class Unit : public Object
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_UNIT, WXL_OBJECT_PLAYER};
        static constexpr bool            Accepts(uint32_t type)
        {
            return type == WXL_OBJECT_UNIT || type == WXL_OBJECT_PLAYER;
        }

        using Object::Object;

        /// The same object as this class, unchecked: for a handler whose event already gave the
        /// kind (`Accepts(type)`). As<T>() is the checked way.
        explicit constexpr Unit(const Object& object) : Object(object.Handle()) {}

#include "wxl/objects/generated/unit.inc"

        /// Health as a fraction in 0..1; 0 when the unit has no maximum to compare against.
        Expected<float> HealthFraction() const
        {
            const Expected<uint32_t> max = MaxHealth();
            if (!max) return Unexpected{max.Error()};
            const Expected<int32_t> health = Health();
            if (!health) return Unexpected{health.Error()};
            return *max && *health > 0 ? static_cast<float>(*health) / static_cast<float>(*max) : 0.0f;
        }

        /// What the unit targets. A null Unit as the value means it targets nothing in view.
        Expected<Unit> Target() const
        {
            Expected<wxl_handle> target = Read(&wxl_object_api::unit_target);
            if (!target) return Unexpected{target.Error()};
            return Unit(*target);
        }

        /// The unit's own power and its maximum, by PowerType().
        Expected<uint32_t> PowerValue() const
        {
            const Expected<Power> type = PowerType();
            if (!type) return Unexpected{type.Error()};
            return PowerValue(*type);
        }
        Expected<uint32_t> MaxPowerValue() const
        {
            const Expected<Power> type = PowerType();
            if (!type) return Unexpected{type.Error()};
            return MaxPowerValue(*type);
        }

        /// Every aura the client holds on it, in slot order (AuraCount and Aura in one call).
        Expected<std::vector<wxl_aura>> Auras() const
        {
            const Expected<uint32_t> count = AuraCount();
            if (!count) return Unexpected{count.Error()};
            std::vector<wxl_aura> auras;
            auras.reserve(*count);
            for (uint32_t slot = 0; slot < *count; ++slot)
                if (Expected<wxl_aura> aura = Aura(slot)) auras.push_back(*aura);
            return auras;
        }

        // --- what the player can do with it (wxl.action) ------------------------------------

        /// Casts a spell the player knows at this unit, as CastSpellByID does.
        wxl_result CastAt(uint32_t spell_id) const
        {
            const wxl_action_api* api = detail::ActionTable::Raw();
            if (!detail::Carries(api, &wxl_action_api::cast_spell)) return WXL_E_UNSUPPORTED;
            if (!HasHandle()) return WXL_E_NOT_FOUND;
            return api->cast_spell(spell_id, handle_);
        }

        /// Follows it, as /follow does.
        wxl_result Follow() const { return Act(&wxl_action_api::follow); }

        /// Targets what it targets, as /assist does.
        wxl_result Assist() const { return Act(&wxl_action_api::assist); }
    };
} // namespace wxl

namespace std
{
    template <>
    struct hash<wxl::Unit> : hash<wxl::Object>
    {
    };
} // namespace std

#endif // WXL_OBJECTS_UNIT_HPP
