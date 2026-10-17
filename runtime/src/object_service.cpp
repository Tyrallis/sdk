// The shared half of wxl.object: the thunks behind the table, over a runtime's leaf reads.
// Copyright (C) 2026 WarcraftXL
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "wxl/runtime/object_service.hpp"

namespace wxl::runtime
{
    ObjectService* ObjectService::instance_ = nullptr;

    template <class V, auto Read, uint32_t Kinds>
    wxl_result WXL_CALL ObjectService::Get(wxl_handle handle, V* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Object(handle);
        if (!native) return WXL_E_NOT_FOUND;
        if (!OfKind(native, Kinds)) return WXL_E_INVALID;
        return (instance_->binding_.*Read)(native, *out) ? WXL_OK : WXL_E_INVALID;
    }

    template <class V, auto Read, uint32_t Kinds>
    wxl_result WXL_CALL ObjectService::GetAt(wxl_handle handle, uint32_t index, V* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Object(handle);
        if (!native) return WXL_E_NOT_FOUND;
        if (!OfKind(native, Kinds)) return WXL_E_INVALID;
        return (instance_->binding_.*Read)(native, index, *out) ? WXL_OK : WXL_E_INVALID;
    }

    ObjectService::ObjectService(ObjectBinding& binding) : binding_(binding)
    {
        instance_ = this;

        api_.struct_size = sizeof(api_);

        api_.valid        = &ObjectService::ValidThunk;
        api_.guid         = &ObjectService::GuidThunk;
        api_.type         = &ObjectService::TypeThunk;
        api_.from_guid    = &ObjectService::FromGuidThunk;
        api_.local_player = &ObjectService::LocalPlayerThunk;

        api_.position = &Get<wxl_vec3, &ObjectBinding::Position, kAny>;
        api_.facing   = &Get<float, &ObjectBinding::Facing, kAny>;
        api_.name     = &ObjectService::NameThunk;

        api_.unit_health     = &Get<uint32_t, &ObjectBinding::UnitHealth, kUnits>;
        api_.unit_max_health = &Get<uint32_t, &ObjectBinding::UnitMaxHealth, kUnits>;
        api_.unit_level      = &Get<uint32_t, &ObjectBinding::UnitLevel, kUnits>;
        api_.unit_target     = &ObjectService::UnitTargetThunk;

        api_.enumerate = &ObjectService::EnumerateThunk;
        api_.positions = &ObjectService::PositionsThunk;

        api_.entry = &Get<uint32_t, &ObjectBinding::Entry, kAny>;
        api_.scale = &Get<float, &ObjectBinding::Scale, kAny>;

        api_.unit_power             = &GetAt<uint32_t, &ObjectBinding::UnitPower, kUnits>;
        api_.unit_max_power         = &GetAt<uint32_t, &ObjectBinding::UnitMaxPower, kUnits>;
        api_.unit_power_type        = &Get<uint32_t, &ObjectBinding::UnitPowerType, kUnits>;
        api_.unit_race              = &Get<uint32_t, &ObjectBinding::UnitRace, kUnits>;
        api_.unit_class             = &Get<uint32_t, &ObjectBinding::UnitClass, kUnits>;
        api_.unit_sex               = &Get<uint32_t, &ObjectBinding::UnitSex, kUnits>;
        api_.unit_display_id        = &Get<uint32_t, &ObjectBinding::UnitDisplayId, kUnits>;
        api_.unit_native_display_id = &Get<uint32_t, &ObjectBinding::UnitNativeDisplayId, kUnits>;
        api_.unit_mount_display_id  = &Get<uint32_t, &ObjectBinding::UnitMountDisplayId, kUnits>;
        api_.unit_faction_template  = &Get<uint32_t, &ObjectBinding::UnitFactionTemplate, kUnits>;
        api_.unit_flags             = &Get<uint32_t, &ObjectBinding::UnitFlags, kUnits>;
        api_.unit_flags2            = &Get<uint32_t, &ObjectBinding::UnitFlags2, kUnits>;
        api_.unit_dynamic_flags     = &Get<uint32_t, &ObjectBinding::UnitDynamicFlags, kUnits>;
        api_.unit_npc_flags         = &Get<uint32_t, &ObjectBinding::UnitNpcFlags, kUnits>;
        api_.unit_bounding_radius   = &Get<float, &ObjectBinding::UnitBoundingRadius, kUnits>;
        api_.unit_combat_reach      = &Get<float, &ObjectBinding::UnitCombatReach, kUnits>;
        api_.unit_charm             = &Get<wxl_guid, &ObjectBinding::UnitCharm, kUnits>;
        api_.unit_summon            = &Get<wxl_guid, &ObjectBinding::UnitSummon, kUnits>;
        api_.unit_charmed_by        = &Get<wxl_guid, &ObjectBinding::UnitCharmedBy, kUnits>;
        api_.unit_summoned_by       = &Get<wxl_guid, &ObjectBinding::UnitSummonedBy, kUnits>;
        api_.unit_created_by        = &Get<wxl_guid, &ObjectBinding::UnitCreatedBy, kUnits>;

        api_.player_flags        = &Get<uint32_t, &ObjectBinding::PlayerFlags, kPlayers>;
        api_.player_guild_id     = &Get<uint32_t, &ObjectBinding::PlayerGuildId, kPlayers>;
        api_.player_guild_rank   = &Get<uint32_t, &ObjectBinding::PlayerGuildRank, kPlayers>;
        api_.player_chosen_title = &Get<uint32_t, &ObjectBinding::PlayerChosenTitle, kPlayers>;
        api_.player_visible_item = &GetAt<uint32_t, &ObjectBinding::PlayerVisibleItem, kPlayers>;

        api_.item_owner          = &Get<wxl_guid, &ObjectBinding::ItemOwner, kItems>;
        api_.item_contained_in   = &Get<wxl_guid, &ObjectBinding::ItemContainedIn, kItems>;
        api_.item_creator        = &Get<wxl_guid, &ObjectBinding::ItemCreator, kItems>;
        api_.item_stack_count    = &Get<uint32_t, &ObjectBinding::ItemStackCount, kItems>;
        api_.item_duration       = &Get<uint32_t, &ObjectBinding::ItemDuration, kItems>;
        api_.item_flags          = &Get<uint32_t, &ObjectBinding::ItemFlags, kItems>;
        api_.item_durability     = &Get<uint32_t, &ObjectBinding::ItemDurability, kItems>;
        api_.item_max_durability = &Get<uint32_t, &ObjectBinding::ItemMaxDurability, kItems>;

        api_.container_num_slots = &Get<uint32_t, &ObjectBinding::ContainerNumSlots, kContainers>;
        api_.container_slot      = &GetAt<wxl_guid, &ObjectBinding::ContainerSlot, kContainers>;

        api_.gameobject_created_by    = &Get<wxl_guid, &ObjectBinding::GameObjectCreatedBy, kGameObjects>;
        api_.gameobject_display_id    = &Get<uint32_t, &ObjectBinding::GameObjectDisplayId, kGameObjects>;
        api_.gameobject_flags         = &Get<uint32_t, &ObjectBinding::GameObjectFlags, kGameObjects>;
        api_.gameobject_dynamic_flags = &Get<uint32_t, &ObjectBinding::GameObjectDynamicFlags, kGameObjects>;
        api_.gameobject_faction       = &Get<uint32_t, &ObjectBinding::GameObjectFaction, kGameObjects>;
        api_.gameobject_level         = &Get<uint32_t, &ObjectBinding::GameObjectLevel, kGameObjects>;
        api_.gameobject_state         = &Get<uint32_t, &ObjectBinding::GameObjectState, kGameObjects>;
        api_.gameobject_type          = &Get<uint32_t, &ObjectBinding::GameObjectType, kGameObjects>;
        api_.gameobject_art_kit       = &Get<uint32_t, &ObjectBinding::GameObjectArtKit, kGameObjects>;

        api_.dynamic_caster    = &Get<wxl_guid, &ObjectBinding::DynamicCaster, kDynamics>;
        api_.dynamic_spell_id  = &Get<uint32_t, &ObjectBinding::DynamicSpellId, kDynamics>;
        api_.dynamic_radius    = &Get<float, &ObjectBinding::DynamicRadius, kDynamics>;
        api_.dynamic_cast_time = &Get<uint32_t, &ObjectBinding::DynamicCastTime, kDynamics>;

        api_.corpse_owner         = &Get<wxl_guid, &ObjectBinding::CorpseOwner, kCorpses>;
        api_.corpse_party         = &Get<wxl_guid, &ObjectBinding::CorpseParty, kCorpses>;
        api_.corpse_display_id    = &Get<uint32_t, &ObjectBinding::CorpseDisplayId, kCorpses>;
        api_.corpse_guild         = &Get<uint32_t, &ObjectBinding::CorpseGuild, kCorpses>;
        api_.corpse_flags         = &Get<uint32_t, &ObjectBinding::CorpseFlags, kCorpses>;
        api_.corpse_dynamic_flags = &Get<uint32_t, &ObjectBinding::CorpseDynamicFlags, kCorpses>;
    }

