// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The shared reads of wxl.object: the rows of object_reads.def whose source is `shared`, worked
// out here from the descriptor block and the runtime's other reads, the same on every runtime.
// Each one reads what the client's own code reads for the same answer; the function named
// beside it is the client's, as the fork writes it.

#include "wxl/runtime/object_service.hpp"

#include <cmath>
#include <cstring>

#include "wxl/objects/fields.hpp"

namespace wxl::runtime
{
    namespace
    {
        // UNIT_FIELD_FLAGS.
        constexpr uint32_t kUnitPvp       = 0x00001000u; // UNIT_FLAG_PVP
        constexpr uint32_t kUnitInCombat  = 0x00080000u; // Script_UnitAffectingCombat
        constexpr uint32_t kUnitSkinnable = 0x04000000u; // UNIT_FLAG_SKINNABLE

        // UNIT_DYNAMIC_FLAGS.
        constexpr uint32_t kDynamicLootable        = 0x01u; // UNIT_DYNFLAG_LOOTABLE
        constexpr uint32_t kDynamicTapped          = 0x04u; // Script_UnitIsTapped
        constexpr uint32_t kDynamicTappedByPlayer  = 0x08u; // UNIT_DYNFLAG_TAPPED_BY_PLAYER
        constexpr uint32_t kDynamicDead            = 0x20u; // Script_UnitIsDead

        // PLAYER_FLAGS.
        constexpr uint32_t kPlayerAfk     = 0x02u; // Script_UnitIsAFK
        constexpr uint32_t kPlayerDnd     = 0x04u; // Script_UnitIsDND
        constexpr uint32_t kPlayerGhost   = 0x10u; // Script_UnitIsGhost
        constexpr uint32_t kPlayerResting = 0x20u; // Script_IsResting

        // The movement block's flags.
        constexpr uint32_t kMoveFallingFar = 0x00000800u;
        constexpr uint32_t kMoveFalling    = 0x00001000u; // Script_IsFalling: this, and not the above
        constexpr uint32_t kMoveFlying     = 0x02000000u; // Script_IsFlying

        // UNIT_FIELD_BYTES_1's third byte.
        constexpr uint32_t kVisCreep = 0x02u; // Script_IsStealthed

        constexpr uint32_t kEquipSlots    = 23; // PLAYER_FIELD_INV_SLOT_HEAD: 19 worn, 4 bags
        constexpr uint32_t kBackpackSlots = 16; // PLAYER_FIELD_PACK_SLOT_1
        constexpr uint32_t kSkillLines    = 128;
        constexpr uint32_t kEnchantSlots  = 12;

        wxl_guid GuidAt(const uint32_t* dwords) { return wxl_guid(dwords[0]) | (wxl_guid(dwords[1]) << 32); }

        float Between(const wxl_vec3& a, const wxl_vec3& b)
        {
            const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }
    } // namespace

    bool ObjectService::Dword(void* object, wxl_object_type kind, uint32_t index, uint32_t& out) const
    {
        uint32_t* dwords = nullptr;
        uint32_t  end    = 0;
        if (Block(object, kind, dwords, end) != WXL_OK || index >= end) return false;
        out = dwords[index];
        return true;
    }

    wxl_handle ObjectService::Named(wxl_guid guid) const
    {
        // Tracking is what resolving a guid does: the object is in view, it gets its handle.
        return const_cast<ObjectService*>(this)->FromGuid(guid);
    }

    // --- Any object ----------------------------------------------------------------------------

    bool ObjectService::IsActivePlayer(void* object, wxl_bool& out) const
    {
        out = object == binding_.LocalPlayer() ? WXL_TRUE : WXL_FALSE;
        return true;
    }

    bool ObjectService::Distance(void* object, wxl_handle other, float& out) const
    {
        void* there = handles_.Resolve(other, nullptr);
        if (!there) return false;
        wxl_vec3 a{}, b{};
        if (!binding_.Position(object, a) || !binding_.Position(there, b)) return false;
        out = Between(a, b);
        return true;
    }

    bool ObjectService::DistanceTo(void* object, wxl_vec3 point, float& out) const
    {
        wxl_vec3 at{};
        if (!binding_.Position(object, at)) return false;
        out = Between(at, point);
        return true;
    }

    // --- Units ---------------------------------------------------------------------------------

    namespace
    {
        /// Whether a flag is set in a value a read fetched, when the read could fetch it.
        bool Flag(bool read, uint32_t value, uint32_t mask, wxl_bool& out)
        {
            if (!read) return false;
            out = (value & mask) ? WXL_TRUE : WXL_FALSE;
            return true;
        }
    } // namespace

