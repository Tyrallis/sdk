// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The guide's example, end to end: Scout's real DLL, built from examples/scout, loaded by the mod
// host from a Mods tree as a runtime would, with wxl.object and wxl.script played by the fakes.
// The events are raised in the order a session raises them, and every log line and script answer
// the guide shows is checked.
//
//     wxl-scout-tests <scout.dll> <mod.json> <scratch directory>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "fake_services.hpp"
#include "wxl/host/log.hpp"
#include "wxl/host/mod_host.hpp"

namespace
{
    namespace fs = std::filesystem;
    using wxltest::FakeObjects;
    using wxltest::FakeScript;

    int g_failures = 0;

    void Check(bool condition, const char* what)
    {
        std::printf("  %s  %s\n", condition ? "ok  " : "FAIL", what);
        if (!condition) ++g_failures;
    }

    void Section(const char* title) { std::printf("%s\n", title); }

    /// Every line the host and the mod log, kept to be searched.
    class Lines final : public wxl::host::LogSink
    {
    public:
        void Write(wxl_log_level level, std::string_view tag, std::string_view message) override
        {
            const std::lock_guard<std::mutex> hold(lock_);
            lines_.push_back(wxl::host::FormatLine(level, tag, message));
        }

        bool Has(const std::string& line) const
        {
            const std::lock_guard<std::mutex> hold(lock_);
            for (const std::string& kept : lines_)
                if (kept == line) return true;
            return false;
        }

        bool Mentions(const char* part) const
        {
            const std::lock_guard<std::mutex> hold(lock_);
            for (const std::string& kept : lines_)
                if (kept.find(part) != std::string::npos) return true;
            return false;
        }

        void Print() const
        {
            const std::lock_guard<std::mutex> hold(lock_);
            for (const std::string& kept : lines_) std::printf("    | %s\n", kept.c_str());
        }

    private:
        mutable std::mutex       lock_;
        std::vector<std::string> lines_;
    };

    wxl_value Number(double n)
    {
        wxl_value v{};
        v.type      = WXL_VALUE_NUMBER;
        v.as.number = n;
        return v;
    }

    wxl_value String(const char* text)
    {
        wxl_value v{};
        v.type      = WXL_VALUE_STRING;
        v.as.string = wxl_str{text, static_cast<uint32_t>(std::strlen(text))};
        return v;
    }

    wxl_value Bool(bool flag)
    {
        wxl_value v{};
        v.type       = WXL_VALUE_BOOL;
        v.as.boolean = flag ? WXL_TRUE : WXL_FALSE;
        return v;
    }

    /// Calls a function Scout registered, as the client's Lua would.
    wxl_script_call Call(const char* name, std::vector<wxl_value> args = {})
    {
        wxl_script_call call;
        call.args = std::move(args);
        if (!FakeScript::Invoke(name, call)) call.error = "<not registered>";
        return call;
    }

    std::string Text(const wxl_value& value)
    {
        return value.type == WXL_VALUE_STRING ? std::string(value.as.string.data, value.as.string.size)
                                              : std::string("<not a string>");
    }

    /// How many lines of the fake's event log start with @p prefix: declarations and fires.
    size_t EventLines(const char* prefix)
    {
        size_t count = 0;
        for (const std::string& line : FakeScript::Events())
            if (line.rfind(prefix, 0) == 0) ++count;
        return count;
    }
} // namespace