    ObjectService::~ObjectService()
    {
        if (instance_ == this) instance_ = nullptr;
    }

    wxl_handle ObjectService::Track(void* object) { return handles_.Track(object); }

    void ObjectService::Untrack(void* object) { handles_.Untrack(object); }

    void ObjectService::Clear() { handles_.Clear(); }

    wxl_handle ObjectService::FromGuid(wxl_guid guid) const
    {
        if (guid == WXL_GUID_NONE) return WXL_HANDLE_NONE;

        wxl_handle found = WXL_HANDLE_NONE;
        handles_.ForEach(
            [&](wxl_handle handle, void* native)
            {
                if (found != WXL_HANDLE_NONE) return;
                if (binding_.Guid(native) == guid) found = handle;
            });
        return found;
    }

    void* ObjectService::Object(wxl_handle handle)
    {
        return instance_ ? instance_->handles_.Resolve(handle) : nullptr;
    }

    bool ObjectService::OfKind(void* native, uint32_t kinds)
    {
        const uint32_t type = static_cast<uint32_t>(instance_->binding_.Type(native));
        return type < 32u && (kinds & (1u << type)) != 0;
    }

    wxl_bool WXL_CALL ObjectService::ValidThunk(wxl_handle object)
    {
        return Object(object) ? WXL_TRUE : WXL_FALSE;
    }