    bool ObjectService::UnitIsDead(void* object, wxl_bool& out) const
    {
        // Script_UnitIsDead: no health left, or the server's dead flag.
        int32_t  health  = 0;
        uint32_t dynamic = 0;
        if (!binding_.UnitHealth(object, health) || !binding_.UnitDynamicFlags(object, dynamic)) return false;
        out = health <= 0 || (dynamic & kDynamicDead) ? WXL_TRUE : WXL_FALSE;
        return true;
    }

    bool ObjectService::UnitInCombat(void* object, wxl_bool& out) const
    {
        uint32_t flags = 0;
        const bool read = binding_.UnitFlags(object, flags);
        return Flag(read, flags, kUnitInCombat, out);
    }

    bool ObjectService::UnitIsPvp(void* object, wxl_bool& out) const
    {
        uint32_t flags = 0;
        const bool read = binding_.UnitFlags(object, flags);
        return Flag(read, flags, kUnitPvp, out);
    }

    bool ObjectService::UnitIsTapped(void* object, wxl_bool& out) const
    {
        uint32_t dynamic = 0;
        const bool read = binding_.UnitDynamicFlags(object, dynamic);
        return Flag(read, dynamic, kDynamicTapped, out);
    }

    bool ObjectService::UnitIsTappedByPlayer(void* object, wxl_bool& out) const
    {
        uint32_t dynamic = 0;
        const bool read = binding_.UnitDynamicFlags(object, dynamic);
        return Flag(read, dynamic, kDynamicTappedByPlayer, out);
    }

    bool ObjectService::UnitIsLootable(void* object, wxl_bool& out) const
    {
        uint32_t dynamic = 0;
        const bool read = binding_.UnitDynamicFlags(object, dynamic);
        return Flag(read, dynamic, kDynamicLootable, out);
    }

    bool ObjectService::UnitIsSkinnable(void* object, wxl_bool& out) const
    {
        uint32_t flags = 0;
        const bool read = binding_.UnitFlags(object, flags);
        return Flag(read, flags, kUnitSkinnable, out);
    }

    bool ObjectService::UnitStandState(void* object, uint32_t& out) const
    {
        uint32_t bytes = 0;
        if (!Dword(object, WXL_OBJECT_UNIT, field::Unit::Bytes1.index, bytes)) return false;
        out = bytes & 0xFFu;
        return true;
    }

    bool ObjectService::UnitIsSitting(void* object, wxl_bool& out) const
    {
        uint32_t state = 0;
        if (!UnitStandState(object, state)) return false;
        out = state >= WXL_STAND_SITTING && state <= WXL_STAND_SITTING_HIGH ? WXL_TRUE : WXL_FALSE;
        return true;
    }

    bool ObjectService::UnitIsStealthed(void* object, wxl_bool& out) const
    {
        uint32_t bytes = 0;
        const bool read = Dword(object, WXL_OBJECT_UNIT, field::Unit::Bytes1.index, bytes);
        return Flag(read, bytes >> 16, kVisCreep, out);
    }

    bool ObjectService::UnitHasAura(void* object, uint32_t spell, wxl_bool& out) const
    {
        // CGUnit_C::HasAuraBySpellId: any slot holding the spell.
        uint32_t count = 0;
        if (!binding_.UnitAuraCount(object, count)) return false;
        out = WXL_FALSE;
        for (uint32_t slot = 0; slot < count; ++slot)
        {
            wxl_aura aura{};
            aura.struct_size = sizeof aura;
            if (binding_.UnitAura(object, slot, aura) && aura.spell_id == spell)
            {
                out = WXL_TRUE;
                break;
            }
        }
        return true;
    }

    bool ObjectService::UnitIsFlying(void* object, wxl_bool& out) const
    {
        uint32_t flags = 0;
        const bool read = binding_.UnitMovementFlags(object, flags);
        return Flag(read, flags, kMoveFlying, out);
    }

    bool ObjectService::UnitIsFalling(void* object, wxl_bool& out) const
    {
        uint32_t flags = 0;
        if (!binding_.UnitMovementFlags(object, flags)) return false;
        out = (flags & kMoveFalling) && !(flags & kMoveFallingFar) ? WXL_TRUE : WXL_FALSE;
        return true;
    }

    // --- Players -------------------------------------------------------------------------------

    bool ObjectService::PlayerIsGhost(void* object, wxl_bool& out) const
    {
        uint32_t flags = 0;
        const bool read = binding_.PlayerFlags(object, flags);
        return Flag(read, flags, kPlayerGhost, out);
    }

    bool ObjectService::PlayerIsAfk(void* object, wxl_bool& out) const
    {
        uint32_t flags = 0;
        const bool read = binding_.PlayerFlags(object, flags);
        return Flag(read, flags, kPlayerAfk, out);
    }

