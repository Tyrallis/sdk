// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The conformance checks a runtime has to pass. Run here against the fake host; a real runtime
// passes the same file by linking its own host in place of the fake one.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "fake_host.hpp"
#include "fake_services.hpp"
#include "wxl/services.hpp"

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
        int      updates     = 0;
        int      worldEnters = 0;
        float    lastDelta   = 0.0f;
        uint32_t lastMap     = 0;

        void OnUpdate(float delta) override
        {
            ++updates;
            lastDelta = delta;
        }

        void OnWorldEnter(uint32_t map_id, wxl_vec3) override
        {
            ++worldEnters;
            lastMap = map_id;
        }

        void OnInput(uint32_t message, uint64_t, int64_t, wxl::Answer handled) override
        {
            if (!handled.Value() && message == 0x100) handled.Set();
        }
    };

    struct CountingRenderScript final : wxl::RenderScript
    {
        int frames = 0;
        void OnFrameBegin(uint64_t, float) override { ++frames; }
    };

    struct SecondWorldScript final : wxl::WorldScript
    {
        int updates = 0;
        void OnUpdate(float) override { ++updates; }
    };

    struct CountingNetScript final : wxl::NetScript
    {
        int      received   = 0;
        uint32_t lastOpcode = 0;

        void OnMessageReceived(uint32_t opcode, wxl::Bytes, wxl::Answer suppressed) override
        {
            ++received;
            lastOpcode = opcode;
            if (opcode == 0xF001) suppressed.Set();
        }
    };

    // What a script handles. Each is a way an override could hide from the check that decides
    // what to subscribe; each must still be subscribed, and must still run.
    struct NothingWorldScript final : wxl::WorldScript
    {
    };

    struct LeaveWorldScript final : wxl::WorldScript
    {
        int leaves = 0;
        void OnWorldLeave(uint32_t, wxl_vec3) override { ++leaves; }
    };

    struct PrivateWorldScript final : wxl::WorldScript
    {
        int clicks = 0;

    private:
        void OnWorldClick(wxl_vec3, wxl_handle, uint32_t) override { ++clicks; }
    };

    struct ProtectedWorldScript final : wxl::WorldScript
    {
        int sounds = 0;

    protected:
        void OnSoundPlay(uint32_t, std::string_view, wxl::Answer) override { ++sounds; }
    };

    struct MidWorldScript : wxl::WorldScript
    {
        int updates = 0;
        void OnUpdate(float) override { ++updates; }
    };

    struct LeafWorldScript final : MidWorldScript
    {
    };

    /// Changes its own family from inside its handler.
    struct RearrangingWorldScript final : wxl::WorldScript
    {
        int                updates    = 0;
        SecondWorldScript* add        = nullptr;
        SecondWorldScript* remove     = nullptr;
        bool               removeSelf = false;

        void OnUpdate(float) override
        {
            ++updates;
            if (add) wxl::ScriptMgr::Add(std::exchange(add, nullptr));
            if (remove) wxl::ScriptMgr::Remove(std::exchange(remove, nullptr));
            if (removeSelf) wxl::ScriptMgr::Remove(this);
        }
    };

    struct CountingLuaScript final : wxl::LuaScript
    {
        int      ready       = 0;
        int      closing     = 0;
        uint32_t lastContext = 0;

        void OnScriptVmReady(uint32_t context, void*) override
        {
            ++ready;
            lastContext = context;
        }

        void OnScriptVmClosing(uint32_t, void*) override { ++closing; }
    };

    /// A typed handler for the free-standing Subscribe.
    struct TickHandler
    {
        int   ticks = 0;
        float last  = 0.0f;
        void  operator()(float delta)
        {
            ++ticks;
            last = delta;
        }
    };

    /// Reads every kind of parameter a handler can be given, and answers through the writable ones.
    struct ParameterScript final : wxl::AssetScript
    {
        std::string path;
        std::string redirectSeen;
        uint32_t    capacity = 0;
        bool        wrote    = false;
        std::string body;

        void OnFileOpen(std::string_view asked, wxl::Buffer redirect) override
        {
            path         = std::string(asked);
            redirectSeen = std::string(redirect.Value());
            capacity     = redirect.Capacity();
            wrote        = redirect.Write("Textures\\swapped.blp");
        }

        void OnMapObjectLoad(std::string_view, wxl::Bytes data) override
        {
            body.assign(data.View());
            for (uint8_t byte : data) body += static_cast<char>('0' + byte % 10);
        }
    };

    /// Sees what an earlier handler answered.
    struct LateNetScript final : wxl::NetScript
    {
        bool sawAnswer = false;
        void OnMessageReceived(uint32_t, wxl::Bytes, wxl::Answer suppressed) override
        {
            sawAnswer = suppressed.Value();
        }
    };

    // --- a mod under test -------------------------------------------------------------------

    struct LifecycleMod final : wxl::Mod
    {
        static inline std::vector<std::string> trace;
        static inline CountingWorldScript*     script = nullptr;

        wxl_result OnLoad() override
        {
            trace.push_back("load");
            if (script) wxl::ScriptMgr::Add(script);
            return WXL_OK;
        }
        void OnEarly() override { trace.push_back("early"); }
        void OnStorageReady() override { trace.push_back("storage"); }
        void OnGraphicsReady() override { trace.push_back("graphics"); }
        void OnScriptReady() override { trace.push_back("script"); }
        void OnReady() override { trace.push_back("ready"); }
        void OnShutdown() override { trace.push_back("shutdown"); }
        void OnStage(wxl_stage stage) override { trace.push_back("stage" + std::to_string(stage)); }
        void OnUnload() override { trace.push_back("unload"); }
    };

    // --- script-side functions --------------------------------------------------------------

    void Greet(wxl::ScriptCall& call)
    {
        const std::optional<std::string_view> name = call.String(0);
        if (!name)
        {
            call.Fail("Greet(name) wants a string");
            return;
        }
        call.PushString(*name);
        call.PushNumber(static_cast<double>(call.ArgCount()));
        call.PushBool(call.Bool(1).value_or(false));
    }

    struct Adder
    {
        double total = 0.0;
        void   operator()(wxl::ScriptCall& call)
        {
            total += call.Number(0).value_or(0.0);
            call.PushNumber(total);
        }
    };

    // --- panels and hooks -------------------------------------------------------------------

    int g_panelDraws = 0;
    void DrawPanel()
    {
        ++g_panelDraws;
        wxl::UiService().Text("hello");
    }

    using ThingFn = int(WXL_CALL*)(int);
    int WXL_CALL ThingDetour(int x) { return x + 1; }
} // namespace

