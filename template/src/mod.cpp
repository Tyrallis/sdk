// A WarcraftXL mod: derive, override, register.

#include <cstdio>

#include "wxl/objects.hpp"
#include "wxl/sdk.hpp"

namespace
{
    constexpr const char* kTag = "example";

    /// Reacts to the game's own facts.
    class ExampleWorld final : public wxl::WorldScript
    {
    public:
        void OnWorldEnter(const wxl_world_args& args) override
        {
            char line[128];
            std::snprintf(line, sizeof(line), "entered map %u at %.1f %.1f %.1f", args.map_id,
                          args.position.x, args.position.y, args.position.z);
            wxl::Log(WXL_LOG_INFO, kTag, line);

            // The object service is optional here: on a runtime that does not publish it, the mod
            // keeps working and simply says less.
            if (!wxl::HasObjects()) return;

            const wxl::Player me = wxl::Player::Local();
            if (!me) return;

            std::snprintf(line, sizeof(line), "local player %s, %u/%u hp", me.Name().c_str(),
                          me.Health(), me.MaxHealth());
            wxl::Log(WXL_LOG_INFO, kTag, line);
        }
    };

    /// Runs in the frame.
    class ExampleRender final : public wxl::RenderScript
    {
    public:
        void OnFrameEnd(const wxl_frame_args& args) override
        {
            if (args.index % 3600 != 0) return;

            char line[64];
            std::snprintf(line, sizeof(line), "frame %llu",
                          static_cast<unsigned long long>(args.index));
            wxl::Log(WXL_LOG_DEBUG, kTag, line);
        }
    };

    ExampleWorld  g_world;
    ExampleRender g_render;

    class ExampleMod final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            wxl::ScriptMgr::Add(&g_world);
            wxl::ScriptMgr::Add(&g_render);

            const wxl_runtime_info* runtime = wxl::Host()->runtime();
            char                    line[128];
            std::snprintf(line, sizeof(line), "loaded on %.*s", static_cast<int>(runtime->name.size),
                          runtime->name.data);
            wxl::Log(WXL_LOG_INFO, kTag, line);
            return WXL_OK;
        }

        void OnStage(wxl_stage stage) override
        {
            if (stage == WXL_STAGE_READY) wxl::Log(WXL_LOG_INFO, kTag, "client is up");
        }

        void OnUnload() override { wxl::Log(WXL_LOG_INFO, kTag, "going away"); }
    };
} // namespace

WXL_MOD(ExampleMod, "wxl.example", "Example", 1)
