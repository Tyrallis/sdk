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

    /// Every id the table declares, in table order.
    constexpr uint32_t kEventIds[] = {
#define WXL_EVENT(family, name, id, args) id,
#include "wxl/events.def"
#undef WXL_EVENT
    };

    constexpr size_t kEventRows = sizeof(kEventIds) / sizeof(kEventIds[0]);

    /// Ids are the ABI: a duplicate silently routes one event's args to another's handler.
    bool IdsAreUnique()
    {
        for (size_t i = 0; i < kEventRows; ++i)
            for (size_t j = i + 1; j < kEventRows; ++j)
                if (kEventIds[i] == kEventIds[j]) return false;
        return true;
    }

    /// No gaps: a hole means an id was removed, which the table forbids.
    bool IdsAreDense()
    {
        for (uint32_t id = 0; id < kEventRows; ++id)
        {
            bool found = false;
            for (size_t i = 0; i < kEventRows && !found; ++i) found = kEventIds[i] == id;
            if (!found) return false;
        }
        return true;
    }

    constexpr uint32_t CountFamilyWorld()
    {
        uint32_t n = 0;
#define WXL_EVENT_World(name, id, args) ++n;
#include "wxl/events.def"
#undef WXL_EVENT_World
        return n;
    }

    constexpr uint32_t CountFamilyRender()
    {
        uint32_t n = 0;
#define WXL_EVENT_Render(name, id, args) ++n;
#include "wxl/events.def"
#undef WXL_EVENT_Render
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

    struct CountingNetScript final : wxl::NetScript
    {
        int      received   = 0;
        uint32_t lastOpcode = 0;

        void OnMessageReceived(const wxl_message_args& args) override
        {
            ++received;
            lastOpcode = args.opcode;
            if (args.suppressed && args.opcode == 0xF001) *args.suppressed = WXL_TRUE;
        }
    };

    struct CountingLuaScript final : wxl::LuaScript
    {
        int      ready       = 0;
        int      closing     = 0;
        uint32_t lastContext = 0;

        void OnScriptVmReady(const wxl_script_vm_args& args) override
        {
            ++ready;
            lastContext = args.context;
        }

        void OnScriptVmClosing(const wxl_script_vm_args&) override { ++closing; }
    };
} // namespace

int main()
{
    wxltest::FakeHost host;
    wxl::detail::HostRef() = host.Table();

    Section("event table");
    {
        Check(WXL_EV_COUNT == kEventRows, "the enum holds exactly the rows the table declares");
        Check(WXL_EV_Update == 0, "the first id is 0");
        Check(IdsAreUnique(), "no id is used twice");
        Check(IdsAreDense(), "the ids run 0..n-1 with no hole");
        Check(CountFamilyWorld() + CountFamilyRender() < WXL_EV_COUNT,
              "no family holds the whole table");
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
    CountingNetScript    net;
    CountingLuaScript    lua;
    {
        const size_t before = host.LiveSubscriptions();
        wxl::ScriptMgr::Add(&world);
        const size_t afterWorld = host.LiveSubscriptions();
        Check(afterWorld - before == CountFamilyWorld(),
              "a family subscribes its own events, and only those");

        wxl::ScriptMgr::Add(&secondWorld);
        Check(host.LiveSubscriptions() == afterWorld,
              "a second script of a family costs no further subscription");

        wxl::ScriptMgr::Add(&render);
        wxl::ScriptMgr::Add(&net);
        wxl::ScriptMgr::Add(&lua);
        Check(host.LiveSubscriptions() > afterWorld, "other families subscribe on their own");

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

        wxl_bool                 suppressed = WXL_FALSE;
        const uint8_t            body[4]    = {1, 2, 3, 4};
        const wxl_message_args   message{0xF001, body, sizeof(body), &suppressed};
        host.Raise(WXL_EV_MessageReceived, &message);
        Check(net.received == 1 && net.lastOpcode == 0xF001, "the net family runs");
        Check(suppressed == WXL_TRUE, "a mod can take an opcode the client does not know");

        const wxl_script_vm_args vm{WXL_SCRIPT_CONTEXT_GAME, nullptr};
        host.Raise(WXL_EV_ScriptVmReady, &vm);
        host.Raise(WXL_EV_ScriptVmClosing, &vm);
        Check(lua.ready == 1 && lua.closing == 1, "the script VM family sees both ends");
        Check(lua.lastContext == WXL_SCRIPT_CONTEXT_GAME, "and which context it was");
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
