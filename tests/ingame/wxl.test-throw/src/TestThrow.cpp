// A C++ mod whose handler and script function throw. Each throw is caught where the SDK called
// the mod's code and costs that call only: the second handler still runs, the next event still
// comes, and the mod stays enabled until the client closes.

#include <stdexcept>

#include "wxl/sdk.hpp"
#include "wxl/services/script.hpp"

namespace
{
    constexpr const char* kTag = "test-throw";

    int g_throws = 0;

    /// Throws on every new target.
    class Thrower final : public wxl::ObjectScript
    {
    public:
        void OnTargetChanged(wxl::Object, wxl::Object current) override
        {
            if (!current.HasHandle()) return;
            ++g_throws;
            wxl::Log(WXL_LOG_INFO, kTag, "target changed: throwing from OnTargetChanged (throw {})", g_throws);
            throw std::runtime_error("thrown on purpose from OnTargetChanged");
        }
    };

    /// Subscribed after Thrower: it must still run after each throw.
    class After final : public wxl::ObjectScript
    {
    public:
        void OnTargetChanged(wxl::Object, wxl::Object current) override
        {
            if (!current.HasHandle()) return;
            wxl::Log(WXL_LOG_INFO, kTag, "the next handler still ran (throw {})", g_throws);
        }
    };

    /// TestThrow() in the client's Lua: throws, so the script call fails with the message.
    class Functions final : public wxl::LuaScript
    {
    public:
        void OnScriptVmReady(wxl_script_context, void*) override
        {
            const wxl::ScriptService script;
            if (!script) return;
            const wxl_result result = script.RegisterFunction<&Functions::Throw>("TestThrow", this);
            wxl::Log(WXL_LOG_INFO, kTag, "TestThrow registered: {}", wxl::ResultName(result));
        }

        void Throw(wxl::ScriptCall&)
        {
            ++g_throws;
            wxl::Log(WXL_LOG_INFO, kTag, "TestThrow called: throwing (throw {})", g_throws);
            throw std::runtime_error("thrown on purpose from TestThrow");
        }
    };

    class TestThrow final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            wxl::ScriptMgr::Add(&thrower_);
            wxl::ScriptMgr::Add(&after_);
            wxl::ScriptMgr::Add(&functions_);
            wxl::Log(WXL_LOG_INFO, kTag, "loaded: target something, or /run TestThrow()");
            return WXL_OK;
        }

        void OnReady() override { wxl::Log(WXL_LOG_INFO, kTag, "ready"); }

        void OnShutdown() override
        {
            wxl::Log(WXL_LOG_INFO, kTag, "still enabled at shutdown, {} throw(s) contained", g_throws);
        }

    private:
        Thrower   thrower_;
        After     after_;
        Functions functions_;
    };
} // namespace

WXL_MOD(TestThrow, "wxl.test-throw", "Test throw")