    wxl_guid WXL_CALL ObjectService::GuidThunk(wxl_handle object)
    {
        void* native = Object(object);
        return native ? instance_->binding_.Guid(native) : WXL_GUID_NONE;
    }

    uint32_t WXL_CALL ObjectService::TypeThunk(wxl_handle object)
    {
        void* native = Object(object);
        return native ? static_cast<uint32_t>(instance_->binding_.Type(native))
                      : static_cast<uint32_t>(WXL_OBJECT_UNKNOWN);
    }

    wxl_handle WXL_CALL ObjectService::FromGuidThunk(wxl_guid guid)
    {
        return instance_ ? instance_->FromGuid(guid) : WXL_HANDLE_NONE;
    }

    wxl_handle WXL_CALL ObjectService::LocalPlayerThunk(void)
    {
        if (!instance_) return WXL_HANDLE_NONE;

        void* player = instance_->binding_.LocalPlayer();
        return player ? instance_->handles_.HandleOf(player) : WXL_HANDLE_NONE;
    }

    wxl_result WXL_CALL ObjectService::NameThunk(wxl_handle object, wxl_str* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Object(object);
        if (!native) return WXL_E_NOT_FOUND;

        const char* text = nullptr;
        uint32_t    size = 0;
        if (!instance_->binding_.Name(native, text, size)) return WXL_E_INVALID;

        out->data = text;
        out->size = size;
        return WXL_OK;
    }

    wxl_result WXL_CALL ObjectService::UnitTargetThunk(wxl_handle unit, wxl_handle* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Object(unit);
        if (!native) return WXL_E_NOT_FOUND;
        if (!OfKind(native, kUnits)) return WXL_E_INVALID;

        wxl_guid target = WXL_GUID_NONE;
        if (!instance_->binding_.UnitTarget(native, target)) return WXL_E_INVALID;
        *out = instance_->FromGuid(target);
        return WXL_OK;
    }

    uint32_t WXL_CALL ObjectService::EnumerateThunk(uint32_t type, wxl_handle* out, uint32_t cap)
    {
        if (!instance_) return 0;

        uint32_t matched = 0;
        instance_->handles_.ForEach(
            [&](wxl_handle handle, void* native)
            {
                if (type != WXL_OBJECT_UNKNOWN &&
                    static_cast<uint32_t>(instance_->binding_.Type(native)) != type)
                    return;

                if (out && matched < cap) out[matched] = handle;
                ++matched;
            });
        return matched;
    }

    wxl_result WXL_CALL ObjectService::PositionsThunk(const wxl_handle* objects, uint32_t count,
                                                      wxl_vec3* out)
    {
        if (!objects || !out) return WXL_E_INVALID;
        if (!instance_) return WXL_E_UNSUPPORTED;

        for (uint32_t i = 0; i < count; ++i)
        {
            out[i] = wxl_vec3{0.0f, 0.0f, 0.0f};

            void* native = Object(objects[i]);
            if (native) instance_->binding_.Position(native, out[i]);
        }
        return WXL_OK;
    }
} // namespace wxl::runtime