    bool ObjectService::PlayerIsDnd(void* object, wxl_bool& out) const
    {
        uint32_t flags = 0;
        const bool read = binding_.PlayerFlags(object, flags);
        return Flag(read, flags, kPlayerDnd, out);
    }

    bool ObjectService::PlayerIsResting(void* object, wxl_bool& out) const
    {
        uint32_t flags = 0;
        const bool read = binding_.PlayerFlags(object, flags);
        return Flag(read, flags, kPlayerResting, out);
    }

    bool ObjectService::PlayerXp(void* object, uint32_t& out) const
    {
        return Dword(object, WXL_OBJECT_PLAYER, field::Player::Xp.index, out);
    }

    bool ObjectService::PlayerNextLevelXp(void* object, uint32_t& out) const
    {
        return Dword(object, WXL_OBJECT_PLAYER, field::Player::NextLevelXp.index, out);
    }

    bool ObjectService::PlayerRestedXp(void* object, uint32_t& out) const
    {
        return Dword(object, WXL_OBJECT_PLAYER, field::Player::RestStateExperience.index, out);
    }

    bool ObjectService::PlayerHonorPoints(void* object, uint32_t& out) const
    {
        return Dword(object, WXL_OBJECT_PLAYER, field::Player::HonorCurrency.index, out);
    }

    bool ObjectService::PlayerArenaPoints(void* object, uint32_t& out) const
    {
        return Dword(object, WXL_OBJECT_PLAYER, field::Player::ArenaCurrency.index, out);
    }

    bool ObjectService::PlayerInventoryItem(void* object, uint32_t slot, wxl_handle& out) const
    {
        uint32_t guid[2] = {0, 0};
        if (slot >= kEquipSlots) return false;
        const uint32_t at = field::Player::InvSlotHead.index + 2 * slot;
        if (!Dword(object, WXL_OBJECT_PLAYER, at, guid[0]) || !Dword(object, WXL_OBJECT_PLAYER, at + 1, guid[1])) return false;
        out = Named(GuidAt(guid));
        return true;
    }

    bool ObjectService::PlayerBackpackItem(void* object, uint32_t slot, wxl_handle& out) const
    {
        uint32_t guid[2] = {0, 0};
        if (slot >= kBackpackSlots) return false;
        const uint32_t at = field::Player::PackSlot1.index + 2 * slot;
        if (!Dword(object, WXL_OBJECT_PLAYER, at, guid[0]) || !Dword(object, WXL_OBJECT_PLAYER, at + 1, guid[1])) return false;
        out = Named(GuidAt(guid));
        return true;
    }

    bool ObjectService::PlayerSkill(void* object, uint32_t line, wxl_skill& out) const
    {
        // PLAYER_SKILL_INFO_1_1: three dwords a line -- the id and its step, the value and its
        // maximum, the temporary and the permanent bonus -- each two 16-bit halves, low first.
        if (line >= kSkillLines) return false;
        const uint32_t at       = field::Player::SkillInfo1_1.index + 3 * line;
        uint32_t       dword[3] = {0, 0, 0};
        for (uint32_t i = 0; i < 3; ++i)
            if (!Dword(object, WXL_OBJECT_PLAYER, at + i, dword[i])) return false;
        out.id              = dword[0] & 0xFFFFu;
        out.step            = dword[0] >> 16;
        out.value           = dword[1] & 0xFFFFu;
        out.max             = dword[1] >> 16;
        out.temporary_bonus = static_cast<int16_t>(dword[2] & 0xFFFFu);
        out.permanent_bonus = static_cast<int16_t>(dword[2] >> 16);
        return true;
    }

    bool ObjectService::PlayerSkillOf(void* object, uint32_t id, wxl_skill& out) const
    {
        if (id == 0 || id > 0xFFFFu) return false;
        for (uint32_t line = 0; line < kSkillLines; ++line)
        {
            wxl_skill skill = out;
            if (!PlayerSkill(object, line, skill)) return false;
            if (skill.id != id) continue;
            out = skill;
            return true;
        }
        return false;
    }

    // --- Items ---------------------------------------------------------------------------------

    bool ObjectService::ItemEnchant(void* object, uint32_t slot, uint32_t& out) const
    {
        // ITEM_FIELD_ENCHANTMENT_n_1: three dwords a slot, the enchantment's id first.
        if (slot >= kEnchantSlots) return false;
        return Dword(object, WXL_OBJECT_ITEM, field::Item::Enchantment1_1.index + 3 * slot, out);
    }

    bool ObjectService::ContainerSlotItem(void* object, uint32_t slot, wxl_handle& out) const
    {
        wxl_guid guid = WXL_GUID_NONE;
        if (!binding_.ContainerSlot(object, slot, guid)) return false;
        out = Named(guid);
        return true;
    }
} // namespace wxl::runtime
