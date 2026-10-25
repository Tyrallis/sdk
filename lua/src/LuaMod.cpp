// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The mod itself: builds the runtime at load, hands it the stages, publishes wxl.lua.

#include <memory>

#include "Runtime.hpp"
#include "wxl/lua.h"
#include "wxl/sdk.hpp"

namespace
{
    uint32_t WXL_CALL Count()
    {
        wxl::lua::Runtime* runtime = wxl::lua::Runtime::Instance();
        return runtime ? runtime->Count() : 0;
    }

    uint32_t WXL_CALL Poll()
    {
        wxl::lua::Runtime* runtime = wxl::lua::Runtime::Instance();
        return runtime ? runtime->Poll() : 0;
    }

    wxl_result WXL_CALL Reload(const char* id)
    {
        wxl::lua::Runtime* runtime = wxl::lua::Runtime::Instance();
        return runtime ? runtime->Reload(id) : WXL_E_FAILED;
    }

    wxl_lua_api g_api{sizeof(wxl_lua_api), &Count, &Poll, &Reload};

    class LuaMod final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            runtime_ = std::make_unique<wxl::lua::Runtime>(wxl::Host());
            runtime_->Start();
            if (wxl::Host()->publish_service(WXL_LUA_SERVICE, WXL_LUA_SERVICE_VERSION, &g_api) != WXL_OK)
                wxl::Log(WXL_LOG_WARN, wxl::lua::Runtime::kTag,
                         "the wxl.lua service was not published: the name is taken");
            return WXL_OK;
        }

        void OnStage(wxl_stage stage) override { runtime_->Stage(stage); }

        void OnUnload() override
        {
            runtime_->Stop();
            runtime_.reset();
        }

    private:
        std::unique_ptr<wxl::lua::Runtime> runtime_;
    };
} // namespace

WXL_MOD(LuaMod, "wxl.lua", "Lua")
