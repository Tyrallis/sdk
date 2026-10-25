// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.m2: the models objects draw with, and models a mod hangs on them.

#ifndef WXL_SERVICE_M2_HPP
#define WXL_SERVICE_M2_HPP

#include <cstdint>
#include <string_view>
#include <utility>

#include "wxl/objects.hpp"
#include "wxl/sdk.hpp"
#include "wxl/services/m2.h"
#include "wxl/services/rows.hpp"

namespace wxl::api::facade
{
    template <>
    struct Methods<wxl_m2_api>
    {
#define WXL_API_TABLE "wxl/services/m2.def"
#include "wxl/services/methods.inc"
    };
} // namespace wxl::api::facade

namespace wxl
{
    /**
     * @brief The typed facade over wxl.m2.
     *
     *     void OnAppearanceChanged(uint32_t change, void*, void* model, uint32_t, uint32_t,
     *                              uint32_t slot, uint32_t display, bool) override
     *     {
     *         if (change != WXL_APPEARANCE_WORN || slot != 4) return;   // a belt put on
     *         buckle_ = wxl::M2Model(model, "Item\\Buckle.m2");
     *         buckle_.Attach(model, 53, WXL_M2_ATTACH_ROOT);
     *     }
     *
     *   ObjectModel(object)               the model an object draws with
     *   ItemDisplay(id)                   an ItemDisplayInfo row
     *   RacePrefix(race)                  "Hu", "Or": what model file names carry
     *   CreateModel(scene, path)          a model of the mod's (M2Model holds one)
     *   Attach(model, parent, point, f)   hangs it on a model (WXL_M2_ATTACH_* flags)
     *   Detach(model), Parent(model)      takes it off, what it hangs on
     *   SetGeosets(model, first, last, v) shows or hides geosets
     *   ReplaceTexture(model, type, path) a texture of its own
     *   AttachmentPoint(model, point)     where a point sits in the bind pose
     *   AttachToBone(model, parent, point, bone, offset)  an attachment point made by hand
     *
     * Main thread only. A missing service answers null, nothing, or WXL_E_UNSUPPORTED.
     */
    class M2Service : public detail::Facade<M2Service, wxl_m2_api>, public api::facade::Methods<wxl_m2_api>
    {
    public:
        static constexpr const char* kName    = WXL_M2_SERVICE;
        static constexpr uint32_t    kVersion = WXL_M2_SERVICE_VERSION;

        /// The ItemDisplayInfo row @p display_id; WXL_E_NOT_FOUND for none.
        static Expected<wxl_m2_item_display> ItemDisplay(uint32_t display_id)
        {
            const wxl_m2_api* api = Raw();
            if (!detail::Carries(api, &wxl_m2_api::item_display)) return Unexpected{WXL_E_UNSUPPORTED};
            wxl_m2_item_display row{};
            row.struct_size         = sizeof row;
            const wxl_result result = api->item_display(display_id, &row);
            if (result != WXL_OK) return Unexpected{result};
            return row;
        }

        /// Gives back a model this mod made; nothing for null.
        static void Release(void* model)
        {
            const wxl_m2_api* api = Raw();
            if (model && detail::Carries(api, &wxl_m2_api::release_model)) api->release_model(model);
        }
    };

    /**
     * @brief A model of the mod's own, given back when this goes: movable, not copyable.
     *
     * Empty when the service is missing or the scene made none; every call on an empty one does
     * nothing and answers WXL_E_INVALID.
     */
    class M2Model
    {
    public:
        M2Model() = default;
        /// Makes a model of @p path in the scene @p scene_model is in.
        M2Model(void* scene_model, std::string_view path) : model_(M2Service::CreateModel(scene_model, path)) {}
        M2Model(M2Model&& other) noexcept : model_(std::exchange(other.model_, nullptr)) {}
        M2Model& operator=(M2Model&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                model_ = std::exchange(other.model_, nullptr);
            }
            return *this;
        }
        M2Model(const M2Model&)            = delete;
        M2Model& operator=(const M2Model&) = delete;
        ~M2Model() { Reset(); }

        void* Get() const { return model_; }
        explicit operator bool() const { return model_ != nullptr; }

        /// Gives the model back now.
        void Reset()
        {
            M2Service::Release(model_);
            model_ = nullptr;
        }

        wxl_result Attach(void* parent, uint32_t attachment, uint32_t flags = 0) const
        {
            return model_ ? M2Service::Attach(model_, parent, attachment, flags) : WXL_E_INVALID;
        }
        wxl_result AttachToBone(void* parent, uint32_t attachment, uint32_t key_bone, wxl_vec3 offset) const
        {
            return model_ ? M2Service::AttachToBone(model_, parent, attachment, key_bone, offset) : WXL_E_INVALID;
        }
        void  Detach() const { if (model_) M2Service::Detach(model_); }
        void* Parent() const { return model_ ? M2Service::Parent(model_) : nullptr; }
        void  SetGeosets(uint32_t first, uint32_t last, bool visible) const
        {
            if (model_) M2Service::SetGeosets(model_, first, last, visible);
        }
        wxl_result ReplaceTexture(uint32_t type, std::string_view path) const
        {
            return model_ ? M2Service::ReplaceTexture(model_, type, path) : WXL_E_INVALID;
        }

    private:
        void* model_ = nullptr;
    };
} // namespace wxl

#endif // WXL_SERVICE_M2_HPP
