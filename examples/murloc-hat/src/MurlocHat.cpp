// Murloc Hat: a model of the mod's own, hung on the player's head through wxl.m2, and what
// characters put on, through AppearanceChanged.

#include <cstdint>

#include "wxl/objects.hpp"
#include "wxl/sdk.hpp"
#include "wxl/services/m2.hpp"

namespace
{
    constexpr const char* kTag = "murloc-hat";

    /// The attachment point on top of a character's head, where the client hangs a helm.
    constexpr uint32_t kHead = 11;

    /// Keeps a murloc on the player's head: hangs it once the player's model is there, and again
    /// whenever the client lets it go -- a new model when the player changes shape, for one.
    class Hat final : public wxl::WorldScript
    {
    public:
        void OnUpdate(float delta) override
        {
            if (hat_ && hat_.Parent()) return; // still on the head
            wait_ -= delta;
            if (wait_ > 0.0f) return;
            wait_ = 1.0f; // one try a second while it is not

            // The model the player draws with: its mount's, while it rides one.
            void* body = wxl::M2Service::ObjectModel(wxl::Player::Local());
            if (!body) return; // not in the world yet

            if (!hat_) hat_ = wxl::M2Model(body, "Creature\\Murloc\\Murloc.m2");
            const wxl_result hung = hat_.Attach(body, kHead);
            if (hung == WXL_OK)
                wxl::Log(WXL_LOG_INFO, kTag, "a murloc sits on the player's head");
            else
                wxl::Log(WXL_LOG_WARN, kTag, "the murloc could not be hung: {}", wxl::ResultName(hung));
        }

        // The world's models go with it; a new one is made on the way back in.
        void OnWorldLeave(uint32_t, wxl_vec3) override { hat_.Reset(); }

    private:
        wxl::M2Model hat_; // given back when the mod goes
        float        wait_ = 0.0f;
    };

    /// Logs the first items characters put on, with the model file the item display names.
    class Wardrobe final : public wxl::ObjectScript
    {
    public:
        void OnAppearanceChanged(wxl_appearance_change change, void*, void*, uint32_t, uint32_t, uint32_t slot,
                                 uint32_t display, bool) override
        {
            if (change != WXL_APPEARANCE_WORN || told_ >= 10) return;
            ++told_;
            const wxl::Expected<wxl_m2_item_display> row   = wxl::M2Service::ItemDisplay(display);
            const char*                              model = row && *row->model[0] ? row->model[0] : "no model";
            wxl::Log(WXL_LOG_INFO, kTag, "slot {} puts on display {} ({})", slot, display, model);
        }

    private:
        int told_ = 0;
    };

    class MurlocHat final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            wxl::ScriptMgr::Add(&hat_);
            wxl::ScriptMgr::Add(&wardrobe_);
            return WXL_OK;
        }

    private:
        Hat      hat_;
        Wardrobe wardrobe_;
    };
} // namespace

WXL_MOD(MurlocHat, "wxl.murloc-hat", "Murloc Hat")
