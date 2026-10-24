// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The shared runtime scaffolding, checked against a binding backed by plain structs.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "wxl/runtime/object_service.hpp"

namespace
{
    int g_failures = 0;

    void Check(bool condition, const char* what)
    {
        if (condition) return;
        std::printf("  FAIL  %s\n", what);
        ++g_failures;
    }

    void Section(const char* title) { std::printf("%s\n", title); }

    /// A world of plain structs, standing in for a client. One struct for every kind; a read a
    /// kind does not have answers from the zeroed field, which the service never asks for.
    struct FakeObject
    {
        wxl_guid        guid = 0;
        wxl_object_type type = WXL_OBJECT_UNKNOWN;
        wxl_vec3        position{};
        float           facing = 0.0f;
        std::string     name;
        uint32_t        entry = 0;
        uint32_t        health    = 0;
        uint32_t        maxHealth = 0;
        uint32_t        level     = 0;
        wxl_guid        target    = 0;
        uint32_t        power[7]{};
        uint32_t        maxPower[7]{};
        uint32_t        powerType = 0;
        uint32_t        visibleItem[19]{};
        uint32_t        stackCount = 0;
        uint32_t        numSlots   = 0;
        wxl_guid        slots[36]{};
        uint32_t        goType = 0;
        uint32_t        spellId = 0;
        wxl_guid        owner   = 0;
    };

    /// Every read of the table answering zero, so a row added to it needs nothing here.
    class ZeroBinding : public wxl::runtime::ObjectBinding
    {
    public:
#define WXL_OBJECT_READ(kinds, member, Method, type) \
    bool Method(void*, type& out) const override       \
    {                                                  \
        out = type{};                                  \
        return true;                                   \
    }
#define WXL_OBJECT_READ_AT(kinds, member, Method, type)    \
    bool Method(void*, uint32_t, type& out) const override \
    {                                                      \
        out = type{};                                      \
        return true;                                       \
    }
#include "wxl/runtime/object_reads.def"
#undef WXL_OBJECT_READ
#undef WXL_OBJECT_READ_AT
    };

    /// The reads the checks below look at, from the fake objects.
    class FakeBinding final : public ZeroBinding
    {
    public:
        FakeObject*  localPlayer = nullptr;
        mutable int  typeReads   = 0;
        /// What the client holds, tracked or not: what FindByGuid looks through.
        std::vector<FakeObject*> held;

        static FakeObject* As(void* object) { return static_cast<FakeObject*>(object); }

        template <class T>
        static bool Give(T& out, const T& value)
        {
            out = value;
            return true;
        }

        wxl_guid        Guid(void* o) const override { return As(o)->guid; }
        wxl_object_type Type(void* o) const override
        {
            ++typeReads;
            return As(o)->type;
        }
        bool Name(void* o, const char*& out, uint32_t& size) const override
        {
            const std::string& name = As(o)->name;
            out                     = name.c_str();
            size                    = static_cast<uint32_t>(name.size());
            return true;
        }
        bool  UnitTarget(void* o, wxl_guid& out) const override { return Give(out, As(o)->target); }
        void* LocalPlayer() const override { return localPlayer; }
        void* FindByGuid(wxl_guid guid) const override
        {
            for (FakeObject* object : held)
                if (object->guid == guid) return object;
            return nullptr;
        }

        bool Entry(void* o, uint32_t& out) const override { return Give(out, As(o)->entry); }
        bool Position(void* o, wxl_vec3& out) const override { return Give(out, As(o)->position); }
        bool Facing(void* o, float& out) const override { return Give(out, As(o)->facing); }

        bool UnitHealth(void* o, uint32_t& out) const override { return Give(out, As(o)->health); }
        bool UnitMaxHealth(void* o, uint32_t& out) const override { return Give(out, As(o)->maxHealth); }
        bool UnitLevel(void* o, uint32_t& out) const override { return Give(out, As(o)->level); }
        bool UnitPower(void* o, uint32_t index, uint32_t& out) const override
        {
            return index < 7 && Give(out, As(o)->power[index]);
        }
        bool UnitMaxPower(void* o, uint32_t index, uint32_t& out) const override
        {
            return index < 7 && Give(out, As(o)->maxPower[index]);
        }
        bool UnitPowerType(void* o, uint32_t& out) const override { return Give(out, As(o)->powerType); }

        bool PlayerVisibleItem(void* o, uint32_t slot, uint32_t& out) const override
        {
            return slot < 19 && Give(out, As(o)->visibleItem[slot]);
        }

