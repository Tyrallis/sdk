// Repairs wxl.test-reload's manifest. That Lua mod breaks its own mod.json on request and keeps
// the good one as mod.json.good; once broken it is unloaded and cannot write anything, so the
// repair comes from here: TestManifestRepair() in the client's Lua moves mod.json.good back over
// mod.json, which makes the runtime load the mod again. The same repair runs at load and at
// shutdown, so a client closed in between never leaves the folder broken. It touches those two
// files of that one folder and nothing else.

#include <cstddef>
#include <filesystem>
#include <string>
#include <system_error>

#include "wxl/sdk.hpp"
#include "wxl/services/script.hpp"

namespace
{
    constexpr const char* kTag = "test-manifest";

    namespace fs = std::filesystem;

    /// <Mods>/wxl.test-reload, or empty when the runtime scans no folder.
    fs::path TargetFolder()
    {
        const wxl_host* host = wxl::Host();
        const wxl_runtime_info* info = host ? host->runtime() : nullptr;
        constexpr size_t kNeeded = offsetof(wxl_runtime_info, mods_directory) + sizeof(wxl_str);
        if (!info || info->struct_size < kNeeded || !info->mods_directory.data || info->mods_directory.size == 0)
            return fs::path();
        // The folder is UTF-8: read as such, whatever the process's code page.
        const std::u8string mods(reinterpret_cast<const char8_t*>(info->mods_directory.data),
                                 info->mods_directory.size);
        return fs::path(mods) / "wxl.test-reload";
    }

    /// Moves mod.json.good over mod.json. Answers what happened, for the log and the caller.
    std::string Repair(bool& repaired)
    {
        repaired = false;
        const fs::path folder = TargetFolder();
        if (folder.empty()) return "the runtime names no Mods folder";
        const fs::path good = folder / "mod.json.good";
        std::error_code ec;
        if (!fs::is_regular_file(good, ec)) return "nothing to repair: no mod.json.good";
        fs::copy_file(good, folder / "mod.json", fs::copy_options::overwrite_existing, ec);
        if (ec) return "mod.json not written: " + ec.message();
        fs::remove(good, ec);
        repaired = true;
        return "mod.json of wxl.test-reload put back";
    }

    /// TestManifestRepair() in the client's Lua: true and what was done, or false and why not.
    class Functions final : public wxl::LuaScript
    {
    public:
        void OnScriptVmReady(wxl_script_context, void*) override
        {
            const wxl::ScriptService script;
            if (!script) return;
            const wxl_result result = script.RegisterFunction<&Functions::RepairCall>("TestManifestRepair", this);
            wxl::Log(WXL_LOG_INFO, kTag, "TestManifestRepair registered: {}", wxl::ResultName(result));
        }

        void RepairCall(wxl::ScriptCall& call)
        {
            bool repaired = false;
            const std::string what = Repair(repaired);
            wxl::Log(WXL_LOG_INFO, kTag, "TestManifestRepair: {}", what);
            call.PushBool(repaired);
            call.PushString(what);
        }
    };

    class TestManifest final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            RepairQuietly("at load");
            wxl::ScriptMgr::Add(&functions_);
            return WXL_OK;
        }

        void OnShutdown() override { RepairQuietly("at shutdown"); }

    private:
        static void RepairQuietly(const char* when)
        {
            bool repaired = false;
            const std::string what = Repair(repaired);
            if (repaired) wxl::Log(WXL_LOG_INFO, kTag, "{}: {}", when, what);
        }

        Functions functions_;
    };
} // namespace

WXL_MOD(TestManifest, "wxl.test-manifest", "Test manifest")
