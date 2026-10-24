// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 WarcraftXL
//
// wxl.lua against the mod host. Run with the wxl-lua binary, the lua-hello example's folder and
// a scratch directory:
//
//     wxl-lua-tests <wxl-lua.dll> <examples/lua-hello> <scratch>
//
// A mods tree is written into the scratch directory: wxl.lua itself, the example, and Lua mods
// written here that exercise what the example does not. The host loads wxl.lua through the OS
// loader, the test raises events and reads the log.

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "fake_services.hpp"
#include "wxl/host/event_bus.hpp"
#include "wxl/host/log.hpp"
#include "wxl/host/manifest.hpp"
#include "wxl/host/mod_host.hpp"
#include "wxl/lua.h"

namespace fs = std::filesystem;
using namespace wxl::host;

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

    bool Contains(const std::string& text, const char* needle)
    {
        return text.find(needle) != std::string::npos;
    }

    class CaptureSink final : public LogSink
    {
    public:
        void Write(wxl_log_level level, std::string_view tag, std::string_view message) override
        {
            lines.push_back(FormatLine(level, tag, message));
        }

        bool Has(const char* needle) const { return Count(needle) != 0; }

        size_t Count(const char* needle) const
        {
            size_t count = 0;
            for (const std::string& line : lines)
                if (Contains(line, needle)) ++count;
            return count;
        }

        /// Prints the lines, for a failing run.
        void Dump() const
        {
            for (const std::string& line : lines) std::printf("        %s\n", line.c_str());
        }

        std::vector<std::string> lines;
    };

    void WriteFile(const fs::path& path, const std::string& text)
    {
        fs::create_directories(path.parent_path());
        std::ofstream stream(path, std::ios::binary);
        stream << text;
    }

#define WXL_TEST_STR_(x) #x
#define WXL_TEST_STR(x)  WXL_TEST_STR_(x)

    constexpr const char* kAbi = WXL_TEST_STR(WXL_ABI_VERSION_MAJOR) "." WXL_TEST_STR(WXL_ABI_VERSION_MINOR);

    /// A script mod's manifest: no binaries, a lua entry.
    std::string LuaManifest(const char* id, const std::string& extra = std::string())
    {
        std::string text = "{\n  \"id\": \"";
        text += id;
        text += "\",\n  \"name\": \"";
        text += id;
        text += "\",\n  \"version\": \"1.0.0\",\n  \"abi\": \"";
        text += kAbi;
        text += "\",\n  \"lua\": \"init.lua\"";
        if (!extra.empty()) text += ",\n  " + extra;
        text += "\n}\n";
        return text;
    }

    /// wxl.lua's own manifest for this architecture.
    std::string RuntimeManifest()
    {
        std::string text = "{\n  \"id\": \"wxl.lua\",\n  \"name\": \"Lua\",\n  \"version\": \"0.1.0\",\n"
                           "  \"abi\": \"";
        text += kAbi;
        text += "\",\n  \"binaries\": { \"";
        text += CurrentTarget();
        text += "\": \"bin/wxl-lua.dll\" }\n}\n";
        return text;
    }

    /// The mods tree: wxl.lua, the example, and whatever else the section adds.
    fs::path MakeTree(const fs::path& scratch, const char* name, const fs::path& binary,
                      const fs::path& example)
    {
        const fs::path mods = scratch / name;
        fs::remove_all(mods);
        fs::create_directories(mods / "wxl.lua" / "bin");
        fs::copy_file(binary, mods / "wxl.lua" / "bin" / "wxl-lua.dll");
        WriteFile(mods / "wxl.lua" / "mod.json", RuntimeManifest());
        if (!example.empty())
            fs::copy(example, mods / "lua-hello", fs::copy_options::recursive);
        return mods;
    }

    HostConfig Config(const fs::path& mods, CaptureSink& sink)
    {
        HostConfig config;
        config.mods_directory = mods;
        config.runtime_id     = "test";
        config.runtime_name   = "Test runtime";
        config.log_sink       = &sink;
        config.events         = AllEvents();
        config.services.push_back(PublishedService{WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION,
                                                   wxltest::FakeObjects::Table()});
        return config;
    }

    const RefusedMod* FindRefused(const ModHost& host, const char* id)
    {
        for (const RefusedMod& mod : host.Refused())
            if (mod.id == id) return &mod;
        return nullptr;
    }

    // --- the probe mod: what the example leaves out ------------------------------------------

    constexpr const char* kProbeV1 = R"lua(