        bool ItemOwner(void* o, wxl_guid& out) const override { return Give(out, As(o)->owner); }
        bool ItemStackCount(void* o, uint32_t& out) const override { return Give(out, As(o)->stackCount); }

        bool ContainerNumSlots(void* o, uint32_t& out) const override { return Give(out, As(o)->numSlots); }
        bool ContainerSlot(void* o, uint32_t index, wxl_guid& out) const override
        {
            return index < As(o)->numSlots && Give(out, As(o)->slots[index]);
        }

        bool GameObjectType(void* o, uint32_t& out) const override { return Give(out, As(o)->goType); }
        bool DynamicCaster(void* o, wxl_guid& out) const override { return Give(out, As(o)->owner); }
        bool DynamicSpellId(void* o, uint32_t& out) const override { return Give(out, As(o)->spellId); }
    };
} // namespace

int main()
{
    // Unbuffered, so the last line printed before a crash is never held back in a buffer.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    FakeObject player;
    player.guid      = 0x100;
    player.type      = WXL_OBJECT_PLAYER;
    player.position  = {10.0f, 20.0f, 30.0f};
    player.facing    = 1.5f;
    player.name      = "Thorgrim";
    player.health    = 4200;
    player.maxHealth = 5000;
    player.level     = 80;
    player.target    = 0x200;
    player.power[0]  = 900;
    player.powerType = 0;
    player.visibleItem[15] = 19019;

    FakeObject boar;
    boar.guid      = 0x200;
    boar.type      = WXL_OBJECT_UNIT;
    boar.position  = {11.0f, 21.0f, 31.0f};
    boar.name      = "Boar";
    boar.entry     = 113;
    boar.health    = 90;
    boar.maxHealth = 120;
    boar.level     = 3;

    FakeObject chest;
    chest.guid     = 0x300;
    chest.type     = WXL_OBJECT_GAMEOBJECT;
    chest.position = {12.0f, 22.0f, 32.0f};
    chest.name     = "Chest";
    chest.goType   = 3;

    FakeObject bag;
    bag.guid       = 0x400;
    bag.type       = WXL_OBJECT_CONTAINER;
    bag.name       = "Bag";
    bag.stackCount = 1;
    bag.numSlots   = 2;
    bag.slots[1]   = 0x401;
    bag.owner      = 0x100;

    FakeObject cloud;
    cloud.guid    = 0x500;
    cloud.type    = WXL_OBJECT_DYNAMIC;
    cloud.spellId = 42;
    cloud.owner   = 0x100;

    FakeBinding binding;
    binding.localPlayer = &player;

    wxl::runtime::ObjectService service(binding);
    const wxl_object_api*       api = service.Api();

    Section("tracking");
    const wxl_handle playerHandle = service.Track(&player);
    const wxl_handle boarHandle   = service.Track(&boar);
    const wxl_handle chestHandle  = service.Track(&chest);
    const wxl_handle bagHandle    = service.Track(&bag);
    const wxl_handle cloudHandle  = service.Track(&cloud);
    {
        Check(api->struct_size == sizeof(wxl_object_api), "the table reports its own size");
        Check(playerHandle != WXL_HANDLE_NONE, "tracking mints a handle");
        Check(service.Track(&player) == playerHandle, "tracking the same object answers the same");
        Check(service.Handles().Size() == 5, "five objects are live");
        Check(api->valid(playerHandle) == WXL_TRUE, "a live handle is valid");
        Check(api->valid(WXL_HANDLE_NONE) == WXL_FALSE, "the null handle never is");
        Check(api->valid(0xDEADBEEF) == WXL_FALSE, "nor is a made-up one");
    }

    Section("reading");
    {
        wxl_vec3 position{};
        Check(api->position(playerHandle, &position) == WXL_OK, "a position reads");
        Check(position.x == 10.0f && position.y == 20.0f, "and arrives intact");

        wxl_str name{};
        Check(api->name(boarHandle, &name) == WXL_OK, "a name reads");
        Check(name.size == 4 && std::strncmp(name.data, "Boar", 4) == 0, "with its length");

        Check(api->guid(playerHandle) == 0x100, "a guid reads");
        Check(api->type(chestHandle) == WXL_OBJECT_GAMEOBJECT, "a type reads");
        Check(api->from_guid(0x200) == boarHandle, "a guid resolves back to its handle");
        Check(api->from_guid(0x999) == WXL_HANDLE_NONE, "an unknown guid resolves to nothing");
        Check(api->local_player() == playerHandle, "the local player resolves");

        uint32_t entry = 0;
        Check(api->entry(boarHandle, &entry) == WXL_OK && entry == 113, "an entry reads on any object");
    }

    Section("type gating");
    {
        uint32_t health = 0;
        Check(api->unit_health(boarHandle, &health) == WXL_OK && health == 90, "a unit has health");
        Check(api->unit_health(chestHandle, &health) == WXL_E_INVALID,
              "a game object asked for health is a mistake, not a missing value");
        Check(api->unit_health(0xDEADBEEF, &health) == WXL_E_NOT_FOUND,
              "an unknown handle is not found, which is a different thing");
        Check(api->unit_health(boarHandle, nullptr) == WXL_E_INVALID, "a null out is refused");

        wxl_handle target = WXL_HANDLE_NONE;
        Check(api->unit_target(playerHandle, &target) == WXL_OK && target == boarHandle,
              "a target guid resolves to the handle of the unit in view");
        Check(api->unit_target(boarHandle, &target) == WXL_OK && target == WXL_HANDLE_NONE,
              "no target resolves to the null handle");

        uint32_t value = 0;
        Check(api->unit_power(playerHandle, 0, &value) == WXL_OK && value == 900, "an indexed read reads");
        Check(api->unit_power(playerHandle, 7, &value) == WXL_E_INVALID, "past its bound it is invalid");
        Check(api->player_visible_item(playerHandle, 15, &value) == WXL_OK && value == 19019,
              "a player's visible item reads");
        Check(api->player_visible_item(boarHandle, 15, &value) == WXL_E_INVALID,
              "a creature has no equipment slots");
        Check(api->item_stack_count(bagHandle, &value) == WXL_OK && value == 1,
              "a container answers the item reads");
        Check(api->container_num_slots(bagHandle, &value) == WXL_OK && value == 2,
              "and its own");
        wxl_guid slot = 0;
        Check(api->container_slot(bagHandle, 1, &slot) == WXL_OK && slot == 0x401, "a bag slot reads");
        Check(api->container_slot(bagHandle, 2, &slot) == WXL_E_INVALID, "past the bag's slots it is invalid");
        Check(api->container_num_slots(chestHandle, &value) == WXL_E_INVALID,
              "a chest is a game object, not a container");
        Check(api->gameobject_type(chestHandle, &value) == WXL_OK && value == 3, "a game-object type reads");
        Check(api->dynamic_spell_id(cloudHandle, &value) == WXL_OK && value == 42, "a dynamic object's spell reads");
        Check(api->dynamic_spell_id(playerHandle, &value) == WXL_E_INVALID, "a player is not a dynamic object");
        Check(api->corpse_display_id(playerHandle, &value) == WXL_E_INVALID, "nor a corpse");
    }

    Section("an object's kind");
    {
        const int reads = binding.typeReads;
        uint32_t  value = 0;
        api->unit_health(boarHandle, &value);
        api->unit_health(chestHandle, &value);
        api->type(chestHandle);
        api->enumerate(WXL_OBJECT_UNIT, nullptr, 0);
        Check(reads == 5 && binding.typeReads == reads,
              "is read once per object, when it is tracked, and never asked of the binding again");
    }

    Section("enumeration");
    {
        Check(api->enumerate(WXL_OBJECT_UNKNOWN, nullptr, 0) == 5, "counting without a buffer");
        Check(api->enumerate(WXL_OBJECT_UNIT, nullptr, 0) == 1, "counting one type");

        wxl_handle found[4]      = {};
        const uint32_t unitCount = api->enumerate(WXL_OBJECT_UNIT, found, 4);
        Check(unitCount == 1 && found[0] == boarHandle, "and listing it");

        wxl_handle       one       = WXL_HANDLE_NONE;
        const uint32_t   overflow  = api->enumerate(WXL_OBJECT_UNKNOWN, &one, 1);
        Check(overflow == 5, "a short buffer still reports the real total");
        Check(one != WXL_HANDLE_NONE, "and fills what it can");
    }

    Section("bulk");
    {
        const wxl_handle handles[3] = {playerHandle, 0xDEADBEEF, boarHandle};
        wxl_vec3         out[3]{};
        Check(api->positions(handles, 3, out) == WXL_OK, "positions read in bulk");
        Check(out[0].x == 10.0f, "a live handle gives its position");
        Check(out[1].x == 0.0f && out[1].y == 0.0f, "a dead one gives the origin, not garbage");
        Check(out[2].x == 11.0f, "and the rest still read");
    }

    Section("staleness");
    {
        service.Untrack(&boar);
        Check(api->valid(boarHandle) == WXL_FALSE, "a retired handle stops resolving");
        Check(service.Handles().Size() == 4, "and the table shrinks");

        wxl_handle target = WXL_HANDLE_NONE;
        Check(api->unit_target(playerHandle, &target) == WXL_OK && target == WXL_HANDLE_NONE,
              "a target that left view resolves to the null handle");

        // The retired slot is reused. The point of the generation is that the old handle must
        // not come back to life pointing at whatever took the slot.
        FakeObject wolf;
        wolf.guid   = 0x400;
        wolf.type   = WXL_OBJECT_UNIT;
        wolf.health = 50;
        const wxl_handle wolfHandle = service.Track(&wolf);
        Check(wolfHandle != boarHandle, "a reused slot mints a different handle");
        Check(api->valid(boarHandle) == WXL_FALSE, "and the old handle stays dead");
        Check(api->valid(wolfHandle) == WXL_TRUE, "while the new one lives");

        uint32_t health = 0;
        Check(api->unit_health(boarHandle, &health) == WXL_E_NOT_FOUND,
              "reading through a stale handle finds nothing rather than the wrong unit");

        // Looking an object up by guid is an index, not a walk, so the index has to be right
        // in the places an index can be wrong: when the object goes, and when the client hands
        // the same guid to something else.
        Check(api->from_guid(0x200) == WXL_HANDLE_NONE,
              "the guid of a retired object resolves to nothing");
        Check(api->from_guid(0x400) == wolfHandle, "while a live one still resolves");

        FakeObject heir;
        heir.guid = 0x400; // the wolf's, reused by the client for something else
        heir.type = WXL_OBJECT_UNIT;
        const wxl_handle heirHandle = service.Track(&heir);
        Check(heirHandle != wolfHandle, "a second object with a guid in use gets its own handle");
        Check(api->from_guid(0x400) == heirHandle, "and the guid now names the one that holds it");

        service.Untrack(&wolf);
        Check(api->valid(wolfHandle) == WXL_FALSE, "retiring the first one retires its handle");
        Check(api->from_guid(0x400) == heirHandle,
              "but leaves the guid with the object that took it over");

        service.Untrack(heirHandle);
        Check(api->valid(heirHandle) == WXL_FALSE, "a handle can be retired by handle");
        Check(api->from_guid(0x400) == WXL_HANDLE_NONE, "and that frees the guid");
        service.Untrack(heirHandle);
        Check(api->from_guid(0x400) == WXL_HANDLE_NONE, "retiring a stale handle changes nothing");

        service.Clear();
        Check(api->valid(wolfHandle) == WXL_FALSE && api->valid(playerHandle) == WXL_FALSE,
              "clearing retires everything, the local player included");
        binding.localPlayer = nullptr;
        Check(api->local_player() == WXL_HANDLE_NONE, "with no local player there is none");
        Check(api->from_guid(0x100) == WXL_HANDLE_NONE, "and the guid index is empty with it");
    }

    Section("what the client holds and no seam announced");
    {
        // The client revived the boar by a path no seam sees: it holds it, nothing tracks it.
        binding.held = {&boar, &player};
        const wxl_handle found = api->from_guid(0x200);
        Check(found != WXL_HANDLE_NONE && found != boarHandle,
              "a guid the client holds resolves, with a handle of its own episode");
        Check(api->valid(found) == WXL_TRUE && api->type(found) == WXL_OBJECT_UNIT,
              "the object is tracked from there, its kind read once");
        Check(api->from_guid(0x200) == found && service.HandleOf(&boar) == found,
              "and looked up again it is the same handle, from the index");
        Check(api->from_guid(0x999) == WXL_HANDLE_NONE, "a guid the client does not hold is nothing");

        binding.localPlayer = &player;
        const wxl_handle me = api->local_player();
        Check(me != WXL_HANDLE_NONE && me != playerHandle && api->from_guid(0x100) == me,
              "the local player is tracked on demand too, under its guid");

        uint32_t   health = 0;
        wxl_handle target = WXL_HANDLE_NONE;
        Check(api->unit_target(me, &target) == WXL_OK && target == found && api->unit_health(target, &health) == WXL_OK &&
                  health == 90,
              "a target resolves through the same lookup");
        service.Untrack(found);
        binding.held.clear();
        Check(api->from_guid(0x200) == WXL_HANDLE_NONE, "once the client lets it go, nothing resolves");
    }

    if (g_failures == 0)
    {
        std::printf("\nall checks passed\n");
        return 0;
    }

    std::printf("\n%d check(s) failed\n", g_failures);
    return 1;
}
