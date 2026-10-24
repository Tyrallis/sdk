// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// Every public header of the SDK in one mod, built at the strictest warning level with warnings
// as errors, once as C++17 and once as C++20: a mod that includes the SDK compiles clean. Nothing
// here runs; building it is the check.

#include "wxl/reflect.hpp"
#include "wxl/services.hpp"

namespace
{
    constexpr const char* kTag = "headers";

    /// One handler of each parameter shape, so their templates are instantiated.
    class Everything final : public wxl::ObjectScript
    {
    public:
        void OnObjectCreate(wxl::Object object, wxl_guid, wxl_object_type type) override
        {
            if (wxl::Unit::Accepts(type)) kept_[object] = wxl::Unit(object).Level().ValueOr(0u);
        }
        void OnUnitDeath(wxl::Object unit) override { kept_.Erase(unit); }

    private:
        wxl::ObjectMap<uint32_t> kept_;
    };

    class Interface final : public wxl::LuaScript
    {
    public:
        void OnScriptVmReady(wxl_script_context, void*) override
        {
            const wxl::ScriptService script;
            if (script.RegisterFunction<&Interface::Count>("HeadersCount", this) != WXL_OK)
                wxl::Log(WXL_LOG_WARN, kTag, "not registered");
        }

    private:
        void Count(wxl::ScriptCall& call) const { call.PushNumber(count_); }

        double count_ = 0.0;
    };

    struct Tick
    {
        void operator()(float delta) const
        {
#if defined(__cpp_lib_format) && __cpp_lib_format >= 202207L
            wxl::Log(WXL_LOG_TRACE, kTag, "tick {:.3f}", delta);
#else
            (void)delta;
#endif
        }
    };

    class HeadersMod final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            wxl::ScriptMgr::Add(&objects_);
            wxl::ScriptMgr::Add(&interface_);
            tick_ = wxl::Subscribe<WXL_EV_Update>(&tickHandler_);
            uint32_t count = 0;
            wxl::reflect::Enums(&count);
            wxl::Log(WXL_LOG_INFO, kTag, wxl::ResultName(count ? WXL_OK : WXL_E_FAILED));
            return WXL_OK;
        }

    private:
        Everything        objects_;
        Interface         interface_;
        Tick              tickHandler_;
        wxl::Subscription tick_;
    };
} // namespace

WXL_MOD(HeadersMod, "wxl.headers", "Headers")
