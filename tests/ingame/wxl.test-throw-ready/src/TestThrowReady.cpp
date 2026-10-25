// A C++ mod whose OnReady throws. A stage that throws costs the mod: the runtime disables it, so
// its handler and its OnShutdown never run afterwards.

#include <stdexcept>

#include "wxl/sdk.hpp"

namespace
{
    constexpr const char* kTag = "test-throw-ready";

    class Never final : public wxl::ObjectScript
    {
    public:
        void OnTargetChanged(wxl::Object, wxl::Object) override
        {
            wxl::Log(WXL_LOG_ERROR, kTag, "this line must never appear: the mod is disabled");
        }
    };

    class TestThrowReady final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            wxl::ScriptMgr::Add(&never_);
            wxl::Log(WXL_LOG_INFO, kTag, "loaded: OnReady will throw");
            return WXL_OK;
        }

        void OnReady() override { throw std::runtime_error("thrown on purpose from OnReady"); }

        void OnShutdown() override
        {
            wxl::Log(WXL_LOG_ERROR, kTag, "this line must never appear either: the mod is disabled");
        }

    private:
        Never never_;
    };
} // namespace

WXL_MOD(TestThrowReady, "wxl.test-throw-ready", "Test throw at ready")
