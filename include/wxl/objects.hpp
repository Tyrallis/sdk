// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// World objects as C++ objects.
//
// A handle plus inline accessors. Each call below is one call through the object service, so the
// ergonomics are free and the portability is real: the runtime decides how a unit's health is
// actually found.
//
// For a per-frame pass over thousands of objects, prefer the bulk calls on wxl_object_api over a
// loop of these.

#ifndef WXL_OBJECTS_HPP
#define WXL_OBJECTS_HPP

#include <string>
#include <vector>

#include "wxl/sdk.hpp"
#include "wxl/services/object.h"

namespace wxl
{
    namespace detail
    {
        inline const wxl_object_api* ObjectApi()
        {
            static Service<wxl_object_api> service(WXL_OBJECT_SERVICE, WXL_OBJECT_SERVICE_VERSION);
            return service.Get();
        }
    } // namespace detail

    /// Whether this runtime can read world objects at all.
    inline bool HasObjects() { return detail::ObjectApi() != nullptr; }

    /**
     * @brief Anything the client holds in the world.
     *
     * Safe to copy and to keep across frames: a handle whose subject is gone answers IsValid()
     * with false instead of reading whatever took its place. Every accessor on an invalid handle
     * returns a zero value rather than failing.
     */
    class Object
    {
    public:
        constexpr Object() = default;
        explicit constexpr Object(wxl_handle handle) : handle_(handle) {}

        constexpr wxl_handle Handle() const { return handle_; }
        constexpr bool       operator==(const Object& other) const { return handle_ == other.handle_; }
        constexpr bool       operator!=(const Object& other) const { return handle_ != other.handle_; }

        bool IsValid() const
        {
            const wxl_object_api* api = detail::ObjectApi();
            return api && handle_ != WXL_HANDLE_NONE && api->valid(handle_) == WXL_TRUE;
        }

        explicit operator bool() const { return IsValid(); }

        wxl_guid Guid() const
        {
            const wxl_object_api* api = detail::ObjectApi();
            return api ? api->guid(handle_) : WXL_GUID_NONE;
        }

        wxl_object_type Type() const
        {
            const wxl_object_api* api = detail::ObjectApi();
            return api ? static_cast<wxl_object_type>(api->type(handle_)) : WXL_OBJECT_UNKNOWN;
        }

        wxl_vec3 Position() const
        {
            wxl_vec3              out = {0.0f, 0.0f, 0.0f};
            const wxl_object_api* api = detail::ObjectApi();
            if (api) api->position(handle_, &out);
            return out;
        }

        /// Facing in radians.
        float Facing() const
        {
            float                 out = 0.0f;
            const wxl_object_api* api = detail::ObjectApi();
            if (api) api->facing(handle_, &out);
            return out;
        }

        /// Copied out: the runtime's own string is only valid until its next call.
        std::string Name() const
        {
            wxl_str               out = {nullptr, 0};
            const wxl_object_api* api = detail::ObjectApi();
            if (api && api->name(handle_, &out) == WXL_OK && out.data)
                return std::string(out.data, out.size);
            return std::string();
        }

    protected:
        wxl_handle handle_ = WXL_HANDLE_NONE;

        uint32_t ReadU32(wxl_result (WXL_CALL* getter)(wxl_handle, uint32_t*)) const
        {
            uint32_t out = 0;
            if (detail::ObjectApi() && getter) getter(handle_, &out);
            return out;
        }
    };

    /// Something alive: a creature, an NPC, a player.
    class Unit : public Object
    {
    public:
        using Object::Object;

        uint32_t Health() const
        {
            const wxl_object_api* api = detail::ObjectApi();
            return api ? ReadU32(api->unit_health) : 0;
        }

        uint32_t MaxHealth() const
        {
            const wxl_object_api* api = detail::ObjectApi();
            return api ? ReadU32(api->unit_max_health) : 0;
        }

        uint32_t Level() const
        {
            const wxl_object_api* api = detail::ObjectApi();
            return api ? ReadU32(api->unit_level) : 0;
        }

        /// Health as a fraction in 0..1; zero when the unit has no maximum to compare against.
        float HealthFraction() const
        {
            const uint32_t max = MaxHealth();
            return max ? static_cast<float>(Health()) / static_cast<float>(max) : 0.0f;
        }

        Unit Target() const
        {
            wxl_handle            out = WXL_HANDLE_NONE;
            const wxl_object_api* api = detail::ObjectApi();
            if (api) api->unit_target(handle_, &out);
            return Unit(out);
        }
    };

    /// A player-controlled unit.
    class Player : public Unit
    {
    public:
        using Unit::Unit;

        /// The unit this client controls, invalid while out of the world.
        static Player Local()
        {
            const wxl_object_api* api = detail::ObjectApi();
            return Player(api ? api->local_player() : WXL_HANDLE_NONE);
        }
    };

    /// A placed, interactable thing: a door, a chest, a node.
    class GameObject : public Object
    {
    public:
        using Object::Object;
    };

    /**
     * @brief Everything of a type currently in view.
     * @param type  WXL_OBJECT_UNKNOWN for every object.
     */
    template <class T = Object>
    std::vector<T> ObjectsInView(wxl_object_type type)
    {
        std::vector<T>        result;
        const wxl_object_api* api = detail::ObjectApi();
        if (!api) return result;

        const uint32_t count = api->enumerate(type, nullptr, 0);
        if (!count) return result;

        std::vector<wxl_handle> handles(count);
        const uint32_t          written = api->enumerate(type, handles.data(), count);

        result.reserve(written < count ? written : count);
        for (uint32_t i = 0; i < written && i < count; ++i) result.push_back(T(handles[i]));
        return result;
    }
} // namespace wxl

#endif // WXL_OBJECTS_HPP