// The entry points the WXL_MOD macro writes, exercised in-process.
WXL_MOD(LifecycleMod, "wxl.contract", "Contract", 1)

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
        Check(std::is_same_v<wxl::EventArgsOf<WXL_EV_Update>, wxl_update_args> &&
                  std::is_same_v<wxl::EventArgsOf<WXL_EV_MessageReceived>, wxl_message_args>,
              "EventArgs binds each id to its args struct");
    }

    Section("handler parameters");
    {
        Check(std::is_same_v<wxl::HandlerOf<WXL_EV_Update>, void(float)>,
              "a handler takes the members as parameters");
        Check(std::is_same_v<wxl::HandlerOf<WXL_EV_MessageReceived>,
                             void(uint32_t, wxl::Bytes, wxl::Answer)>,
              "a bytes member is a view, and the member holding its length is not a parameter");
        Check(std::is_same_v<wxl::HandlerOf<WXL_EV_FileOpen>, void(std::string_view, wxl::Buffer)>,
              "a str is a string_view, a buffer a view whose capacity is not a parameter");
        Check(std::is_same_v<wxl::HandlerOf<WXL_EV_WorldEnter>, void(uint32_t, wxl_vec3)> &&
                  std::is_same_v<wxl::HandlerOf<WXL_EV_UnitAuraChanged>,
                                 void(wxl_handle, uint32_t, bool)>,
              "values, vectors, handles and booleans keep their order");
        Check(std::is_same_v<wxl::HandlerOf<WXL_EV_ModelLoad>,
                             void(std::string_view, wxl::Bytes, const void**, uint32_t*)>,
              "a ptr member keeps its C type");

        uint8_t     raw[3] = {1, 2, 3};
        wxl::Bytes  bytes(raw, 3);
        wxl::Bytes  none(nullptr, 5);
        Check(bytes.size() == 3 && bytes[2] == 3 && !bytes.empty() && none.empty() &&
                  none.View().empty(),
              "Bytes is a range, and empty over a null pointer whatever the length says");

        wxl_bool    flag = WXL_FALSE;
        wxl::Answer answer(&flag);
        Check(!answer.Value() && answer.Set() && flag == WXL_TRUE && answer.Value(),
              "Answer reads the flag and sets it");
        Check(!wxl::Answer(nullptr).Set() && !wxl::Answer(nullptr).Value(),
              "an Answer without a flag answers nothing");

        char        storage[8] = {};
        wxl::Buffer buffer(storage, sizeof storage);
        Check(buffer.Value().empty() && buffer.Capacity() == 8, "a Buffer starts empty");
        Check(buffer.Write("1234567") && buffer.Value() == "1234567" && storage[7] == '\0',
              "a string that fits with its terminator is written");
        Check(!buffer.Write("12345678") && buffer.Value() == "1234567",
              "one that does not is refused and leaves the buffer as it was");
        Check(!wxl::Buffer(nullptr, 8).Write("") && wxl::Buffer(nullptr, 8).Capacity() == 0,
              "a Buffer without storage takes nothing");
    }

    Section("version negotiation");
    {
        const wxl_runtime_info* info = host.Table()->runtime();
        Check(info != nullptr, "the runtime describes itself");
        Check(info->abi_major == WXL_ABI_VERSION_MAJOR, "majors agree");
        Check(info->abi_minor == WXL_ABI_VERSION_MINOR, "and, under major 0, minors");
        Check(info->abi_feature >= WXL_ABI_VERSION_FEATURE,
              "the runtime is not older than what this mod was built against");
        Check(host.Table()->abi_major == info->abi_major && host.Table()->abi_minor == info->abi_minor &&
                  host.Table()->abi_feature == info->abi_feature,
              "the table and the runtime info say the same version");
        Check(info->struct_size == sizeof(wxl_runtime_info), "struct_size is filled in");
    }

    Section("subscriptions are owned");
    {
        const size_t before = host.LiveSubscriptions();
        const size_t calls  = host.UnsubscribeCalls();
        TickHandler  handler;
        {
            wxl::Subscription tick = wxl::Subscribe<WXL_EV_Update>(&handler);
            Check(static_cast<bool>(tick), "a typed subscribe takes");
            Check(host.LiveSubscriptions() == before + 1, "and is live");

            const wxl_update_args args{0.25f};
            host.Raise(WXL_EV_Update, &args);
            Check(handler.ticks == 1 && handler.last == 0.25f, "the handler sees typed args");

            wxl::Subscription moved = std::move(tick);
            Check(!tick && moved, "a move hands the token over");
            Check(host.LiveSubscriptions() == before + 1, "without releasing it");

            moved.Reset();
            moved.Reset();
            Check(host.LiveSubscriptions() == before, "Reset drops it");
            Check(host.UnsubscribeCalls() == calls + 1, "exactly once, however often Reset runs");
        }
        Check(host.UnsubscribeCalls() == calls + 1, "and the destructor adds nothing after a Reset");

        {
            wxl::Subscription scoped = wxl::Subscribe<WXL_EV_Update>(&handler);
            Check(host.LiveSubscriptions() == before + 1, "a scoped subscription is live inside");
        }
        Check(host.LiveSubscriptions() == before, "and gone at the end of its scope");
        Check(host.UnsubscribeCalls() == calls + 2, "released by the destructor, once");

        wxl_subscription raw = WXL_SUBSCRIPTION_NONE;
        {
            wxl::Subscription kept = wxl::Subscribe<WXL_EV_Update>(&handler);
            raw                    = kept.Release();
        }
        Check(raw != WXL_SUBSCRIPTION_NONE && host.LiveSubscriptions() == before + 1,
              "Release disowns: the subscription outlives the wrapper");
        host.Table()->unsubscribe(raw);

        wxl::Subscription      orphan      = wxl::Subscribe<WXL_EV_Update>(&handler);
        const wxl_subscription orphanToken = orphan.Get();
        wxl::detail::HostRef()             = nullptr;
        orphan.Reset();
        Check(host.UnsubscribeCalls() == calls + 3 && host.LiveSubscriptions() == before + 1,
              "a wrapper outliving the host does not call into it");
        wxl::detail::HostRef() = host.Table();
        host.Table()->unsubscribe(orphanToken);

        Check(!wxl::Subscribe<WXL_EV_Update>(static_cast<TickHandler*>(nullptr)),
              "a null handler yields an empty subscription");

        uint32_t seenOpcode = 0, seenSize = 0;
        auto     onMessage  = [&](uint32_t opcode, wxl::Bytes data, wxl::Answer suppressed) {
            seenOpcode = opcode;
            seenSize   = data.size();
            suppressed.Set();
        };
        wxl::Subscription message = wxl::Subscribe<WXL_EV_MessageReceived>(&onMessage);
        wxl_bool               kept    = WXL_FALSE;
        const uint8_t          body[2] = {7, 8};
        const wxl_message_args args{0x42, body, sizeof body, &kept};
        host.Raise(WXL_EV_MessageReceived, &args);
        Check(seenOpcode == 0x42 && seenSize == 2 && kept == WXL_TRUE,
              "a lambda takes the parameters and answers through them");
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
        // CountingWorldScript overrides Update, WorldEnter and Input: three of its family's six.
        Check(afterWorld - before == 3 && afterWorld - before < CountFamilyWorld(),
              "a script subscribes the events it overrides, not its whole family");

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

        wxl_bool               suppressed = WXL_FALSE;
        const uint8_t          body[4]    = {1, 2, 3, 4};
        const wxl_message_args message{0xF001, body, sizeof(body), &suppressed};
        host.Raise(WXL_EV_MessageReceived, &message);
        Check(net.received == 1 && net.lastOpcode == 0xF001, "the net family runs");
        Check(suppressed == WXL_TRUE, "a mod can take an opcode the client does not know");

        LateNetScript late;
        wxl::ScriptMgr::Add(&late);
        suppressed = WXL_FALSE;
        host.Raise(WXL_EV_MessageReceived, &message);
        Check(late.sawAnswer, "a later handler sees the answer an earlier one gave");
        wxl::ScriptMgr::Remove(&late);

        ParameterScript asset;
        wxl::ScriptMgr::Add(&asset);
        char                     redirect[32] = "Textures\\first.blp";
        const char*              asked        = "Textures\\asked.blp";
        const wxl_file_open_args open{wxl::Str(asked), redirect, sizeof redirect};
        host.Raise(WXL_EV_FileOpen, &open);
        Check(asset.path == asked && asset.redirectSeen == "Textures\\first.blp" &&
                  asset.capacity == sizeof redirect,
              "a str and a buffer arrive as views, with what an earlier handler wrote");
        Check(asset.wrote && std::strcmp(redirect, "Textures\\swapped.blp") == 0,
              "and a handler answers by writing into the buffer");
        const uint8_t             root[3] = {'a', 1, 12};
        const wxl_map_object_args mapObject{wxl::Str("x.wmo"), root, sizeof root};
        host.Raise(WXL_EV_MapObjectLoad, &mapObject);
        Check(asset.body == std::string("a\x01\x0c", 3) + "712", "bytes arrive with their length");
        wxl::ScriptMgr::Remove(&asset);

        const wxl_script_vm_args vm{WXL_SCRIPT_CONTEXT_GAME, nullptr};
        host.Raise(WXL_EV_ScriptVmReady, &vm);
        host.Raise(WXL_EV_ScriptVmClosing, &vm);
        Check(lua.ready == 1 && lua.closing == 1, "the script VM family sees both ends");
        Check(lua.lastContext == WXL_SCRIPT_CONTEXT_GAME, "and which context it was");

        wxl::ScriptMgr::Remove(&secondWorld);
        host.Raise(WXL_EV_Update, &tick);
        Check(world.updates == 2 && secondWorld.updates == 1, "a removed script no longer runs");

        wxl::ScriptMgr::Clear();
        Check(host.LiveSubscriptions() == before, "Clear releases every family subscription");
        host.Raise(WXL_EV_Update, &tick);
        Check(world.updates == 2, "and nothing runs afterwards");

        wxl::ScriptMgr::Add(&world);
        Check(host.LiveSubscriptions() == afterWorld, "a family subscribes again after Clear");
        wxl::ScriptMgr::Clear();
    }

    Section("what a script handles");
    {
        wxl::ScriptMgr::Clear();
        const size_t before = host.LiveSubscriptions();

        NothingWorldScript nothing;
        wxl::ScriptMgr::Add(&nothing);
        Check(host.LiveSubscriptions() == before, "a script that overrides nothing subscribes nothing");

        LeaveWorldScript leave;
        wxl::ScriptMgr::Add(&leave);
        Check(host.LiveSubscriptions() == before + 1, "one that overrides one event subscribes it");

        LeaveWorldScript secondLeave;
        wxl::ScriptMgr::Add(&secondLeave);
        Check(host.LiveSubscriptions() == before + 1,
              "a second script handling the same event subscribes nothing more");

        const wxl_world_args left{};
        host.Raise(WXL_EV_WorldLeave, &left);
        Check(leave.leaves == 1 && secondLeave.leaves == 1, "and both of them run");

        // The cases that would be a mod compiling and silently doing nothing, if the check that
        // decides what to subscribe could be fooled. Each is counted, and each is raised.
        PrivateWorldScript hidden;
        wxl::ScriptMgr::Add(&hidden);
        Check(host.LiveSubscriptions() == before + 2,
              "an override declared private is still found and subscribed");
        const wxl_world_click_args click{};
        host.Raise(WXL_EV_WorldClick, &click);
        Check(hidden.clicks == 1, "and it runs: a private override is not a silent one");

        ProtectedWorldScript guarded;
        wxl::ScriptMgr::Add(&guarded);
        Check(host.LiveSubscriptions() == before + 3, "so is one declared protected");
        const wxl_sound_args sound{};
        host.Raise(WXL_EV_SoundPlay, &sound);
        Check(guarded.sounds == 1, "and it runs too");

        LeafWorldScript leaf;
        wxl::ScriptMgr::Add(&leaf);
        Check(host.LiveSubscriptions() == before + 4,
              "an override made in a class between the script and its family is found");
        const wxl_update_args tick{0.016f};
        host.Raise(WXL_EV_Update, &tick);
        Check(leaf.updates == 1, "and runs on the script that only inherits it");

        wxl::ScriptMgr::Clear();
        Check(host.LiveSubscriptions() == before, "Clear releases what each of them subscribed");

        // A script handed over as its family hides what it overrides, so it costs the family.
        LeaveWorldScript erased;
        wxl::ScriptMgr::Add(static_cast<wxl::WorldScript*>(&erased));
        Check(host.LiveSubscriptions() == before + CountFamilyWorld(),
              "a script passed as its family subscribes every event of it, to be safe");
        host.Raise(WXL_EV_WorldLeave, &left);
        Check(erased.leaves == 1, "and its override still runs");

        wxl::ScriptMgr::Clear();
        Check(host.LiveSubscriptions() == before, "and Clear gives all of it back");
    }

    Section("a handler that changes its family");
    {
        wxl::ScriptMgr::Clear();
        const wxl_update_args tick{0.016f};

        RearrangingWorldScript first;
        SecondWorldScript      late;
        first.add        = &late;
        first.removeSelf = true;
        wxl::ScriptMgr::Add(&first);
        host.Raise(WXL_EV_Update, &tick);
        Check(first.updates == 1 && late.updates == 0,
              "a script added by a handler waits for the next event");
        host.Raise(WXL_EV_Update, &tick);
        Check(first.updates == 1 && late.updates == 1,
              "one that removed itself gets nothing more, and the one it added runs");
        wxl::ScriptMgr::Clear();

        RearrangingWorldScript remover;
        SecondWorldScript      victim;
        remover.remove = &victim;
        wxl::ScriptMgr::Add(&remover);
        wxl::ScriptMgr::Add(&victim);
        host.Raise(WXL_EV_Update, &tick);
        Check(remover.updates == 1 && victim.updates == 0,
              "a script removed by an earlier handler of the same event does not run in it");
        host.Raise(WXL_EV_Update, &tick);
        Check(remover.updates == 2 && victim.updates == 0, "nor afterwards");
        wxl::ScriptMgr::Clear();
    }

    Section("cancellation");
    {
        wxl::ScriptMgr::Add(&world);

        wxl_bool             handled = WXL_FALSE;
        const wxl_input_args input{0x100, 0, 0, &handled};
        host.Raise(WXL_EV_Input, &input);
        Check(handled == WXL_TRUE, "a handler can swallow an input");

        wxl_bool             untouched = WXL_FALSE;
        const wxl_input_args other{0x200, 0, 0, &untouched};
        host.Raise(WXL_EV_Input, &other);
        Check(untouched == WXL_FALSE, "and leaves the rest alone");

        wxl::ScriptMgr::Clear();
    }

    Section("the mod lifecycle");
    {
        const size_t before = host.LiveSubscriptions();
        LifecycleMod::script = &world;

        Check(wxl_mod_query()->struct_size == sizeof(wxl_mod_info) &&
                  wxl_mod_query()->abi_major == WXL_ABI_VERSION_MAJOR &&
                  wxl_mod_query()->abi_minor == WXL_ABI_VERSION_MINOR &&
                  wxl_mod_query()->abi_feature == WXL_ABI_VERSION_FEATURE &&
                  std::strcmp(wxl_mod_query()->id, "wxl.contract") == 0,
              "WXL_MOD reports the mod");
        Check(wxl_mod_load(host.Table()) == WXL_OK, "load runs OnLoad");
        Check(host.LiveSubscriptions() == before + 3,
              "a script added in OnLoad subscribes what it overrides");

        for (int stage = WXL_STAGE_EARLY; stage <= WXL_STAGE_SHUTDOWN; ++stage)
            wxl_mod_stage(static_cast<wxl_stage>(stage));
        wxl_mod_unload();

        const std::vector<std::string> expected = {
            "load",    "early",  "stage0", "storage",  "stage1", "graphics", "stage2", "script",
            "stage3",  "ready",  "stage4", "shutdown", "stage5", "unload"};
        Check(LifecycleMod::trace == expected,
              "each stage reaches its named virtual, then OnStage; unload comes last");
        Check(host.LiveSubscriptions() == before,
              "unload releases what ScriptMgr subscribed, so no handler outlives the mod");

        LifecycleMod::trace.clear();
        wxl_mod_stage(static_cast<wxl_stage>(99));
        Check(LifecycleMod::trace.empty(), "a stage after unload reaches nothing");
    }

    Section("a service that is not there");
    {
        Check(!wxl::ObjectService::Available() && !wxl::HasObjects(),
              "an unpublished service resolves to nothing");

        const wxl::Unit unit(wxltest::FakeObjects::kPlayer);
        Check(!unit.IsValid() && !unit, "no object is valid without the service");

        const wxl::Expected<uint32_t> health = unit.Health();
        Check(!health && health.IsUnsupported() && health.Error() == WXL_E_UNSUPPORTED,
              "a read without the service is unsupported, not zero");
        Check(health.ValueOr(7) == 7, "ValueOr answers the fallback");
        Check(!health.ToOptional().has_value(), "and the optional view is empty");

        Check(!wxl::Player::Local().HasHandle(), "no local player without the service");
        Check(wxl::ObjectService().InView<wxl::Unit>().empty(), "and nothing is in view");

        Check(wxl::ScriptService().RegisterFunction<&Greet>("Greet") == WXL_E_UNSUPPORTED,
              "a facade call without its service answers unsupported");
        Check(!wxl::StorageService().Open("x"), "a File without the service is empty");
        Check(!wxl::PatchService().Apply("x", 0x1000, "a", "b", 1), "so is a Patch");
        Check(!wxl::HookService().Attach("Thing", &ThingDetour, static_cast<ThingFn*>(nullptr)),
              "so is a Hook");
        Check(wxl::NetService().Claim(0x600).IsUnsupported(), "and a claim says why");
        Check(wxl::UiService().AddPanel<&DrawPanel>("P").IsUnsupported(), "as does a panel");
    }

    Section("services");
    {
        wxl::Service<wxl_object_api> objects(WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION);
        Check(!objects, "a raw handle to an unpublished service is empty");

        Check(host.Table()->publish_service(WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION,
                                            wxltest::FakeObjects::Table()) == WXL_OK,
              "a service can be published");
        Check(host.Table()->has_service(WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION) == WXL_TRUE,
              "and is then visible");
        Check(host.Table()->has_service(WXL_OBJECT_SERVICE, 99) == WXL_FALSE,
              "another version does not match");
        Check(static_cast<bool>(objects), "a raw handle resolved late picks it up");
        Check(wxl::ObjectService::Available() && wxl::HasObjects(), "so does the facade");
        Check(wxl::ObjectService::Raw() == wxltest::FakeObjects::Table(),
              "the facade reaches the table that was published");
        Check(std::strcmp(wxl::ObjectService::kName, WXL_OBJECT_SERVICE) == 0 &&
                  wxl::ObjectService::kVersion == WXL_OBJECT_SERVICE_VERSION,
              "the name and the version are sealed in the facade");

        host.Table()->publish_service(WXL_SCRIPT_SERVICE, WXL_SCRIPT_SERVICE_VERSION, wxltest::FakeScript::Table());
        host.Table()->publish_service(WXL_STORAGE_SERVICE, WXL_STORAGE_SERVICE_VERSION, wxltest::FakeStorage::Table());
        host.Table()->publish_service(WXL_NET_SERVICE, WXL_NET_SERVICE_VERSION, wxltest::FakeNet::Table());
        host.Table()->publish_service(WXL_UI_SERVICE, WXL_UI_SERVICE_VERSION, wxltest::FakeUi::Table());
        host.Table()->publish_service(WXL_HOOK_SERVICE, WXL_HOOK_SERVICE_VERSION, wxltest::FakeHooks::Table());
        host.Table()->publish_service(WXL_HOOK_RAW_SERVICE, WXL_HOOK_RAW_SERVICE_VERSION, wxltest::FakeHooks::RawTable());
        host.Table()->publish_service(WXL_PATCH_SERVICE, WXL_PATCH_SERVICE_VERSION, wxltest::FakePatch::Table());
    }

    Section("reads that can fail");
    {
        using wxltest::FakeObjects;

        const wxl::Player me = wxl::Player::Local();
        Check(me.IsValid() && me.Handle() == FakeObjects::kPlayer, "the local player resolves");
        Check(me.Type() && *me.Type() == WXL_OBJECT_PLAYER, "and reports its type");

        const wxl::Expected<uint32_t> dead = me.Health();
        Check(dead && *dead == 0, "a dead unit reads as a value of zero");
        Check(me.MaxHealth().ValueOr(0) == 5000 && me.Level().ValueOr(0) == 80, "other reads arrive");
        Check(me.HealthFraction() && *me.HealthFraction() == 0.0f, "a fraction of zero is a value");

        const wxl::Unit stale(FakeObjects::kStale);
        Check(!stale.IsValid(), "a stale handle is not valid");
        const wxl::Expected<uint32_t> gone = stale.Health();
        Check(!gone && gone.IsNotFound(), "a read through a stale handle is absent, not zero");
        Check(gone.Error() == WXL_E_NOT_FOUND, "and says it is not found");
        Check(stale.Name().IsNotFound() && stale.Position().IsNotFound() &&
                  stale.Guid().IsNotFound() && stale.Type().IsNotFound(),
              "every read on a stale handle says the same");

        const wxl::Unit none;
        Check(none.Health().IsNotFound() && !none.HasHandle(),
              "a null handle reads as not found without asking the runtime");

        const wxl::Unit chest(FakeObjects::kChest);
        Check(chest.IsValid(), "a game object is a valid object");
        Check(chest.Health().IsInvalid(), "but asking it for health is a mistake, not a miss");
        Check(chest.Guid() && *chest.Guid() == WXL_GUID_NONE,
              "a live object the server does not own has a guid of none, as a value");

        Check(me.Name() && *me.Name() == "Thorgrim", "a name is copied out");
        Check(me.Position() && me.Position()->x == 42.0f, "a position reads");

        const wxl::Expected<wxl::Unit> target = me.Target();
        Check(target && target->Handle() == FakeObjects::kBoar, "a target reads as a unit");
        const wxl::Unit boar(FakeObjects::kBoar);
        Check(boar.Target() && !boar.Target()->HasHandle(), "a unit with no target reads as a null unit");
        Check(boar.Health().ValueOr(0) == 90, "a live unit reads its health");
    }

    Section("objects are values");
    {
        using wxltest::FakeObjects;

        const wxl::Unit a(FakeObjects::kBoar), b(FakeObjects::kBoar), c(FakeObjects::kPlayer);
        Check(a == b && a != c && !(a < b) && (c < a) == (c.Handle() < a.Handle()),
              "objects compare by handle");
        Check(wxl::Object(a) == a, "a unit equals itself as an object");

        std::set<wxl::Unit> ordered{a, b, c};
        Check(ordered.size() == 2, "a std::set holds each object once");

        std::unordered_set<wxl::Unit> hashed{a, b, c};
        Check(hashed.size() == 2 && hashed.count(wxl::Unit(FakeObjects::kPlayer)) == 1,
              "and so does an unordered_set");

        Check(static_cast<bool>(a) && !wxl::Unit(FakeObjects::kStale) && !wxl::Unit(),
              "operator bool is IsValid");

        const wxl::Object chest(FakeObjects::kChest);
        Check(!chest.As<wxl::Unit>().has_value(), "a chest does not view as a unit");
        Check(chest.As<wxl::GameObject>().has_value(), "but does as a game object");
        Check(wxl::Object(FakeObjects::kPlayer).As<wxl::Unit>().has_value() &&
                  wxl::Object(FakeObjects::kPlayer).As<wxl::Player>().has_value() &&
                  !wxl::Object(FakeObjects::kBoar).As<wxl::Player>().has_value(),
              "a player is a unit and a player; a boar is not a player");
        Check(!wxl::Object(FakeObjects::kStale).As<wxl::Unit>().has_value(),
              "a stale object views as nothing");

        const wxl::ObjectService objects;
        Check(objects.FromGuid(0x200) == wxl::Object(FakeObjects::kBoar), "a guid finds its object");
        Check(!objects.FromGuid(0x999).HasHandle(), "an unknown guid finds nothing");
    }

    Section("iteration composes");
    {
        using wxltest::FakeObjects;
        const wxl::ObjectService objects;

        const wxl::ObjectView<wxl::Object> all = objects.InView<>();
        Check(all.size() == 3 && !all.empty(), "everything in view");

        const wxl::ObjectView<wxl::Unit> units = objects.InView<wxl::Unit>();
        Check(units.size() == 2, "units are units and players");
        Check(objects.InView<wxl::Player>().size() == 1 && objects.InView<wxl::GameObject>().size() == 1,
              "narrower classes narrow the view");
        Check(objects.InView(WXL_OBJECT_GAMEOBJECT).size() == 1, "and so does a server type");

        int seen = 0;
        for (wxl::Unit unit : units) seen += unit.IsValid() ? 1 : 0;
        Check(seen == 2, "a range-for visits each unit");

        const auto alive = std::count_if(units.begin(), units.end(),
                                         [](const wxl::Unit& u) { return u.Health().ValueOr(0) > 0; });
        Check(alive == 1, "std::count_if works over a view");

        const auto found = std::find_if(units.begin(), units.end(), [](const wxl::Unit& u) {
            return u.Name().ValueOr("") == "Boar";
        });
        Check(found != units.end() && (*found).Handle() == FakeObjects::kBoar, "std::find_if finds a unit");

        const std::vector<wxl::Unit> copied = units.ToVector();
        Check(copied.size() == 2 && copied[0] == units[0], "a view materialises to a vector");

        const wxl::Expected<std::vector<wxl_vec3>> positions = objects.Positions(units);
        Check(positions && positions->size() == 2 && (*positions)[0].x == static_cast<float>(units[0].Handle()),
              "positions read in bulk for a whole view");
    }

    Section("wxl.script");
    {
        wxl::ScriptService script;
        Check(static_cast<bool>(script) && script.Context().ValueOr(WXL_SCRIPT_CONTEXT_GLUE) == WXL_SCRIPT_CONTEXT_GAME,
              "the facade sees the context");
        Check(script.RegisterFunction<&Greet>("Greet") == WXL_OK, "a plain function registers");
        Check(script.RegisterFunction<&Greet>("Greet") == WXL_E_INVALID, "a taken name is refused");

        Adder adder;
        Check(script.RegisterFunction("Add", &adder) == WXL_OK, "a callable object registers");

        wxl_script_call call;
        call.args = {wxl::Value::String("Thorgrim"), wxl::Value::Bool(true)};
        Check(wxltest::FakeScript::Invoke("Greet", call), "the VM reaches the function");
        Check(call.results.size() == 3 && call.results[0].type == WXL_VALUE_STRING &&
                  wxl::View(call.results[0].as.string) == "Thorgrim" && call.results[1].as.number == 2.0 &&
                  call.results[2].as.boolean == WXL_TRUE,
              "arguments read and results push through the typed call");

        wxl_script_call bad;
        bad.args = {wxl::Value::Number(1.0)};
        wxltest::FakeScript::Invoke("Greet", bad);
        Check(bad.results.empty() && bad.error == "Greet(name) wants a string",
              "a wrong argument type is a miss, and Fail carries the message");

        wxl_script_call add;
        add.args = {wxl::Value::Number(2.0)};
        wxltest::FakeScript::Invoke("Add", add);
        wxltest::FakeScript::Invoke("Add", add);
        Check(adder.total == 4.0, "a callable object keeps its state between calls");

        Check(script.RegisterEvent("MY_EVENT") == WXL_OK, "an event registers");
        Check(script.FireEvent("MY_EVENT", {wxl::Value::Number(1), wxl::Value::String("x"), wxl::Value::Nil()}) == WXL_OK,
              "and fires with a list of values");
        Check(wxltest::FakeScript::Events().back() == "MY_EVENT n s -", "which arrive typed");
        Check(script.Run("print(1)", "chunk") == WXL_OK && wxltest::FakeScript::Chunks().back() == "chunk: print(1)",
              "a chunk runs with its name");
    }

    Section("wxl.storage");
    {
        wxl::StorageService storage;
        Check(storage.Exists("Interface\\hello.txt") && !storage.Exists("nope"), "exists answers");
        Check(storage.Size("Interface\\hello.txt").ValueOr(0) == 5, "a size reads");
        Check(storage.Size("nope").IsNotFound(), "a missing file is not found");
        Check(storage.ReadText("Interface\\hello.txt").ValueOr("") == "hello", "a file reads as text");
        Check(storage.Read("Interface\\big.bin").ValueOr(std::vector<uint8_t>{}).size() == 1000,
              "and as bytes");
        Check(storage.Read("nope").IsNotFound(), "reading a missing file is not found");

        const std::vector<std::string> found = storage.Find("Interface\\*");
        Check(found.size() == 2, "find lists matches, copied out");

        const int opens = wxltest::FakeStorage::Opens(), closes = wxltest::FakeStorage::Closes();
        {
            wxl::File file = storage.Open("Interface\\hello.txt");
            Check(static_cast<bool>(file) && wxltest::FakeStorage::Opens() == opens + 1, "a file opens");

            char                     buffer[8] = {};
            const wxl::Expected<uint64_t> read = file.ReadAt(1, buffer, sizeof(buffer));
            Check(read && *read == 4 && std::strncmp(buffer, "ello", 4) == 0, "and reads at an offset");

            wxl::File moved = std::move(file);
            Check(!file && moved, "a move hands the handle over");
            moved.Reset();
            moved.Reset();
            Check(wxltest::FakeStorage::Closes() == closes + 1, "Reset closes exactly once");
        }
        Check(wxltest::FakeStorage::Closes() == closes + 1, "and the destructor adds nothing");
        {
            wxl::File scoped = storage.Open("Interface\\hello.txt");
        }
        Check(wxltest::FakeStorage::Closes() == closes + 2, "a scoped file closes at the end of its scope");
        Check(!storage.Open("nope"), "a missing file opens as an empty File");
    }

    Section("wxl.net");
    {
        wxl::NetService netService;
        Check(netService.State() == WXL_NET_AUTHENTICATED, "the state reads");
        Check(netService.ReservedOpcodeLimit().ValueOr(0) == 0x51F, "the reserved limit reads");

        struct Body
        {
            uint32_t a;
            uint16_t b;
        } body{1, 2};
        Check(netService.Send(0x600, body) == WXL_OK && wxltest::FakeNet::Sent().back().second == sizeof(Body),
              "a struct sends with its size");

        const int releases = wxltest::FakeNet::Releases();
        {
            wxl::Expected<wxl::OpcodeClaim> claim = netService.Claim(0x601);
            Check(claim && *claim && claim->Opcode() == 0x601, "an opcode claims");
            Check(netService.Claim(0x601).IsInvalid(), "a second claim is refused");

            wxl::OpcodeClaim moved = std::move(*claim);
            Check(moved && !*claim, "a move hands the claim over");
            moved.Reset();
            moved.Reset();
            Check(wxltest::FakeNet::Releases() == releases + 1, "Reset releases exactly once");
        }
        Check(wxltest::FakeNet::Releases() == releases + 1, "and the destructor adds nothing");
        {
            wxl::Expected<wxl::OpcodeClaim> scoped = netService.Claim(0x602);
        }
        Check(wxltest::FakeNet::Releases() == releases + 2 && !wxltest::FakeNet::Claimed().count(0x602),
              "a scoped claim is given back at the end of its scope");
    }

    Section("wxl.ui");
    {
        wxl::UiService ui;
        Check(static_cast<bool>(ui) && ui.IsOpen() && ui.CanRemovePanels(), "the facade sees the overlay");

        const int removes = wxltest::FakeUi::Removes();
        {
            wxl::Expected<wxl::Panel> panel = ui.AddPanel<&DrawPanel>("Example");
            Check(panel && *panel && panel->Title() == "Example", "a panel registers");
            Check(ui.AddPanel<&DrawPanel>("Example").IsInvalid(), "a taken title is refused");

            wxltest::FakeUi::DrawAll();
            Check(g_panelDraws == 1 && wxltest::FakeUi::Drawn().back() == "hello", "the overlay draws it");

            wxl::Panel moved = std::move(*panel);
            Check(moved && !*panel, "a move hands the panel over");
            Check(moved.Reset() == WXL_OK && moved.Reset() == WXL_OK, "Reset removes it");
            Check(wxltest::FakeUi::Removes() == removes + 1, "exactly once");
        }
        Check(wxltest::FakeUi::Removes() == removes + 1, "and the destructor adds nothing");
        {
            struct Body
            {
                int draws = 0;
                void operator()() { ++draws; }
            } body;
            wxl::Expected<wxl::Panel> scoped = ui.AddPanel("Scoped", &body);
            wxltest::FakeUi::DrawAll();
            Check(scoped && body.draws == 1, "a callable object draws");
        }
        Check(wxltest::FakeUi::Removes() == removes + 2 && wxltest::FakeUi::Panels().empty(),
              "a scoped panel is removed at the end of its scope");

        {
            wxl::Panel older(wxltest::FakeUi::OlderTable(), "Old");
            Check(older.Reset() == WXL_E_UNSUPPORTED && wxltest::FakeUi::Removes() == removes + 2,
                  "a table that predates remove_panel is left alone and says so");
        }

        bool    flag  = false;
        float   f     = 0.0f;
        int32_t i     = 0;
        float   rgba[4] = {};
        char    text[8] = {};
        const char* const items[] = {"a", "b", "c"};
        Check(ui.Checkbox("c", flag) && flag, "a checkbox binds a bool");
        Check(ui.SliderFloat("f", f, 0.0f, 2.0f) && f == 2.0f, "a float slider binds");
        Check(ui.SliderInt("i", i, 0, 9) && i == 9, "an int slider binds");
        Check(ui.ColorEdit("rgba", rgba) && rgba[3] == 0.5f, "a colour binds");
        Check(ui.Combo("combo", i, items) && i == 2, "a combo sizes its items");
        Check(ui.InputText("t", text) && std::strcmp(text, "typed") == 0, "a text field sizes its buffer");
        Check(ui.Button("go") && ui.CollapsingHeader("h"), "the rest answer");
    }

    Section("wxl.hook");
    {
        wxl::HookService hooks;
        const std::vector<wxl::HookPoint> points = hooks.Points();
        Check(points.size() == 1 && points[0].name == "Thing" && points[0].signature == "int(int)",
              "the named points list, copied out");
        Check(hooks.HasPoint("Thing") && !hooks.HasPoint("Other"), "has_point answers");

        const int detaches = wxltest::FakeHooks::Detaches();
        {
            ThingFn   original = nullptr;
            wxl::Hook hook     = hooks.Attach("Thing", &ThingDetour, &original);
            Check(static_cast<bool>(hook), "a typed detour attaches");
            Check(reinterpret_cast<void*>(original) == reinterpret_cast<void*>(&wxltest::FakeHooks::Engine),
                  "and receives the next link, typed");

            Check(!hooks.Attach("Other", &ThingDetour, &original), "an unnamed point attaches nothing");

            wxl::Hook moved = std::move(hook);
            Check(moved && !hook, "a move hands the hook over");
            moved.Reset();
            moved.Reset();
            Check(wxltest::FakeHooks::Detaches() == detaches + 1, "Reset detaches exactly once");
        }
        Check(wxltest::FakeHooks::Detaches() == detaches + 1, "and the destructor adds nothing");
        {
            ThingFn   original = nullptr;
            wxl::Hook scoped   = hooks.Attach("Thing", &ThingDetour, &original);
        }
        Check(wxltest::FakeHooks::Detaches() == detaches + 2 && wxltest::FakeHooks::Live().empty(),
              "a scoped hook detaches at the end of its scope");

        wxl_hook raw = WXL_HOOK_NONE;
        {
            ThingFn   original = nullptr;
            wxl::Hook kept     = hooks.Attach("Thing", &ThingDetour, &original);
            raw                = kept.Release();
        }
        Check(raw != WXL_HOOK_NONE && wxltest::FakeHooks::Live().count(raw) == 1,
              "Release disowns: the hook outlives the wrapper");
        wxltest::FakeHooks::Table()->detach(raw);

        wxl::RawHookService rawHooks;
        Check(rawHooks.ImageBase().ValueOr(0) == 0x400000 && rawHooks.ImageBuild().ValueOr(0) == 12340,
              "the raw service describes the image");
        {
            ThingFn   original = nullptr;
            wxl::Hook at       = rawHooks.AttachAt("Thing", 0x401000, &ThingDetour, &original);
            Check(static_cast<bool>(at) && original != nullptr, "a detour attaches at an address");
        }
        Check(wxltest::FakeHooks::Live().empty(), "and detaches with its wrapper");
    }

    Section("wxl.patch");
    {
        using wxltest::FakePatch;
        wxl::PatchService patches;

        const uint8_t nop[1] = {0x90}, ret[1] = {0xC3}, two[2] = {0x90, 0x90}, jmp[2] = {0xEB, 0x00};
        Check(patches.Matches(FakePatch::kBase, nop) && !patches.Matches(FakePatch::kBase, ret),
              "matches reads without writing");
        Check(patches.Find(two, 2) == FakePatch::kBase && patches.Find(two, 2, FakePatch::kBase) == FakePatch::kBase + 1,
              "find walks the image");
        Check(patches.FindLiteral("zz") == 0, "a literal that is not there finds nothing");

        {
            wxl::Patch patch = patches.Apply("ret", FakePatch::kBase, nop, ret);
            Check(static_cast<bool>(patch) && FakePatch::Image()[0] == 0xC3, "a patch writes");
            Check(!patches.Apply("again", FakePatch::kBase, nop, ret), "a mismatch is refused");

            wxl::Patch moved = std::move(patch);
            Check(moved && !patch, "a move hands the patch over");
            moved.Reset();
            moved.Reset();
            Check(FakePatch::Image()[0] == 0x90 && FakePatch::RestoreOrder().size() == 1,
                  "Reset restores exactly once");
        }
        Check(FakePatch::RestoreOrder().size() == 1, "and the destructor adds nothing");

        FakePatch::RestoreOrder().clear();
        {
            wxl::PatchSet set;
            Check(set.Apply("a", FakePatch::kBase, nop, ret), "a set applies one");
            Check(set.Apply("b", FakePatch::kBase + 2, two, jmp), "and another");
            Check(!set.Apply("c", FakePatch::kBase + 2, two, jmp) && set.Size() == 2,
                  "a refused one is not kept");
            Check(FakePatch::Image()[0] == 0xC3 && FakePatch::Image()[2] == 0xEB, "both are in place");
        }
        Check(FakePatch::RestoreOrder() == std::vector<wxl_patch>{3, 2} && FakePatch::Image()[0] == 0x90 &&
                  FakePatch::Image()[2] == 0x90,
              "a set restores in reverse order of apply");

        {
            wxl::PatchSet kept;
            kept.Apply("a", FakePatch::kBase, nop, ret);
            kept.Release();
        }
        Check(FakePatch::Image()[0] == 0xC3, "Release leaves the bytes in place");
        FakePatch::Image()[0] = 0x90;
        FakePatch::Live().clear();
    }

    Section("a runtime that cannot raise an event");
    {
        Check(host.Table()->subscribe(WXL_EV_Update, nullptr, nullptr, WXL_PRIORITY_DEFAULT) ==
                  WXL_SUBSCRIPTION_NONE,
              "a null handler is refused");
        host.Withhold(WXL_EV_SoundPlay);
        wxl::Subscription none = wxl::Subscribe(WXL_EV_SoundPlay, [](void*, const void*) {}, nullptr);
        Check(!none, "a withheld event yields an empty subscription, which is an answer");
    }

    Section("log");
    {
        wxl::Log(WXL_LOG_INFO, "contract", "hello");
        Check(host.LogLines().size() == 1 && host.LogLines().back() == "[contract] hello",
              "a log line crosses with its tag and its length");
        wxl::Log(WXL_LOG_INFO, "contract", std::string_view("sliced", 5));
        Check(host.LogLines().back() == "[contract] slice", "a view crosses with its own length");
    }

    if (g_failures == 0)
    {
        std::printf("\nall checks passed\n");
        return 0;
    }

    std::printf("\n%d check(s) failed\n", g_failures);
    return 1;
}