-- the sandbox
local problems = {}
if os.execute ~= nil then problems[#problems + 1] = "os.execute" end
if io.popen ~= nil then problems[#problems + 1] = "io.popen" end
if package.loadlib ~= nil then problems[#problems + 1] = "package.loadlib" end
if pcall(require, "ffi") then problems[#problems + 1] = "ffi" end
if io.open("../lua-hello/init.lua") ~= nil then problems[#problems + 1] = "io.open outside" end
if io.open("sub/data.txt") == nil then problems[#problems + 1] = "io.open inside" end
if pcall(require, "..lua-hello.init") then problems[#problems + 1] = "require outside" end
if require("sub.lib") ~= 42 then problems[#problems + 1] = "require inside" end
if loadstring("\27garbage") ~= nil then problems[#problems + 1] = "bytecode" end
if debug.getupvalue or debug.getregistry or debug.setupvalue then problems[#problems + 1] = "debug" end
if pcall(require, "jit.util") then problems[#problems + 1] = "jit.util" end
if type(debug.traceback()) ~= "string" then problems[#problems + 1] = "debug.traceback" end
if #problems == 0 then print("sandbox ok") else print("sandbox FAIL " .. table.concat(problems, ", ")) end

-- names are checked
local ok, err = pcall(Observe, "World", "OnUpdate", function() end)
if not ok and err:find("WorldScript") and err:find("NetScript") then print("family check ok") end
ok, err = pcall(Observe, "WorldScript", "OnTick", function() end)
if not ok and err:find("OnUpdate") and err:find("OnSoundPlay") then print("method check ok") end
ok, err = pcall(Override, "WorldScript", "OnUpdate", function() end)
if not ok and err:find("cannot be overridden") then print("override check ok") end
ok, err = pcall(registerForEvent, "onFrame", function() end)
if not ok and err:find("onDraw") then print("callback check ok") end
if #wxl.events.WorldScript >= 6 and wxl.events.NetScript[1] == "OnMessageReceived" then print("events table ok") end

registerForEvent("onInit", function() print("probe init v1") end)
registerForEvent("onUpdate", function(delta) print("probe update " .. delta) end)
registerForEvent("onDraw", function() print("probe draw") end)
registerForEvent("onShutdown", function() print("probe shutdown") end)

Observe("WorldScript", "OnUpdate", function(delta) print("probe observed update " .. delta) end)

-- a handler that errors, and one after it that must still run
Observe("WorldScript", "OnWorldEnter", function(map_id)
    error("boom " .. map_id)
end)
Observe("WorldScript", "OnWorldEnter", function(map_id, position)
    print("after-error " .. position.x .. "," .. position.y .. "," .. position.z)
end)

-- what a handler keeps is a copy, readable after the call
local kept
Observe("WorldScript", "OnWorldLeave", function(map_id, position) kept = position end)
Observe("WorldScript", "OnSoundPlay", function(sound_id, path, suppressed)
    if kept and kept.x == 1 and kept.z == 3 then print("kept values stay") end
    print("sound " .. sound_id .. " " .. path .. " suppressed=" .. tostring(suppressed))
end)

-- an Override answers with its return value; an Observe after it sees the answer
Override("WorldScript", "OnInput", function(message) return message == 7 end)
Observe("WorldScript", "OnInput", function(message, wparam, lparam, handled)
    print("input " .. message .. " " .. wparam .. " " .. lparam .. " handled=" .. tostring(handled))
end)

-- return values of the wrong kind, and too many of them
Override("WorldScript", "OnSoundPlay", function() return "loud", 1 end)

-- the off-thread events, and a string too long for the buffer
Observe("AssetScript", "OnFileOpen", function(path, redirect) print("file open " .. path .. " redirect " .. redirect) end)
Override("AssetScript", "OnFileOpen", function(path) if path == "long" then return string.rep("x", 100) end end)
Override("NetScript", "OnMessageSent", function(opcode, data)
    print("message sent " .. opcode .. " " .. #data)
    return true
end)

-- the members that only measure another are not parameters
Observe("ObjectScript", "OnSpellCastGo", function(...)
    local caster, caster_guid, target, target_guid, cast_item, spell_id, cast_count, cast_flags,
          target_flags, cast_time, hit_count, hits, misses, miss_reasons = ...
    print("cast go " .. select("#", ...) .. " parameters, spell " .. spell_id .. ", " .. hit_count ..
          " hit in " .. #hits .. " bytes, " .. #miss_reasons .. " miss, caster " .. caster_guid)
end)

-- objects
Observe("ObjectScript", "OnTargetChanged", function(previous, current)
    print("target " .. tostring(previous) .. " -> " .. current:Name() .. " " .. current:Type() ..
          " hp " .. current:Health() .. "/" .. current:MaxHealth() .. " target " .. tostring(current:Target()))
    local hp, why = current:Target():Health()
    print("boar target " .. tostring(hp) .. " " .. tostring(why))
    local entry, why2 = current:Entry()
    print("entry " .. tostring(entry) .. " " .. tostring(why2))
    local objects = wxl.object.InView("Unit")
    print("units in view " .. #objects .. " local " .. wxl.object.LocalPlayer():Name() ..
          " guid " .. wxl.object.LocalPlayer():Guid() .. " same " .. tostring(wxl.object.FromGuid("0x0000000000000100") == current))
end)
Observe("ObjectScript", "OnObjectCreate", function(object, guid, kind)
    print("created " .. tostring(object) .. " guid " .. guid .. " type " .. kind)
    local hp, why = object:Health()
    print("chest health " .. tostring(hp) .. " " .. tostring(why))
end)
Observe("LuaScript", "OnScriptVmReady", function(context, native_state)
    print("vm " .. context .. " " .. type(native_state))
end)

-- handlers defined on the family tables are subscribed after this file has run
function AssetScript:OnTextureLoad(path) print("implicit texture " .. path) end
function WorldScript:OnSoundPlay(sound_id) return sound_id == 9 end
function WorldScript:OnWorldClick(point, object, button) print("implicit click " .. button .. " at " .. point.z) end

local id = Observe("RenderScript", "OnFrameBegin", function(index) print("frame " .. index) end)
Observe("RenderScript", "OnFrameEnd", function() print("unobserve " .. tostring(Unobserve(id)) .. " " .. tostring(Unobserve(id))) end)
)lua";

    constexpr const char* kProbeV2 = R"lua(
registerForEvent("onInit", function() print("probe init v2") end)
Observe("WorldScript", "OnUpdate", function() print("probe v2 update") end)
)lua";

    constexpr const char* kBroken = R"lua(
Observe("WorldScript", "OnUpdate", function() error("always") end)
registerForEvent("onUpdate", function() print("broken still updates") end)
)lua";

    constexpr const char* kBench = R"lua(
local sum = 0
Observe("WorldScript", "OnUpdate", function(delta) sum = sum + delta end)
registerForEvent("onShutdown", function() print("bench sum " .. sum) end)
)lua";

    constexpr const char* kBenchWide = R"lua(
local sum = 0
Observe("CombatScript", "OnCombatDamage", function(source, source_guid, target, target_guid, spell_id, kind,
                                                    environment, amount)
    sum = sum + amount
end)
)lua";
} // namespace

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::printf("usage: wxl-lua-tests <wxl-lua.dll> <examples/lua-hello> <scratch>\n");
        return 2;
    }
    const fs::path binary  = argv[1];
    const fs::path example = argv[2];
    const fs::path scratch = argv[3];

    Section("loading");
    {
        const fs::path mods = MakeTree(scratch, "main", binary, example);
        WriteFile(mods / "probe" / "mod.json", LuaManifest("wxl.probe", "\"loadAfter\": [ \"wxl.lua-hello\" ]"));
        WriteFile(mods / "probe" / "init.lua", kProbeV1);
        WriteFile(mods / "probe" / "sub" / "data.txt", "data");
        WriteFile(mods / "probe" / "sub" / "lib.lua", "return 42\n");
        WriteFile(mods / "broken" / "mod.json", LuaManifest("wxl.broken"));
        WriteFile(mods / "broken" / "init.lua", kBroken);
        WriteFile(mods / "syntax" / "mod.json", LuaManifest("wxl.syntax"));
        WriteFile(mods / "syntax" / "init.lua", "this is not lua\n");
        WriteFile(mods / "needy" / "mod.json",
                  LuaManifest("wxl.needy", "\"requires\": [ { \"service\": \"wxl.nothing\", \"version\": 1 } ]"));
        WriteFile(mods / "needy" / "init.lua", "print('never')\n");
        WriteFile(mods / "badname" / "mod.json", LuaManifest("wxl.badname"));
        WriteFile(mods / "badname" / "init.lua", "function WorldScript:OnTick() end\n");
        WriteFile(mods / "Bare Folder_One" / "init.lua",
                  "registerForEvent('onInit', function() print('bare init ' .. wxl.mod.id .. ' ' .. wxl.mod.version) end)\n");
        WriteFile(mods / "!!" / "init.lua", "print('never')\n");

        CaptureSink sink;
        ModHost     host(Config(mods, sink));
        host.LoadAll();

        Check(host.Loaded().size() == 1 && host.Loaded()[0].id == "wxl.lua", "wxl.lua loads natively");
        const RefusedMod* hello = FindRefused(host, "wxl.lua-hello");
        Check(hello && hello->verdict == Verdict::Skip && Contains(hello->reason, "wxl.lua serves it"),
              "a script mod is skipped by the host with a reason naming wxl.lua");
        Check(sink.Has("[INFO ] [host] skipped wxl.lua-hello: script mod (lua: init.lua)"),
              "and that is an INFO line");
        Check(sink.Has("[wxl.lua] loaded wxl.lua-hello 1.0.0 (Lua hello) from init.lua") &&
                  sink.Has("[wxl.lua] loaded wxl.probe"),
              "wxl.lua loads the script mods");
        Check(sink.Has("mod wxl.syntax failed to load init.lua") && sink.Has("[wxl.lua] 4 script mod(s) loaded, 2 not"),
              "a mod whose entry does not compile is reported and the others load");
        Check(sink.Has("[ERROR] [wxl.lua] mod wxl.badname failed to load init.lua: WorldScript.OnTick is not an event of WorldScript; its events are OnUpdate, "),
              "a family-table member that is no event is a load error naming the events");
        Check(sink.Has("[INFO ] [wxl.lua] found bare-folder-one in folder 'Bare Folder_One' (no manifest)") &&
                  sink.Has("[wxl.lua] loaded bare-folder-one 0.0.0 (Bare Folder_One) from init.lua"),
              "a folder with init.lua and no manifest is a mod named after the folder");
        Check(sink.Has("[WARN ] [wxl.lua] folder '!!' holds an init.lua but its name makes no mod id"),
              "a folder whose name makes no id is refused with a reason");
        Check(sink.Count("[host] refused") == 0, "the host reports no problem with a manifest-less folder");
        Check(sink.Has("[wxl.lua] refused wxl.needy: requires service wxl.nothing"),
              "a script mod requiring a missing service is refused");
        Check(sink.Has("[wxl.probe] sandbox ok"), "the sandbox holds");
        Check(sink.Has("[wxl.probe] family check ok") && sink.Has("[wxl.probe] method check ok") &&
                  sink.Has("[wxl.probe] override check ok") && sink.Has("[wxl.probe] callback check ok"),
              "unknown names are errors naming the valid ones");
        Check(sink.Has("[wxl.probe] events table ok"), "wxl.events lists the contract");

        const wxl_lua_api* api = static_cast<const wxl_lua_api*>(host.Table()->get_service(WXL_LUA_SERVICE, WXL_LUA_SERVICE_VERSION));
        Check(api && api->struct_size == sizeof(wxl_lua_api) && api->count() == 6,
              "the wxl.lua service is published and counts the script mods");

        host.Stage(WXL_STAGE_EARLY);
        Check(!sink.Has("probe init v1"), "onInit waits for the ready stage");
        host.Stage(WXL_STAGE_READY);
        Check(sink.Has("[wxl.lua-hello] hello from Lua hello on Test runtime"), "the example's onInit ran, through require");
        Check(sink.Has("[wxl.probe] probe init v1"), "onInit runs at WXL_STAGE_READY");
        Check(sink.Has("[bare-folder-one] bare init bare-folder-one 0.0.0"), "the manifest-less mod runs too");

        // Update: the Observe handler and the onUpdate callback, in that order.
        const wxl_update_args tick{0.5f};
        host.Raise(WXL_EV_Update, &tick);
        Check(sink.Has("[wxl.probe] probe observed update 0.5") && sink.Has("[wxl.probe] probe update 0.5"),
              "Observe(WorldScript, OnUpdate) and onUpdate both see the delta");
        const wxl_frame_args frame{3, 0.016f};
        host.Raise(WXL_EV_UiRenderEnd, &frame);
        Check(sink.Has("[wxl.probe] probe draw"), "onDraw runs at UiRenderEnd");

        // A handler that errors: logged with the id and a traceback, the next handler still runs.
        const wxl_world_args enter{530, {1.0f, 2.0f, 3.0f}};
        host.Raise(WXL_EV_WorldEnter, &enter);
        Check(sink.Has("[ERROR] [wxl.lua] mod wxl.probe errored in its Observe handler for OnWorldEnter of WorldScript (1/25): ") &&
                  sink.Has("boom 530") && sink.Has("stack traceback:"),
              "an error names the mod, the event and carries a traceback");
        Check(sink.Has("[wxl.probe] after-error 1,2,3"), "the handler after it still runs, and reads a vec3");

        // Values outlive the call: they are copies.
        host.Raise(WXL_EV_WorldLeave, &enter);
        wxl_bool       suppressed = WXL_FALSE;
        wxl_sound_args sound{9, wxl_str{"Sound\\x.wav", 11}, &suppressed};
        host.Raise(WXL_EV_SoundPlay, &sound);
        Check(sink.Has("[wxl.probe] kept values stay"), "a value kept past the handler is still readable");
        Check(sink.Has("[wxl.probe] sound 9 Sound\\x.wav suppressed=false"), "a str and an answer arrive as parameters");
        Check(suppressed == WXL_TRUE, "a family-table handler on an answerable event answers with its return value");
        Check(sink.Has("[WARN ] [wxl.lua] mod wxl.probe: the Override of OnSoundPlay returned 2 values; it answers through 'suppressed', one value each") &&
                  sink.Has("[WARN ] [wxl.lua] mod wxl.probe: the Override of OnSoundPlay returned a string for 'suppressed'; it takes true to answer"),
              "a return value of the wrong kind, and one too many, are warned about");
        const char*           texture = "World\\x.blp";
        wxl_texture_load_args load{wxl_str{texture, static_cast<uint32_t>(std::strlen(texture))}};
        host.Raise(WXL_EV_TextureLoad, &load);
        Check(sink.Has("[wxl.probe] implicit texture World\\x.blp"), "a family-table handler is subscribed without a declaration");
        const wxl_world_click_args click{{1.0f, 2.0f, 3.5f}, WXL_HANDLE_NONE, 2};
        host.Raise(WXL_EV_WorldClick, &click);
        Check(sink.Has("[wxl.probe] implicit click 2 at 3.5"), "and reads its args");

        // An answer returned, and seen by the handler after it.
        wxl_bool       handled = WXL_FALSE;
        wxl_input_args input{7, 1, -1, &handled};
        host.Raise(WXL_EV_Input, &input);
        Check(handled == WXL_TRUE, "returning true from an Override writes the flag");
        Check(sink.Has("[wxl.probe] input 7 1 -1 handled=true"), "an Observe handler after it sees the answer");
        handled = WXL_FALSE;
        input.message = 8;
        host.Raise(WXL_EV_Input, &input);
        Check(handled == WXL_FALSE && sink.Has("[wxl.probe] input 8 1 -1 handled=false"),
              "and returning false writes nothing");

        // The example's Override returning true.
        wxl_bool         kept = WXL_FALSE;
        const char       body[] = "abcd";
        wxl_message_args message{0x1234, body, 4, &kept};
        host.Raise(WXL_EV_MessageReceived, &message);
        Check(kept == WXL_TRUE && sink.Has("[wxl.lua-hello] opcode 0x1234 with 4 bytes, kept from the client"),
              "returning true from an Override writes WXL_TRUE; bytes arrive as a string");
        kept = WXL_FALSE;
        message.opcode = 0x1;
        host.Raise(WXL_EV_MessageReceived, &message);
        Check(kept == WXL_FALSE, "returning nothing leaves the flag alone");

        // The example's Override returning a string.
        char               redirect[64] = {};
        const char*        asked        = "Interface\\Icons\\INV_Misc_QuestionMark.blp";
        wxl_file_open_args open{wxl_str{asked, static_cast<uint32_t>(std::strlen(asked))}, redirect, sizeof redirect};
        host.Raise(WXL_EV_FileOpen, &open);
        Check(std::strcmp(redirect, "Interface\\Icons\\INV_Misc_Coin_01.blp") == 0,
              "returning a string from an Override fills the buffer");
        Check(sink.Has("[wxl.probe] file open Interface\\Icons\\INV_Misc_QuestionMark.blp redirect Interface\\Icons\\INV_Misc_Coin_01.blp"),
              "an Observe of the same event sees the buffer an earlier Override filled");
        const char*        longPath = "long";
        wxl_file_open_args tooLong{wxl_str{longPath, 4}, redirect, sizeof redirect};
        redirect[0] = '\0';
        host.Raise(WXL_EV_FileOpen, &tooLong);
        Check(redirect[0] == '\0' &&
                  sink.Has("[WARN ] [wxl.lua] mod wxl.probe: the Override of OnFileOpen returned 100 bytes for 'redirect', which takes at most 63; it is not taken"),
              "a string longer than the buffer is not taken, and the warning names the limit");

        // Off the main thread: queued, delivered at the next Update, the answer not taken.
        redirect[0] = '\0';
        const size_t before = sink.Count("[wxl.probe] file open");
        std::thread([&host, &open] { host.Raise(WXL_EV_FileOpen, &open); }).join();
        Check(redirect[0] == '\0' && sink.Count("[wxl.probe] file open") == before,
              "an event raised off the main thread is not delivered there");
        host.Raise(WXL_EV_Update, &tick);
        Check(sink.Count("[wxl.probe] file open") == before + 1, "it is delivered at the next Update");
        Check(redirect[0] == '\0', "and the Override's string is not taken");
        Check(sink.Count("[WARN ] [wxl.lua] mod wxl.lua-hello: OnFileOpen was raised off the main thread") == 1,
              "which is warned about");
        std::thread([&host, &open] { host.Raise(WXL_EV_FileOpen, &open); }).join();
        host.Raise(WXL_EV_Update, &tick);
        Check(sink.Count("[WARN ] [wxl.lua] mod wxl.lua-hello: OnFileOpen was raised off the main thread") == 1,
              "once per mod and event");

        wxl_bool         sentKept = WXL_FALSE;
        wxl_message_args sent{0x77, body, 4, &sentKept};
        std::thread([&host, &sent] { host.Raise(WXL_EV_MessageSent, &sent); }).join();
        host.Raise(WXL_EV_Update, &tick);
        Check(sink.Has("[wxl.probe] message sent 119 4") && sentKept == WXL_FALSE,
              "a copied message carries its bytes; the Override's true goes nowhere");

        // Objects through handles.
        const wxl_target_args target{WXL_HANDLE_NONE, wxltest::FakeObjects::kPlayer};
        host.Raise(WXL_EV_TargetChanged, &target);
        Check(sink.Has("[wxl.probe] target nil -> Thorgrim Player hp 0/5000 target Unit(0x000000000000002B)"),
              "handle members are objects with the generated getters and the hand-written ones");
        Check(sink.Has("[wxl.probe] boar target 90 nil"), "a read through Target() reads the boar");
        Check(sink.Has("[wxl.probe] entry nil unsupported"), "a getter the runtime's table does not carry says so");
        Check(sink.Has("[wxl.probe] units in view 1 local Thorgrim guid 0x0000000000000100 same true"),
              "wxl.object lists, finds the local player, resolves a guid, and compares by handle");
        const wxl_object_args created{wxltest::FakeObjects::kChest, 0x300, WXL_OBJECT_GAMEOBJECT};
        host.Raise(WXL_EV_ObjectCreate, &created);
        Check(sink.Has("[wxl.probe] created GameObject(0x000000000000002C) guid 0x0000000000000300 type 5"),
              "a handle, a guid and a number arrive as parameters");
        Check(sink.Has("[wxl.probe] chest health nil a GameObject has no Health"),
              "a unit getter on a game object answers nil and why");
        const wxl_unit_args death{wxltest::FakeObjects::kPlayer};
        host.Raise(WXL_EV_UnitDeath, &death);
        Check(sink.Has("[wxl.lua-hello] Thorgrim died at level 80"), "the example reads a unit from a handle");
        int                vm = 0;
        wxl_script_vm_args ready{WXL_SCRIPT_CONTEXT_GAME, &vm};
        host.Raise(WXL_EV_ScriptVmReady, &ready);
        Check(sink.Has("[wxl.probe] vm 1 userdata"), "a ptr member is a lightuserdata");

        const uint8_t             hits[16]   = {};
        const uint8_t             misses[8]  = {};
        const uint8_t             reasons[1] = {2};
        const wxl_spell_cast_args go{WXL_HANDLE_NONE, 0x10, WXL_HANDLE_NONE, 0, 0, 133, 1, 0, 0, 0,
                                     2, 1, hits, sizeof hits, misses, sizeof misses, reasons};
        host.Raise(WXL_EV_SpellCastGo, &go);
        Check(sink.Has("[wxl.probe] cast go 14 parameters, spell 133, 2 hit in 16 bytes, 1 miss, caster 0x0000000000000010"),
              "a member that only measures another is not a parameter: its bytes carry it");

        // Unobserve.
        host.Raise(WXL_EV_FrameBegin, &frame);
        host.Raise(WXL_EV_FrameEnd, &frame);
        host.Raise(WXL_EV_FrameBegin, &frame);
        Check(sink.Count("[wxl.probe] frame 3") == 1 && sink.Has("[wxl.probe] unobserve true false"),
              "Unobserve drops a handler by id, once");

        // A mod that keeps failing is disabled; the others go on.
        for (int i = 0; i < 30; ++i) host.Raise(WXL_EV_Update, &tick);
        Check(sink.Has("[ERROR] [wxl.lua] mod wxl.broken is disabled: it errored 25 times"),
              "a mod reaching the error limit is disabled");
        Check(sink.Count("mod wxl.broken errored") == 25, "and errors no more");
        Check(sink.Count("[wxl.broken] broken still updates") == 24, "its callbacks stop with it");
        Check(sink.Count("[wxl.probe] probe observed update") >= 30, "the other mods keep running");

        // Reload: the file changes, the mod's state is replaced, onInit runs again.
        WriteFile(mods / "probe" / "init.lua", kProbeV2);
        Check(api && api->poll() == 1, "a poll sees the changed folder and reloads one mod");
        Check(sink.Has("[wxl.probe] probe shutdown") && sink.Has("[wxl.lua] reloading wxl.probe") &&
                  sink.Has("[wxl.probe] probe init v2"),
              "the old state's onShutdown, then the new state's onInit");
        const size_t v1 = sink.Count("[wxl.probe] probe observed update");
        host.Raise(WXL_EV_Update, &tick);
        Check(sink.Count("[wxl.probe] probe observed update") == v1 && sink.Has("[wxl.probe] probe v2 update"),
              "the old handlers are gone and the new ones run");
        Check(api && api->poll() == 0, "nothing changed: nothing reloads");
        Check(api && api->reload("wxl.nothing") == WXL_E_NOT_FOUND && api->reload("wxl.broken") == WXL_OK,
              "reload by id; an unknown id is not found");
        host.Raise(WXL_EV_Update, &tick);
        Check(sink.Count("[wxl.broken] broken still updates") == 25, "a disabled mod reloaded runs again");

        host.Shutdown();
        Check(sink.Has("[wxl.lua-hello] bye"), "onShutdown runs at WXL_STAGE_SHUTDOWN");
        Check(sink.Has("[host] unloaded wxl.lua") && host.Loaded().empty(), "and wxl.lua unloads");

        if (g_failures) sink.Dump();
    }

    Section("cost of one dispatch");
    {
        const fs::path mods = MakeTree(scratch, "bench", binary, fs::path());
        WriteFile(mods / "bench" / "mod.json", LuaManifest("wxl.bench"));
        WriteFile(mods / "bench" / "init.lua", kBench);

        CaptureSink sink;
        ModHost     host(Config(mods, sink));
        host.LoadAll();
        host.Stage(WXL_STAGE_READY);

        const wxl_update_args tick{0.001f};
        const wxl_combat_damage_args damage{WXL_HANDLE_NONE, 0x10, WXL_HANDLE_NONE, 0x20, 133, 1, 0, 250,
                                            0, 4, 0, 0, 0, WXL_TRUE, 0};
        constexpr int kRaises = 200000;
        const auto    measure = [&](wxl_event event, const void* args) {
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < kRaises; ++i) host.Raise(event, args);
            const auto end = std::chrono::steady_clock::now();
            return std::chrono::duration<double, std::nano>(end - start).count() / kRaises;
        };

        measure(WXL_EV_Update, &tick); // warm up
        const double one = measure(WXL_EV_Update, &tick);
        std::printf("  %.0f ns per Update with one Observe handler taking delta\n", one);

        WriteFile(mods / "bench" / "init.lua", kBenchWide);
        const wxl_lua_api* api = static_cast<const wxl_lua_api*>(host.Table()->get_service(WXL_LUA_SERVICE, WXL_LUA_SERVICE_VERSION));
        Check(api && api->poll() == 1, "the bench mod reloads with the wide variant");
        measure(WXL_EV_CombatDamage, &damage);
        const double wide = measure(WXL_EV_CombatDamage, &damage);
        std::printf("  %.0f ns per CombatDamage with one handler taking its 15 parameters\n", wide);

        api->reload("wxl.bench");
        WriteFile(mods / "bench" / "init.lua", "-- nothing observed\n");
        api->poll();
        measure(WXL_EV_Update, &tick);
        const double idle = measure(WXL_EV_Update, &tick);
        std::printf("  %.0f ns per Update with no Lua handler (the queue and poll checks)\n", idle);

        Check(sink.Has("[wxl.bench] bench sum"), "the handler ran");
        host.Shutdown();
    }

    if (g_failures == 0)
    {
        std::printf("\nall checks passed\n");
        return 0;
    }
    std::printf("\n%d check(s) failed\n", g_failures);
    return 1;
}
