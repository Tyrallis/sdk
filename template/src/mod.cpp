// A WarcraftXL mod: derive, override, register.

#include <algorithm>
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
        void OnWorldEnter(uint32_t map_id, wxl_vec3 position) override
        {
            char line[160];
            std::snprintf(line, sizeof(line), "entered map %u at %.1f %.1f %.1f", map_id, position.x,
                          position.y, position.z);
            wxl::Log(WXL_LOG_INFO, kTag, line);

            // The object service is optional here: on a runtime that does not publish it, the mod
            // keeps working and simply says less.
            const wxl::Player me = wxl::Player::Local();
            if (!me) return;

            // A read answers with the value or with why there is none: a dead unit reads as 0,
            // a gone one as not found, a runtime without the service as unsupported.
            const wxl::Expected<std::string> name   = me.Name();
            const wxl::Expected<uint32_t>    health = me.Health();
            if (name && health)
            {
                std::snprintf(line, sizeof(line), "local player %s, %u/%u hp", name->c_str(), *health,
                              me.MaxHealth().ValueOr(0));
                wxl::Log(WXL_LOG_INFO, kTag, line);
            }

            // Everything alive in view, as a range.
            const wxl::ObjectService            objects;
            const wxl::ObjectView<wxl::Unit>    units = objects.InView<wxl::Unit>();
            const auto                          alive = std::count_if(
                units.begin(), units.end(), [](const wxl::Unit& unit) { return unit.Health().ValueOr(0) > 0; });
            std::snprintf(line, sizeof(line), "%zu unit(s) in view, %lld alive", units.size(),
                          static_cast<long long>(alive));
            wxl::Log(WXL_LOG_INFO, kTag, line);
        }
    };

    /// Runs in the frame.
    class ExampleRender final : public wxl::RenderScript
    {
    public:
        void OnFrameEnd(uint64_t index, float) override
        {
            if (index % 3600 != 0) return;

            char line[64];
            std::snprintf(line, sizeof(line), "frame %llu", static_cast<unsigned long long>(index));
            wxl::Log(WXL_LOG_DEBUG, kTag, line);
        }
    };

    /// One event outside the script families, held as a subscription that releases itself. It
    /// takes the event's parameters, as an override would: wxl::HandlerOf<WXL_EV_TargetChanged>.
    struct TargetWatcher
    {
        void operator()(wxl::Object, wxl::Object current) const
        {
            const wxl::Unit                  target(current.Handle());
            const wxl::Expected<std::string> name = target.Name();
            wxl::Log(WXL_LOG_INFO, kTag, name ? "target: " + *name : "target cleared");
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
            targets_ = wxl::Subscribe<WXL_EV_TargetChanged>(&targetWatcher_);

            const wxl_runtime_info* runtime = wxl::Host()->runtime();
            char                    line[128];
            std::snprintf(line, sizeof(line), "loaded on %.*s", static_cast<int>(runtime->name.size),
                          runtime->name.data);
            wxl::Log(WXL_LOG_INFO, kTag, line);
            return WXL_OK;
        }

        void OnReady() override { wxl::Log(WXL_LOG_INFO, kTag, "client is up"); }

        void OnShutdown() override { wxl::Log(WXL_LOG_INFO, kTag, "client is coming down"); }

        void OnUnload() override { wxl::Log(WXL_LOG_INFO, kTag, "going away"); }

    private:
        TargetWatcher     targetWatcher_;
        wxl::Subscription targets_; // released when the mod is destroyed, before its code is gone
    };
} // namespace

WXL_MOD(ExampleMod, "wxl.example", "Example", 1)