int main(int argc, char** argv)
{
    // Unbuffered, so the last line printed before a crash is never held back in a buffer.
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 4)
    {
        std::printf("usage: wxl-scout-tests <scout.dll> <mod.json> <scratch directory>\n");
        return 2;
    }

    // The mod folder as the build lays it out: Mods/wxl.scout/{mod.json, bin/<arch>/scout.dll}.
    const fs::path scratch = argv[3];
    const fs::path folder  = scratch / "Mods" / "wxl.scout";
    const fs::path bin     = folder / "bin" / (sizeof(void*) == 8 ? "x64" : "x86");
    std::error_code ignored;
    fs::remove_all(scratch, ignored);
    fs::create_directories(bin);
    fs::copy_file(argv[1], bin / "scout.dll");
    fs::copy_file(argv[2], folder / "mod.json");

    Lines                  log;
    wxl::host::HostConfig config;
    config.mods_directory = scratch / "Mods";
    config.runtime_id     = "test";
    config.runtime_name   = "Test runtime";
    config.log_sink       = &log;
    config.events         = wxl::host::AllEvents();
    config.services       = {{WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION, FakeObjects::Table()},
                             {WXL_SCRIPT_SERVICE, WXL_SCRIPT_SERVICE_VERSION, FakeScript::Table()}};

    wxl::host::ModHost             host(config);
    const FakeScript::Raise raise = [&](wxl_event event, const void* args) { host.Raise(event, args); };

    Section("loading");
    {
        // The client has no Lua yet when the mods load.
        FakeScript::Close(nullptr);
        host.LoadAll();
        Check(host.Loaded().size() == 1 && host.Loaded()[0].id == "wxl.scout", "Scout loads");
        Check(log.Has("[INFO ] [scout] watching, on Test runtime"), "OnLoad logs the runtime's name");
        Check(log.Mentions("[INFO ] [host] loaded wxl.scout 1.0.0 (Scout) from bin/"),
              "the host's line gives the manifest's version");
        for (wxl_stage stage : {WXL_STAGE_EARLY, WXL_STAGE_STORAGE, WXL_STAGE_GRAPHICS,
                                WXL_STAGE_SCRIPT, WXL_STAGE_READY})
            host.Stage(stage);
        Check(!log.Mentions("no wxl.script"), "OnReady finds wxl.script");
    }

    Section("the login screens");
    {
        FakeScript::Open(WXL_SCRIPT_CONTEXT_GLUE, raise);
        Check(FakeScript::Functions().size() == 5 && FakeScript::Functions().count("ScoutEntry") &&
                  FakeScript::Functions().count("ScoutDrop"),
              "its five functions are registered");
        Check(FakeScript::Declared().count("SCOUT_UPDATED") == 1, "and its event declared");
        Check(log.Has("[INFO ] [scout] functions registered on the login screens"),
              "the context arrives by name");
        FakeScript::Close(raise);
        Check(FakeScript::Functions().empty(), "closing the context takes them away");
    }

    Section("the world");
    FakeScript::Open(WXL_SCRIPT_CONTEXT_GAME, raise);
    Check(log.Has("[INFO ] [scout] functions registered in the game"),
          "a new context gets them again");
    Check(FakeScript::Functions().size() == 5, "all five");

    const wxl_update_args tick{0.5f};
    host.Raise(WXL_EV_Update, &tick);
    const wxl_world_args enter{571, wxl_vec3{5800.4f, 640.9f, 600.0f}};
    host.Raise(WXL_EV_WorldEnter, &enter);
    Check(log.Has("[INFO ] [scout] entered map 571 at 5800, 641"), "entering is logged, formatted");

    Section("the units around");
    {
        const wxl_object_args me{FakeObjects::kPlayer, 0x100, WXL_OBJECT_PLAYER};
        const wxl_object_args boar{FakeObjects::kBoar, 0x200, WXL_OBJECT_UNIT};
        const wxl_object_args chest{FakeObjects::kChest, 0, WXL_OBJECT_GAMEOBJECT};
        host.Raise(WXL_EV_ObjectCreate, &me);
        host.Raise(WXL_EV_ObjectCreate, &boar);
        host.Raise(WXL_EV_ObjectCreate, &chest);

        const wxl_script_call count = Call("ScoutCount");
        Check(count.results.size() == 1 && count.results[0].as.number == 1,
              "only the boar is noted: not the player, not the chest");
        Check(EventLines("SCOUT_UPDATED s s n") == 1, "SCOUT_UPDATED fires with what, who, level");

        host.Raise(WXL_EV_Update, &tick);
        const wxl_script_call newest = Call("ScoutEntry");
        Check(newest.results.size() == 4 && Text(newest.results[0]) == "appeared" &&
                  Text(newest.results[1]) == "Boar" && newest.results[2].as.number == 3 &&
                  newest.results[3].as.number == 0,
              "ScoutEntry() is the newest line: appeared, Boar, 3, 0 whole seconds ago");

        const wxl_target_args target{WXL_HANDLE_NONE, FakeObjects::kBoar};
        host.Raise(WXL_EV_TargetChanged, &target);
        const wxl_unit_args death{FakeObjects::kBoar};
        host.Raise(WXL_EV_UnitDeath, &death);
        const wxl_object_args stale{FakeObjects::kStale, 0x300, WXL_OBJECT_UNIT};
        host.Raise(WXL_EV_ObjectDestroy, &stale);

        const wxl_script_call after = Call("ScoutCount");
        Check(after.results.size() == 1 && after.results[0].as.number == 3,
              "the target and the death are noted, a unit that cannot be read is not");
        const wxl_script_call second = Call("ScoutEntry", {Number(2)});
        Check(second.results.size() == 4 && Text(second.results[0]) == "targeted",
              "ScoutEntry(2) is the line before");
        const wxl_script_call past = Call("ScoutEntry", {Number(9)});
        Check(past.results.empty() && past.error.empty(), "past the oldest it answers nothing");
        const wxl_script_call zero = Call("ScoutEntry", {Number(0)});
        Check(zero.error == "ScoutEntry([index]): index is a whole number from 1, the newest line",
              "ScoutEntry(0) fails the call with Scout's message");
        const wxl_script_call nearby = Call("ScoutNearby");
        Check(nearby.results.size() == 1 && nearby.results[0].as.number == 2,
              "ScoutNearby counts the units in view");
    }

    Section("the traffic");
    {
        const uint8_t    body[4]    = {1, 2, 3, 4};
        wxl_bool         suppressed = WXL_FALSE;
        wxl_message_args in{0x0085, body, 4, &suppressed};
        host.Raise(WXL_EV_MessageReceived, &in);
        host.Raise(WXL_EV_MessageReceived, &in);
        Check(suppressed == WXL_FALSE, "nothing is dropped unasked");

        // Sent from another thread, as the connection's thread does, while Lua reads the tallies.
        std::thread connection([&] {
            for (int i = 0; i < 2000; ++i)
            {
                wxl_bool         untouched = WXL_FALSE;
                wxl_message_args out{0x01DA, body, 3, &untouched};
                host.Raise(WXL_EV_MessageSent, &out);
            }
        });
        for (int i = 0; i < 2000; ++i) Call("ScoutTraffic", {Number(0x01DA), String("sent")});
        connection.join();

        const wxl_script_call received = Call("ScoutTraffic", {Number(0x0085)});
        Check(received.results.size() == 2 && received.results[0].as.number == 2 &&
                  received.results[1].as.number == 8,
              "ScoutTraffic(0x85) is 2 messages, 8 bytes");
        const wxl_script_call sent = Call("ScoutTraffic", {Number(0x01DA), String("sent")});
        Check(sent.results.size() == 2 && sent.results[0].as.number == 2000 &&
                  sent.results[1].as.number == 6000,
              "ScoutTraffic(0x1DA, \"sent\") counts the other thread's 2000");
        const wxl_script_call bad = Call("ScoutTraffic", {String("x")});
        Check(bad.error == "ScoutTraffic(opcode [, \"sent\"]): opcode is a whole number from 0 to 0xFFFF",
              "a bad opcode fails the call");

        const wxl_script_call drop = Call("ScoutDrop", {Number(0x0085), Bool(true)});
        Check(drop.results.size() == 1 && drop.results[0].as.boolean == WXL_FALSE,
              "ScoutDrop answers that the opcode was not dropped before");
        host.Raise(WXL_EV_MessageReceived, &in);
        Check(suppressed == WXL_TRUE, "a dropped opcode is suppressed");
        Check(log.Has("[WARN ] [scout] opcode 0x0085 is now dropped before the client sees it"),
              "dropping is a warning");
        const wxl_script_call undrop = Call("ScoutDrop", {Number(0x0085), Bool(false)});
        Check(undrop.results.size() == 1 && undrop.results[0].as.boolean == WXL_TRUE &&
                  log.Has("[INFO ] [scout] opcode 0x0085 is delivered again"),
              "undropping answers that it was, and says so");
        suppressed = WXL_FALSE;
        host.Raise(WXL_EV_MessageReceived, &in);
        Check(suppressed == WXL_FALSE, "the opcode is delivered again");
        Check(!Call("ScoutDrop", {Number(0x0085)}).error.empty(), "ScoutDrop without its boolean fails");
    }

    Section("leaving");
    {
        FakeScript::Close(raise);
        const size_t fired = EventLines("SCOUT_UPDATED s");
        const wxl_world_args leave{571, enter.position};
        host.Raise(WXL_EV_WorldLeave, &leave);
        Check(log.Has("[INFO ] [scout] leaving map 571 with 3 line(s) in the journal"),
              "leaving is logged");
        const wxl_object_args boar{FakeObjects::kBoar, 0x200, WXL_OBJECT_UNIT};
        host.Raise(WXL_EV_ObjectCreate, &boar);
        Check(EventLines("SCOUT_UPDATED s") == fired, "nothing is fired while no context is open");
    }

    Section("shutting down");
    {
        host.Shutdown();
        Check(log.Has("[INFO ] [scout] 4 line(s) written this session"), "OnShutdown logs its count");
        Check(log.Has("[INFO ] [host] unloaded wxl.scout"), "the host unloads it");
        Check(!log.Mentions("could not") && !log.Mentions("faulted") && !log.Mentions("threw") &&
                  !log.Mentions("disabled"),
              "nothing was refused, faulted, thrown or disabled");
    }

    if (g_failures == 0)
    {
        std::printf("\nall checks passed\n");
        return 0;
    }
    std::printf("\n%d check(s) failed; the log:\n", g_failures);
    log.Print();
    return 1;
}
