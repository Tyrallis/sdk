// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/runtime/object_service.hpp"

namespace wxl::runtime
{
    ObjectService* ObjectService::instance_ = nullptr;

    ObjectService::ObjectService(ObjectBinding& binding) : binding_(binding)
    {
        instance_ = this;

        api_.struct_size = sizeof(api_);

        api_.valid        = &ObjectService::ValidThunk;
        api_.guid         = &ObjectService::GuidThunk;
        api_.type         = &ObjectService::TypeThunk;
        api_.from_guid    = &ObjectService::FromGuidThunk;
        api_.local_player = &ObjectService::LocalPlayerThunk;

        api_.position = &ObjectService::PositionThunk;
        api_.facing   = &ObjectService::FacingThunk;
        api_.name     = &ObjectService::NameThunk;

        api_.unit_health     = &ObjectService::UnitHealthThunk;
        api_.unit_max_health = &ObjectService::UnitMaxHealthThunk;
        api_.unit_level      = &ObjectService::UnitLevelThunk;
        api_.unit_target     = &ObjectService::UnitTargetThunk;

        api_.enumerate = &ObjectService::EnumerateThunk;
        api_.positions = &ObjectService::PositionsThunk;
    }

    ObjectService::~ObjectService()
    {
        if (instance_ == this) instance_ = nullptr;
    }

    wxl_handle ObjectService::Track(void* object) { return handles_.Track(object); }

    void ObjectService::Untrack(void* object) { handles_.Untrack(object); }

    void ObjectService::Clear() { handles_.Clear(); }

    void* ObjectService::Object(wxl_handle handle)
    {
        return instance_ ? instance_->handles_.Resolve(handle) : nullptr;
    }

    void* ObjectService::Unit(wxl_handle handle)
    {
        void* object = Object(handle);
        if (!object) return nullptr;

        const wxl_object_type type = instance_->binding_.Type(object);
        if (type != WXL_OBJECT_UNIT && type != WXL_OBJECT_PLAYER) return nullptr;
        return object;
    }

    wxl_bool WXL_CALL ObjectService::ValidThunk(wxl_handle object)
    {
        return Object(object) ? WXL_TRUE : WXL_FALSE;
    }

    wxl_guid WXL_CALL ObjectService::GuidThunk(wxl_handle object)
    {
        void* native = Object(object);
        return native ? instance_->binding_.Guid(native) : WXL_GUID_NONE;
    }

    uint32_t WXL_CALL ObjectService::TypeThunk(wxl_handle object)
    {
        void* native = Object(object);
        return native ? static_cast<uint32_t>(instance_->binding_.Type(native))
                      : static_cast<uint32_t>(WXL_OBJECT_UNKNOWN);
    }

    wxl_handle WXL_CALL ObjectService::FromGuidThunk(wxl_guid guid)
    {
        if (!instance_ || guid == WXL_GUID_NONE) return WXL_HANDLE_NONE;

        wxl_handle found = WXL_HANDLE_NONE;
        instance_->handles_.ForEach(
            [&](wxl_handle handle, void* native)
            {
                if (found != WXL_HANDLE_NONE) return;
                if (instance_->binding_.Guid(native) == guid) found = handle;
            });
        return found;
    }

    wxl_handle WXL_CALL ObjectService::LocalPlayerThunk(void)
    {
        if (!instance_) return WXL_HANDLE_NONE;

        void* player = instance_->binding_.LocalPlayer();
        return player ? instance_->handles_.HandleOf(player) : WXL_HANDLE_NONE;
    }

    wxl_result WXL_CALL ObjectService::PositionThunk(wxl_handle object, wxl_vec3* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Object(object);
        if (!native) return WXL_E_NOT_FOUND;
        return instance_->binding_.Position(native, *out) ? WXL_OK : WXL_E_INVALID;
    }

    wxl_result WXL_CALL ObjectService::FacingThunk(wxl_handle object, float* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Object(object);
        if (!native) return WXL_E_NOT_FOUND;
        return instance_->binding_.Facing(native, *out) ? WXL_OK : WXL_E_INVALID;
    }

    wxl_result WXL_CALL ObjectService::NameThunk(wxl_handle object, wxl_str* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Object(object);
        if (!native) return WXL_E_NOT_FOUND;

        const char* text = nullptr;
        uint32_t    size = 0;
        if (!instance_->binding_.Name(native, text, size)) return WXL_E_INVALID;

        out->data = text;
        out->size = size;
        return WXL_OK;
    }

    wxl_result WXL_CALL ObjectService::UnitHealthThunk(wxl_handle unit, uint32_t* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Unit(unit);
        if (!native) return Object(unit) ? WXL_E_INVALID : WXL_E_NOT_FOUND;
        return instance_->binding_.UnitHealth(native, *out) ? WXL_OK : WXL_E_INVALID;
    }

    wxl_result WXL_CALL ObjectService::UnitMaxHealthThunk(wxl_handle unit, uint32_t* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Unit(unit);
        if (!native) return Object(unit) ? WXL_E_INVALID : WXL_E_NOT_FOUND;
        return instance_->binding_.UnitMaxHealth(native, *out) ? WXL_OK : WXL_E_INVALID;
    }

    wxl_result WXL_CALL ObjectService::UnitLevelThunk(wxl_handle unit, uint32_t* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Unit(unit);
        if (!native) return Object(unit) ? WXL_E_INVALID : WXL_E_NOT_FOUND;
        return instance_->binding_.UnitLevel(native, *out) ? WXL_OK : WXL_E_INVALID;
    }

    wxl_result WXL_CALL ObjectService::UnitTargetThunk(wxl_handle unit, wxl_handle* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Unit(unit);
        if (!native) return Object(unit) ? WXL_E_INVALID : WXL_E_NOT_FOUND;

        void* target = instance_->binding_.UnitTarget(native);
        *out         = target ? instance_->handles_.HandleOf(target) : WXL_HANDLE_NONE;
        return WXL_OK;
    }

    uint32_t WXL_CALL ObjectService::EnumerateThunk(uint32_t type, wxl_handle* out, uint32_t cap)
    {
        if (!instance_) return 0;

        uint32_t matched = 0;
        instance_->handles_.ForEach(
            [&](wxl_handle handle, void* native)
            {
                if (type != WXL_OBJECT_UNKNOWN &&
                    static_cast<uint32_t>(instance_->binding_.Type(native)) != type)
                    return;

                if (out && matched < cap) out[matched] = handle;
                ++matched;
            });
        return matched;
    }

    wxl_result WXL_CALL ObjectService::PositionsThunk(const wxl_handle* objects, uint32_t count,
                                                      wxl_vec3* out)
    {
        if (!objects || !out) return WXL_E_INVALID;
        if (!instance_) return WXL_E_UNSUPPORTED;

        for (uint32_t i = 0; i < count; ++i)
        {
            out[i] = wxl_vec3{0.0f, 0.0f, 0.0f};

            void* native = Object(objects[i]);
            if (native) instance_->binding_.Position(native, out[i]);
        }
        return WXL_OK;
    }
} // namespace wxl::runtime
