// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The shared runtime scaffolding, checked against a binding backed by plain structs.

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include "wxl/objects/fields.hpp"
#include "wxl/runtime/object_service.hpp"

namespace getters
{
    // A getter answers a field as the client holds it in the type update_fields.def gives it:
    // getter_fields.def pairs them, and the build fails on a pair that disagrees.
    template <class F>
    struct OutOf;
    template <class V>
    struct OutOf<wxl_result(WXL_CALL*)(wxl_handle, V*)>
    {
        using type = V;
    };
    template <class V>
    struct OutOf<wxl_result(WXL_CALL*)(wxl_handle, uint32_t, V*)>
    {
        using type = V;
    };

    template <class T>
    struct ElementOf;
    template <class T>
    struct ElementOf<const wxl::field::Field<T>>
    {
        using type = T;
    };

#define WXL_GETTER_FIELD(member, Kind, Name)                                                  \
    static_assert(std::is_same_v<OutOf<decltype(wxl_object_api::member)>::type,               \
                                 ElementOf<decltype(wxl::field::Kind::Name)>::type>,           \
                  #member " answers another type than " #Kind "::" #Name " holds");
#include "getter_fields.def"
#undef WXL_GETTER_FIELD
} // namespace getters

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
        int32_t         health    = 0;
        uint32_t        maxHealth = 0;
        uint32_t        level     = 0;
        wxl_guid        target    = 0;
        uint32_t        power[7]{};
        uint32_t        maxPower[7]{};
        uint32_t        powerType = 0;
        uint32_t        visibleItem[19]{};
        int32_t         stackCount = 0;
        uint32_t        numSlots   = 0;
        wxl_guid        slots[36]{};
        uint32_t        goType = 0;
        uint32_t        spellId = 0;
        wxl_guid        owner   = 0;
        std::vector<uint32_t> block; ///< the descriptor block; empty for none
        uint32_t        unitFlags     = 0;
        uint32_t        dynamicFlags  = 0;
        uint32_t        playerFlags   = 0;
        uint32_t        movementFlags = 0;
        std::vector<wxl_aura> auras;
        uint32_t        quality = 0;
        bool            cached  = false; ///< the item cache holds its record
    };

    /// Every read of the table answering zero, so a row added to it needs nothing here.
    class ZeroBinding : public wxl::runtime::ObjectBinding
    {
    public:
#define WXL_OBJECT_READ(kinds, member, Method, type, source, Cxx, cxx, doc) \
    WXL_OBJECT_BINDS(source, bool Method(void*, type& out) const override { out = type{}; return true; })
#define WXL_OBJECT_READ_AT(kinds, member, Method, type, source, Cxx, cxx, Arg, arg, doc) \
    WXL_OBJECT_BINDS(source, bool Method(void*, Arg, type& out) const override { out = type{}; return true; })
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
        bool Position(void* o, wxl_vec3& out) const override
        {
            // An item has no place in the world, as the client's own reads say.
            const wxl_object_type type = As(o)->type;
            return type != WXL_OBJECT_ITEM && type != WXL_OBJECT_CONTAINER && Give(out, As(o)->position);
        }
        bool Facing(void* o, float& out) const override { return Give(out, As(o)->facing); }

        bool UnitHealth(void* o, int32_t& out) const override { return Give(out, As(o)->health); }
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
        bool ItemStackCount(void* o, int32_t& out) const override { return Give(out, As(o)->stackCount); }

        bool ContainerNumSlots(void* o, uint32_t& out) const override { return Give(out, As(o)->numSlots); }
        bool ContainerSlot(void* o, uint32_t index, wxl_guid& out) const override
        {
            return index < As(o)->numSlots && Give(out, As(o)->slots[index]);
        }

        uint32_t* Descriptor(void* o) const override
        {
            std::vector<uint32_t>& block = As(o)->block;
            return block.empty() ? nullptr : block.data();
        }

        /// What the service asked the client to watch, and whether this client can.
        struct Watched
        {
            wxl_object_type kind;
            uint32_t        first;
            uint32_t        dwords;
            bool            local;
        };
        mutable std::vector<Watched>     watched;
        bool                             canWatch = true;
        mutable std::vector<std::string> reports;

        bool WatchElement(wxl_object_type kind, uint32_t first, uint32_t dwords, bool local) const override
        {
            if (!canWatch) return false;
            watched.push_back(Watched{kind, first, dwords, local});
            return true;
        }

        /// What the service let go of, and whether this client can take a handler back.
        mutable std::vector<Watched> unwatched;
        bool                         canUnwatch = true;

        bool UnwatchElement(wxl_object_type kind, uint32_t first, uint32_t dwords, bool local) const override
        {
            if (!canUnwatch) return false;
            unwatched.push_back(Watched{kind, first, dwords, local});
            return true;
        }
        void Report(const char* line) const override { reports.emplace_back(line); }

        bool Reported(const char* line) const
        {
            for (const std::string& report : reports)
                if (report == line) return true;
            return false;
        }

        bool GameObjectType(void* o, uint32_t& out) const override { return Give(out, As(o)->goType); }

        bool UnitFlags(void* o, uint32_t& out) const override { return Give(out, As(o)->unitFlags); }
        bool UnitDynamicFlags(void* o, uint32_t& out) const override { return Give(out, As(o)->dynamicFlags); }
        bool PlayerFlags(void* o, uint32_t& out) const override { return Give(out, As(o)->playerFlags); }
        bool UnitMovementFlags(void* o, uint32_t& out) const override { return Give(out, As(o)->movementFlags); }
        bool UnitAuraCount(void* o, uint32_t& out) const override
        {
            return Give(out, static_cast<uint32_t>(As(o)->auras.size()));
        }
        bool UnitAura(void* o, uint32_t slot, wxl_aura& out) const override
        {
            return slot < As(o)->auras.size() && Give(out, As(o)->auras[slot]);
        }
        bool ItemQuality(void* o, uint32_t& out) const override
        {
            return As(o)->cached && Give(out, As(o)->quality);
        }
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
        int32_t health = 0;
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
        int32_t stack = 0;
        Check(api->item_stack_count(bagHandle, &stack) == WXL_OK && stack == 1,
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
        int32_t   health = 0;
        api->unit_health(boarHandle, &health);
        api->unit_health(chestHandle, &health);
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

        int32_t health = 0;
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

        int32_t    health = 0;
        wxl_handle target = WXL_HANDLE_NONE;
        Check(api->unit_target(me, &target) == WXL_OK && target == found && api->unit_health(target, &health) == WXL_OK &&
                  health == 90,
              "a target resolves through the same lookup");
        service.Untrack(found);
        binding.held.clear();
        Check(api->from_guid(0x200) == WXL_HANDLE_NONE, "once the client lets it go, nothing resolves");
    }

    Section("descriptor fields");
    {
        FakeObject me;
        me.guid = 0x700;
        me.type = WXL_OBJECT_PLAYER;
        me.block.assign(0x52E, 0u);
        me.block[0x18] = 4200;
        FakeObject other;
        other.guid = 0x710;
        other.type = WXL_OBJECT_PLAYER;
        other.block.assign(0x52E, 0u);
        FakeObject wolf;
        wolf.guid = 0x720;
        wolf.type = WXL_OBJECT_UNIT;
        wolf.block.assign(0x94, 0u);
        wolf.block[0x18] = 77;
        FakeObject rock;
        rock.guid = 0x730;
        rock.type = WXL_OBJECT_GAMEOBJECT; // no block the runtime can reach
        binding.localPlayer = &me;
        binding.held        = {&me, &other, &wolf};
        const wxl_handle hMe = service.Track(&me), hOther = service.Track(&other);
        const wxl_handle hWolf = service.Track(&wolf), hRock = service.Track(&rock);

        Check(api->field_count(hMe) == 0x52E && api->field_count(hOther) == 0x144 &&
                  api->field_count(hWolf) == 0x94 && api->field_count(hRock) == 0 && api->field_count(0xDEAD) == 0,
              "field_count is the kind's block, a player other than the active one's shorter");

        uint32_t two[2] = {0, 0};
        Check(api->read_fields(hWolf, 0x18, 1, two) == WXL_OK && two[0] == 77, "read_fields reads by index");
        Check(api->read_fields(hWolf, 0x93, 2, two) == WXL_E_INVALID &&
                  binding.Reported("read_fields: dwords 0x93..0x94 run past a Unit's 0x94"),
              "a range past the block is refused and named");
        Check(api->read_fields(hOther, 0x27A, 1, two) == WXL_E_INVALID &&
                  binding.Reported("read_fields: dword 0x27A is past a Player's 0x144: a player other than the "
                                   "active one has no fields past PLAYER_FIELD_PAD_0"),
              "and another player's local fields say why they are not there");
        Check(api->read_fields(hRock, 0, 1, two) == WXL_E_UNSUPPORTED, "an object the runtime cannot reach is unsupported");
        Check(api->read_fields(0xDEAD, 0, 1, two) == WXL_E_NOT_FOUND, "a stale handle is not found");

        const uint32_t ninety = 90;
        Check(api->write_fields(hWolf, 0x18, 1, &ninety, WXL_FIELD_WRITE_PLAIN) == WXL_OK && wolf.block[0x18] == 90,
              "write_fields writes the client's dword");
        Check(api->write_fields(hWolf, 0x95, 1, &ninety, 0) == WXL_E_INVALID &&
                  binding.Reported("write_fields: dword 0x95 is past a Unit's 0x94"),
              "past the block it is refused, as the log says");
        Check(api->write_fields(hWolf, 0x18, 1, &ninety, 0x1) == WXL_E_INVALID &&
                  binding.Reported("write_fields: flags 0x1 are not known to this runtime; 0 is a plain write"),
              "an unknown flag is refused");
        Check(api->write_fields(hWolf, 0x2, 1, &ninety, 0) == WXL_E_INVALID && wolf.block[0x2] == 0,
              "the guid and the type are not written");

        const wxl_handle many[] = {hMe, hWolf, 0xDEAD, hOther};
        uint32_t         out[4] = {1, 1, 1, 1};
        wxl_result       results[4];
        Check(api->read_field_many(many, 4, 0x18, 1, out, results) == WXL_OK && out[0] == 4200 && out[1] == 90 &&
                  out[2] == 0 && results[2] == WXL_E_NOT_FOUND && results[3] == WXL_OK,
              "read_field_many reads each object, zeros and a result for one that does not answer");

        Check(api->watch_field(WXL_OBJECT_UNIT, 0x18, 1) == WXL_OK && binding.watched.size() == 1 &&
                  binding.watched[0].kind == WXL_OBJECT_UNIT && binding.watched[0].first == 0x18 &&
                  !binding.watched[0].local,
              "watching UNIT_FIELD_HEALTH asks the client once, on the unit's fields");
        Check(api->watch_field(WXL_OBJECT_PLAYER, 0x18, 1) == WXL_OK && binding.watched.size() == 1,
              "watching it again for players asks nothing more");
        Check(api->watch_field(WXL_OBJECT_PLAYER, 0x27A, 1) == WXL_OK && binding.watched.size() == 2 &&
                  binding.watched[1].kind == WXL_OBJECT_PLAYER && binding.watched[1].local,
              "PLAYER_XP is watched on the active player");
        Check(api->watch_field(WXL_OBJECT_UNIT, 0x12, 2) == WXL_OK && binding.watched.size() == 3 &&
                  binding.watched[2].dwords == 2,
              "a guid is one element of two dwords");
        Check(api->watch_field(WXL_OBJECT_UNIT, 0x13, 1) == WXL_E_INVALID &&
                  binding.Reported("watch_field: dword 0x13 is the second half of an element of UNIT_FIELD_TARGET "
                                   "(from 0x12); start the range at 0x12"),
              "half a guid is refused");
        Check(api->watch_field(WXL_OBJECT_UNIT, 0x10, 2) == WXL_E_UNSUPPORTED &&
                  binding.Reported("watch_field: UNIT_FIELD_CREATEDBY (dword 0x10) is not one the client sees "
                                   "change: it keeps no copy of it from before an update"),
              "a field the client keeps no copy of is unsupported, and named");
        Check(api->watch_field(WXL_OBJECT_UNIT, 0x95, 1) == WXL_E_INVALID &&
                  binding.Reported("watch_field: dword 0x95 is past a Unit's 0x94"),
              "past a kind's block is invalid");
        Check(api->watch_field(9, 0, 1) == WXL_E_INVALID, "a type past the client's is invalid");

        wxl_object_field_args args{};
        const uint32_t        was = 80;
        Check(service.FieldChange(0x720, WXL_OBJECT_UNIT, 0x18, 1, &was, args) && args.object == hWolf &&
                  args.type == WXL_OBJECT_UNIT && args.field == 0x18 && args.count == 1 && args.old_value == 80 &&
                  args.new_value == 90,
              "a reported change becomes the event, the new value read from the block");
        const uint32_t xp = 5;
        me.block[0x27A] = 6;
        Check(service.FieldChange(0x700, WXL_OBJECT_PLAYER, 0x27A, 1, &xp, args) && args.old_value == 5 &&
                  args.new_value == 6,
              "the active player's local field too");
        Check(!service.FieldChange(0x720, WXL_OBJECT_UNIT, 0x3B, 1, &was, args),
              "a field no watch asked for raises nothing");
        Check(!service.FieldChange(0x999, WXL_OBJECT_UNIT, 0x18, 1, &was, args),
              "nor an object the client does not hold");

        api->watch_field(WXL_OBJECT_PLAYER, 0x16, 1);
        Check(service.FieldChange(0x700, WXL_OBJECT_UNIT, 0x16, 1, &was, args) &&
                  !service.FieldChange(0x720, WXL_OBJECT_UNIT, 0x16, 1, &was, args),
              "a unit field watched for players raises for a player and not for a creature");

        // A watch is its caller's: the runtime says who calls, and asks Delivers who receives.
        uint32_t caller = 5;
        service.SetCaller([](void* user) { return *static_cast<uint32_t*>(user); }, &caller);
        const size_t asked = binding.watched.size();
        Check(api->watch_field(WXL_OBJECT_UNIT, 0x20, 1) == WXL_OK && binding.watched.size() == asked + 1,
              "a mod's first watch of UNIT_FIELD_MAXHEALTH asks the client");
        Check(service.FieldChange(0x720, WXL_OBJECT_UNIT, 0x20, 1, &was, args) && service.Delivers(5, args) &&
                  !service.Delivers(0, args) && !service.Delivers(6, args),
              "its changes are delivered to that mod only");
        caller = 6;
        Check(api->watch_field(WXL_OBJECT_PLAYER, 0x20, 1) == WXL_OK && binding.watched.size() == asked + 1,
              "another mod's watch of it, for players, asks nothing more");
        Check(!service.Delivers(6, args), "and receives no creature's change");
        Check(service.FieldChange(0x700, WXL_OBJECT_UNIT, 0x20, 1, &was, args) && service.Delivers(6, args) &&
                  service.Delivers(5, args),
              "a player's reaches both: a watch for units covers players");
        caller = 5;
        Check(api->unwatch_field(WXL_OBJECT_UNIT, 0x20, 1) == WXL_OK && binding.unwatched.empty() &&
                  !service.Delivers(5, args) && service.Delivers(6, args),
              "giving one back stops that mod's changes and keeps the client's handler for the other");
        Check(api->unwatch_field(WXL_OBJECT_UNIT, 0x20, 1) == WXL_OK, "giving back twice is no mistake");
        Check(service.WatchCount(6) == 1, "the other holds its one watch");
        service.DropWatches(6);
        Check(service.WatchCount(6) == 0 && binding.unwatched.size() == 1 && binding.unwatched[0].first == 0x20 &&
                  !service.FieldChange(0x700, WXL_OBJECT_UNIT, 0x20, 1, &was, args),
              "a mod that goes gives back its watches, the last one lets the client's handler go");

        Check(api->watch_field(WXL_OBJECT_UNIT, 0x18, 1) == WXL_OK && api->watch_field(WXL_OBJECT_UNIT, 0x18, 1) == WXL_OK &&
                  api->unwatch_field(WXL_OBJECT_UNIT, 0x18, 1) == WXL_OK &&
                  service.FieldChange(0x720, WXL_OBJECT_UNIT, 0x18, 1, &was, args) && service.Delivers(5, args),
              "each watch is given back on its own: two asked, one given back, the field still comes");
        Check(api->unwatch_field(WXL_OBJECT_UNIT, 0x18, 1) == WXL_OK && !service.Delivers(5, args) &&
                  service.Delivers(0, args) && binding.unwatched.size() == 1,
              "the second given back ends it, while the watch of nobody's code goes on");
        Check(api->unwatch_field(WXL_OBJECT_UNIT, 0x13, 1) == WXL_E_INVALID &&
                  binding.Reported("unwatch_field: dword 0x13 is the second half of an element of UNIT_FIELD_TARGET "
                                   "(from 0x12); start the range at 0x12"),
              "unwatch_field checks the range as watch_field does");

        binding.canUnwatch = false;
        Check(api->watch_field(WXL_OBJECT_UNIT, 0x3B, 1) == WXL_OK && api->unwatch_field(WXL_OBJECT_UNIT, 0x3B, 1) == WXL_OK &&
                  !service.FieldChange(0x720, WXL_OBJECT_UNIT, 0x3B, 1, &was, args),
              "a client that keeps its handler reports into nothing once no watch is left");
        const size_t kept = binding.watched.size();
        Check(api->watch_field(WXL_OBJECT_UNIT, 0x3B, 1) == WXL_OK && binding.watched.size() == kept &&
                  service.FieldChange(0x720, WXL_OBJECT_UNIT, 0x3B, 1, &was, args),
              "and the next watch reuses the handler it kept");
        service.SetCaller(nullptr, nullptr);

        binding.canWatch = false;
        Check(api->watch_field(WXL_OBJECT_UNIT, 0x19, 1) == WXL_E_UNSUPPORTED &&
                  binding.Reported("watch_field: this runtime cannot watch fields"),
              "a runtime that cannot watch says so");
        binding.held.clear();
    }

    Section("the typed reads (abi 0.1.18)");
    {
        // The shared reads work on what the binding reads: flags, the descriptor block, auras.
        boar.health       = 0;
        boar.dynamicFlags = 0x04 | 0x01; // tapped, lootable
        boar.unitFlags    = 0x00080000u | 0x04000000u; // in combat, skinnable
        boar.movementFlags = 0x00001000u;
        boar.block.assign(0x94, 0u);
        boar.block[0x4A] = 0x00020001u; // stand state 1 (sitting), vis flag 2 (stealthed)
        wxl_aura frost{};
        frost.spell_id = 116;
        frost.stacks   = 2;
        frost.caster   = 0x100;
        boar.auras     = {frost};

        // Earlier sections let these go: tracked again, under new handles.
        binding.localPlayer = &player;
        const wxl_handle playerHandle = service.Track(&player);
        const wxl_handle boarHandle   = service.Track(&boar);
        const wxl_handle bagHandle    = service.Track(&bag);

        wxl_bool yes = WXL_FALSE;
        Check(api->is_active_player(playerHandle, &yes) == WXL_OK && yes == WXL_TRUE &&
                  api->is_active_player(boarHandle, &yes) == WXL_OK && yes == WXL_FALSE,
              "IsActivePlayer tells the active player from the others");
        float yards = 0.0f;
        Check(api->distance(playerHandle, boarHandle, &yards) == WXL_OK && yards > 1.73f && yards < 1.74f,
              "Distance is centre to centre, in the world's three axes");
        Check(api->distance(playerHandle, bagHandle, &yards) == WXL_E_INVALID,
              "an object with no place in the world has no distance");
        Check(api->distance_to(boarHandle, wxl_vec3{11.0f, 21.0f, 34.0f}, &yards) == WXL_OK && yards == 3.0f,
              "DistanceTo measures to a point");
        Check(api->unit_is_dead(boarHandle, &yes) == WXL_OK && yes == WXL_TRUE, "no health left is dead");
        Check(api->unit_is_dead(playerHandle, &yes) == WXL_OK && yes == WXL_FALSE, "health left is alive");
        boar.health = 90;
        boar.dynamicFlags |= 0x20;
        Check(api->unit_is_dead(boarHandle, &yes) == WXL_OK && yes == WXL_TRUE, "and so is the server's dead flag");
        Check(api->unit_in_combat(boarHandle, &yes) == WXL_OK && yes == WXL_TRUE &&
                  api->unit_is_skinnable(boarHandle, &yes) == WXL_OK && yes == WXL_TRUE &&
                  api->unit_is_pvp(boarHandle, &yes) == WXL_OK && yes == WXL_FALSE,
              "combat, skinnable and PvP are the unit flags' bits");
        Check(api->unit_is_tapped(boarHandle, &yes) == WXL_OK && yes == WXL_TRUE &&
                  api->unit_is_lootable(boarHandle, &yes) == WXL_OK && yes == WXL_TRUE &&
                  api->unit_is_tapped_by_player(boarHandle, &yes) == WXL_OK && yes == WXL_FALSE,
              "tapped and lootable are the dynamic flags' bits");
        uint32_t state = 99;
        Check(api->unit_stand_state(boarHandle, &state) == WXL_OK && state == WXL_STAND_SITTING &&
                  api->unit_is_sitting(boarHandle, &yes) == WXL_OK && yes == WXL_TRUE,
              "the stand state is UNIT_FIELD_BYTES_1's first byte");
        Check(api->unit_is_stealthed(boarHandle, &yes) == WXL_OK && yes == WXL_TRUE,
              "stealth is its third byte's creep flag");
        Check(api->unit_is_falling(boarHandle, &yes) == WXL_OK && yes == WXL_TRUE &&
                  api->unit_is_flying(boarHandle, &yes) == WXL_OK && yes == WXL_FALSE,
              "falling and flying are the movement flags'");
        Check(api->unit_has_aura(boarHandle, 116, &yes) == WXL_OK && yes == WXL_TRUE &&
                  api->unit_has_aura(boarHandle, 117, &yes) == WXL_OK && yes == WXL_FALSE,
              "HasAura looks through the auras the binding reads");

        // A struct answer: as much as the caller knows of it.
        wxl_aura aura{};
        aura.struct_size = sizeof aura;
        Check(api->unit_aura(boarHandle, 0, &aura) == WXL_OK && aura.spell_id == 116 && aura.stacks == 2 &&
                  aura.caster == 0x100 && aura.struct_size == sizeof aura,
              "an aura arrives whole");
        wxl_aura older{};
        older.struct_size = offsetof(wxl_aura, caster);
        older.stacks      = 77;
        Check(api->unit_aura(boarHandle, 0, &older) == WXL_OK && older.spell_id == 116 && older.stacks == 77 &&
                  older.struct_size == offsetof(wxl_aura, caster),
              "a caller with an older, shorter struct gets no more than it knows");
        Check(api->unit_aura(boarHandle, 1, &aura) == WXL_E_INVALID, "a slot past the auras is invalid");

        // The active player's own fields, from its whole block.
        player.block.assign(0x52E, 0u);
        player.block[0x27A] = 1234; // PLAYER_XP
        player.block[0x27B] = 5000; // PLAYER_NEXT_LEVEL_XP
        player.block[0x491] = 600;  // PLAYER_REST_STATE_EXPERIENCE
        player.block[0x4FD] = 75;   // PLAYER_FIELD_HONOR_CURRENCY
        player.block[0x4FE] = 12;   // PLAYER_FIELD_ARENA_CURRENCY
        player.block[0x27C] = 0x00010000u | 129; // skill 129 (first aid), step 1
        player.block[0x27D] = (225u << 16) | 150; // 150 of 225
        player.block[0x27E] = 0xFFFF0005u;      // +5 temporary, -1 permanent
        player.block[0x144 + 2 * 15] = 0x200;   // the main hand slot holds the boar's guid, as a test
        player.block[0x172 + 2 * 3]  = 0x400;   // the fourth backpack slot holds the bag
        uint32_t value = 0;
        Check(api->player_xp(playerHandle, &value) == WXL_OK && value == 1234 &&
                  api->player_next_level_xp(playerHandle, &value) == WXL_OK && value == 5000 &&
                  api->player_rested_xp(playerHandle, &value) == WXL_OK && value == 600 &&
                  api->player_honor_points(playerHandle, &value) == WXL_OK && value == 75 &&
                  api->player_arena_points(playerHandle, &value) == WXL_OK && value == 12,
              "experience, rest and points are the player's local fields");
        wxl_skill skill{};
        skill.struct_size = sizeof skill;
        Check(api->player_skill(playerHandle, 0, &skill) == WXL_OK && skill.id == 129 && skill.step == 1 &&
                  skill.value == 150 && skill.max == 225 && skill.temporary_bonus == 5 && skill.permanent_bonus == -1,
              "a skill line is its three dwords' halves");
        Check(api->player_skill_of(playerHandle, 129, &skill) == WXL_OK && skill.value == 150 &&
                  api->player_skill_of(playerHandle, 185, &skill) == WXL_E_INVALID,
              "SkillOf finds a skill by id and refuses one the player lacks");
        wxl_handle item = WXL_HANDLE_NONE;
        Check(api->player_inventory_item(playerHandle, WXL_INVENTORY_MAIN_HAND, &item) == WXL_OK && item == boarHandle &&
                  api->player_inventory_item(playerHandle, WXL_INVENTORY_HEAD, &item) == WXL_OK && item == WXL_HANDLE_NONE &&
                  api->player_inventory_item(playerHandle, 23, &item) == WXL_E_INVALID,
              "an inventory slot answers what it holds, none when empty, invalid past the bags");
        Check(api->player_backpack_item(playerHandle, 3, &item) == WXL_OK && item == bagHandle,
              "a backpack slot too");

        // Another player: the client keeps none of that for it.
        FakeObject other;
        other.guid = 0x110;
        other.type = WXL_OBJECT_PLAYER;
        other.block.assign(0x52E, 7u);
        const wxl_handle otherHandle = service.Track(&other);
        Check(api->player_xp(otherHandle, &value) == WXL_E_INVALID &&
                  api->player_skill(otherHandle, 0, &skill) == WXL_E_INVALID,
              "another player's own fields are refused as invalid");
        other.playerFlags = 0x02 | 0x10; // AFK, ghost
        Check(api->player_is_afk(otherHandle, &yes) == WXL_OK && yes == WXL_TRUE &&
                  api->player_is_ghost(otherHandle, &yes) == WXL_OK && yes == WXL_TRUE &&
                  api->player_is_dnd(otherHandle, &yes) == WXL_OK && yes == WXL_FALSE,
              "but its flags read on any player");
        service.Untrack(&other);

        // Items: the enchantments from the block, the cache's reads answer failed until cached.
        bag.block.assign(0x48, 0u);
        bag.block[0x16 + 3 * 2] = 3879; // the first socket's enchantment
        Check(api->item_enchant(bagHandle, 2, &value) == WXL_OK && value == 3879 &&
                  api->item_enchant(bagHandle, 12, &value) == WXL_E_INVALID,
              "an enchantment slot is ITEM_FIELD_ENCHANTMENT_n_1");
        Check(api->item_quality(bagHandle, &value) == WXL_E_FAILED, "an item the cache lacks answers failed");
        bag.cached  = true;
        bag.quality = WXL_QUALITY_EPIC;
        Check(api->item_quality(bagHandle, &value) == WXL_OK && value == WXL_QUALITY_EPIC, "and its value once cached");
        Check(api->item_quality(boarHandle, &value) == WXL_E_INVALID, "a unit has no item quality");

        boar.guid = 0x401; // the bag's second slot names it
        binding.held.push_back(&boar);
        Check(api->container_slot_item(bagHandle, 1, &item) == WXL_OK &&
                  api->container_slot_item(bagHandle, 0, &item) == WXL_OK && item == WXL_HANDLE_NONE &&
                  api->container_slot_item(bagHandle, 2, &item) == WXL_E_INVALID,
              "a bag's slot answers its item, none when empty, invalid past its slots");
        binding.held.clear();
        boar.guid = 0x200;
    }

    if (g_failures == 0)
    {
        std::printf("\nall checks passed\n");
        return 0;
    }

    std::printf("\n%d check(s) failed\n", g_failures);
    return 1;
}
