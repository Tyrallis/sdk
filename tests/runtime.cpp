// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The shared runtime scaffolding, exercised against a binding that answers from a table instead
// of from a client. What passes here is what both runtimes get for free.

#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>

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

    /// A world of plain structs, standing in for a client.
    struct FakeObject
    {
        wxl_guid        guid = 0;
        wxl_object_type type = WXL_OBJECT_UNKNOWN;
        wxl_vec3        position{};
        float           facing = 0.0f;
        std::string     name;
        uint32_t        health    = 0;
        uint32_t        maxHealth = 0;
        uint32_t        level     = 0;
        FakeObject*     target    = nullptr;
    };

    class FakeBinding final : public wxl::runtime::ObjectBinding
    {
    public:
        FakeObject* localPlayer = nullptr;

        wxl_guid Guid(void* object) const override { return As(object)->guid; }
        wxl_object_type Type(void* object) const override { return As(object)->type; }

        bool Position(void* object, wxl_vec3& out) const override
        {
            out = As(object)->position;
            return true;
        }

        bool Facing(void* object, float& out) const override
        {
            out = As(object)->facing;
            return true;
        }

        bool Name(void* object, const char*& out, uint32_t& size) const override
        {
            const std::string& name = As(object)->name;
            out                     = name.c_str();
            size                    = static_cast<uint32_t>(name.size());
            return true;
        }

        bool UnitHealth(void* object, uint32_t& out) const override
        {
            out = As(object)->health;
            return true;
        }

        bool UnitMaxHealth(void* object, uint32_t& out) const override
        {
            out = As(object)->maxHealth;
            return true;
        }

        bool UnitLevel(void* object, uint32_t& out) const override
        {
            out = As(object)->level;
            return true;
        }

        void* UnitTarget(void* object) const override { return As(object)->target; }

        void* LocalPlayer() const override { return localPlayer; }

    private:
        static FakeObject* As(void* object) { return static_cast<FakeObject*>(object); }
    };
} // namespace

int main()
{
    FakeObject player{0x100, WXL_OBJECT_PLAYER, {10.0f, 20.0f, 30.0f}, 1.5f, "Thorgrim",
                      4200,  5000,              80,                   nullptr};
    FakeObject boar{0x200, WXL_OBJECT_UNIT, {11.0f, 21.0f, 31.0f}, 0.0f, "Boar", 90, 120, 3,
                    nullptr};
    FakeObject chest{0x300, WXL_OBJECT_GAMEOBJECT, {12.0f, 22.0f, 32.0f}, 0.0f, "Chest", 0, 0, 0,
                     nullptr};

    player.target = &boar;

    FakeBinding binding;
    binding.localPlayer = &player;

    wxl::runtime::ObjectService service(binding);
    const wxl_object_api*       api = service.Api();

    Section("tracking");
    const wxl_handle playerHandle = service.Track(&player);
    const wxl_handle boarHandle   = service.Track(&boar);
    const wxl_handle chestHandle  = service.Track(&chest);
    {
        Check(api->struct_size == sizeof(wxl_object_api), "the table reports its own size");
        Check(playerHandle != WXL_HANDLE_NONE, "tracking mints a handle");
        Check(service.Track(&player) == playerHandle, "tracking the same object answers the same");
        Check(service.Handles().Size() == 3, "three objects are live");
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
              "a target resolves to a handle");
    }

    Section("enumeration");
    {
        Check(api->enumerate(WXL_OBJECT_UNKNOWN, nullptr, 0) == 3, "counting without a buffer");
        Check(api->enumerate(WXL_OBJECT_UNIT, nullptr, 0) == 1, "counting one type");

        wxl_handle found[4]      = {};
        const uint32_t unitCount = api->enumerate(WXL_OBJECT_UNIT, found, 4);
        Check(unitCount == 1 && found[0] == boarHandle, "and listing it");

        wxl_handle       one       = WXL_HANDLE_NONE;
        const uint32_t   overflow  = api->enumerate(WXL_OBJECT_UNKNOWN, &one, 1);
        Check(overflow == 3, "a short buffer still reports the real total");
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
        Check(service.Handles().Size() == 2, "and the table shrinks");

        // The retired slot is reused. The point of the generation is that the old handle must
        // not come back to life pointing at whatever took the slot.
        FakeObject wolf{0x400, WXL_OBJECT_UNIT, {1.0f, 2.0f, 3.0f}, 0.0f, "Wolf", 50, 50, 5,
                        nullptr};
        const wxl_handle wolfHandle = service.Track(&wolf);
        Check(wolfHandle != boarHandle, "a reused slot mints a different handle");
        Check(api->valid(boarHandle) == WXL_FALSE, "and the old handle stays dead");
        Check(api->valid(wolfHandle) == WXL_TRUE, "while the new one lives");

        uint32_t health = 0;
        Check(api->unit_health(boarHandle, &health) == WXL_E_NOT_FOUND,
              "reading through a stale handle finds nothing rather than the wrong unit");

        service.Clear();
        Check(api->valid(wolfHandle) == WXL_FALSE, "clearing retires everything");
        Check(api->local_player() == WXL_HANDLE_NONE, "including the local player");
    }

    if (g_failures == 0)
    {
        std::printf("\nall checks passed\n");
        return 0;
    }

    std::printf("\n%d check(s) failed\n", g_failures);
    return 1;
}
