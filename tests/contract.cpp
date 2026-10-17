// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The conformance checks a runtime has to pass. Run here against the fake host; a real runtime
// passes the same file by linking its own host in place of the fake one.

#include <cstdio>
#include <cstring>

#include "fake_host.hpp"
#include "wxl/objects.hpp"
#include "wxl/sdk.hpp"

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

    // --- the event table -------------------------------------------------------------------

    constexpr uint32_t CountFamilyWorld()
    {
        uint32_t n = 0;
#define WXL_EVENT_World(name, id, args) ++n;
#define WXL_EVENT_Render(name, id, args)
#define WXL_EVENT_Object(name, id, args)
#define WXL_EVENT_Asset(name, id, args)
#define WXL_EVENT(family, name, id, args) WXL_EVENT_##family(name, id, args)
#include "wxl/events.def"
#undef WXL_EVENT
#undef WXL_EVENT_Asset
#undef WXL_EVENT_Object
#undef WXL_EVENT_Render
#undef WXL_EVENT_World
        return n;
    }

    constexpr uint32_t CountFamilyRender()
    {
        uint32_t n = 0;
#define WXL_EVENT_World(name, id, args)
#define WXL_EVENT_Render(name, id, args) ++n;
#define WXL_EVENT_Object(name, id, args)
#define WXL_EVENT_Asset(name, id, args)
#define WXL_EVENT(family, name, id, args) WXL_EVENT_##family(name, id, args)
#include "wxl/events.def"
#undef WXL_EVENT
#undef WXL_EVENT_Asset
#undef WXL_EVENT_Object
#undef WXL_EVENT_Render
#undef WXL_EVENT_World
        return n;
    }

    // --- scripts under test ----------------------------------------------------------------

    struct CountingWorldScript final : wxl::WorldScript
    {
        int   updates     = 0;
        int   worldEnters = 0;
        float lastDelta   = 0.0f;
        uint32_t lastMap  = 0;

        void OnUpdate(const wxl_update_args& args) override
        {
            ++updates;
            lastDelta = args.delta;
        }

        void OnWorldEnter(const wxl_world_args& args) override
        {
            ++worldEnters;
            lastMap = args.map_id;
        }

        void OnInput(const wxl_input_args& args) override
        {
            if (args.handled && *args.handled == WXL_FALSE && args.message == 0x100)
                *args.handled = WXL_TRUE;
        }
    };

    struct CountingRenderScript final : wxl::RenderScript
    {
        int frames = 0;
        void OnFrameBegin(const wxl_frame_args&) override { ++frames; }
    };

    struct SecondWorldScript final : wxl::WorldScript
    {
        int updates = 0;
        void OnUpdate(const wxl_update_args&) override { ++updates; }
    };
} // namespace

int main()
{
    wxltest::FakeHost host;
    wxl::detail::HostRef() = host.Table();

    Section("event table");
    {
        Check(WXL_EV_COUNT == 23, "the table holds the 23 events it declares");
        Check(WXL_EV_Update == 0, "the first id is 0");
        Check(CountFamilyWorld() + CountFamilyRender() < WXL_EV_COUNT,
              "no family holds the whole table");
        Check(static_cast<uint32_t>(WXL_EV_MapObjectLoad) == WXL_EV_COUNT - 1,
              "the last declared event is the last id");
    }

    Section("version negotiation");
    {
        const wxl_runtime_info* info = host.Table()->runtime();
        Check(info != nullptr, "the runtime describes itself");
        Check(info->abi_major == WXL_ABI_VERSION_MAJOR, "majors agree");
        Check(info->abi_minor >= WXL_ABI_VERSION_MINOR,
              "the runtime is not older than what this mod was built against");
        Check(info->struct_size == sizeof(wxl_runtime_info), "struct_size is filled in");
    }

    Section("script dispatch");
    CountingWorldScript  world;
    CountingRenderScript render;
    SecondWorldScript    secondWorld;
    {
        wxl::ScriptMgr::Add(&world);
        wxl::ScriptMgr::Add(&render);
        wxl::ScriptMgr::Add(&secondWorld);

        Check(host.LiveSubscriptions() == CountFamilyWorld() + CountFamilyRender(),
              "a family subscribes once, for its own events only");

        const wxl_update_args tick{0.016f};
        host.Raise(WXL_EV_Update, &tick);

        Check(world.updates == 1, "an override runs");
        Check(secondWorld.updates == 1, "every script of the family runs");
        Check(world.lastDelta == 0.016f, "the args arrive intact");
        Check(render.frames == 0, "an event does not reach another family");

        const wxl_frame_args frame{1, 0.016f};
        host.Raise(WXL_EV_FrameBegin, &frame);
        Check(render.frames == 1, "the render family runs on its own events");
        Check(world.updates == 1, "and does not disturb the first");

        const wxl_world_args entered{571, {100.0f, 200.0f, 300.0f}};
        host.Raise(WXL_EV_WorldEnter, &entered);
        Check(world.worldEnters == 1 && world.lastMap == 571, "a second event of a family runs");
    }

    Section("cancellation");
    {
        wxl_bool             handled = WXL_FALSE;
        const wxl_input_args input{0x100, 0, 0, &handled};
        host.Raise(WXL_EV_Input, &input);
        Check(handled == WXL_TRUE, "a handler can swallow an input");

        wxl_bool             untouched = WXL_FALSE;
        const wxl_input_args other{0x200, 0, 0, &untouched};
        host.Raise(WXL_EV_Input, &other);
        Check(untouched == WXL_FALSE, "and leaves the rest alone");
    }

    Section("services");
    {
        wxl::Service<wxl_object_api> objects(WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION);
        Check(!objects, "an unpublished service resolves to nothing");
        Check(wxl::HasObjects() == false, "and the convenience check agrees");

        static wxl_object_api api{};
        api.struct_size = sizeof(api);
        api.valid        = [](wxl_handle h) -> wxl_bool { return h == 42 ? WXL_TRUE : WXL_FALSE; };
        api.local_player = []() -> wxl_handle { return 42; };
        api.type         = [](wxl_handle) -> uint32_t { return WXL_OBJECT_PLAYER; };

        Check(host.Table()->publish_service(WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION,
                                            &api) == WXL_OK,
              "a service can be published");
        Check(host.Table()->has_service(WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION) == WXL_TRUE,
              "and is then visible");
        Check(host.Table()->has_service(WXL_OBJECT_SERVICE, 99) == WXL_FALSE,
              "another version does not match");
        Check(static_cast<bool>(objects), "a handle resolved late picks it up");

        const wxl::Player me = wxl::Player::Local();
        Check(me.IsValid(), "the local player resolves through the service");
        Check(me.Type() == WXL_OBJECT_PLAYER, "and reports its type");
        Check(wxl::Object(WXL_HANDLE_NONE).IsValid() == false, "a null handle is never valid");
        Check(wxl::Object(7).IsValid() == false, "a stale handle answers instead of misreading");
    }

    Section("a runtime that cannot raise an event");
    {
        Check(host.Table()->subscribe(WXL_EV_Update, nullptr, nullptr, WXL_PRIORITY_DEFAULT) ==
                  WXL_SUBSCRIPTION_NONE,
              "a null handler is refused");
    }

    Section("log");
    {
        wxl::Log(WXL_LOG_INFO, "contract", "hello");
        Check(host.LogLines().size() == 1 && host.LogLines().back() == "[contract] hello",
              "a log line crosses with its tag and its length");
    }

    if (g_failures == 0)
    {
        std::printf("\nall checks passed\n");
        return 0;
    }

    std::printf("\n%d check(s) failed\n", g_failures);
    return 1;
}
