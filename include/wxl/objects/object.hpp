// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::Object: anything the client holds in the world, as a value over a handle.

#ifndef WXL_OBJECTS_OBJECT_HPP
#define WXL_OBJECTS_OBJECT_HPP

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>

#include "wxl/objects/facade.hpp"

namespace wxl
{
    /**
     * @brief Anything the client holds in the world.
     *
     * A value: copy it, compare it, keep it across frames, put it in a std::set or a
     * std::unordered_map. A handle whose subject is gone answers IsValid() with false, and every
     * read on it answers with WXL_E_NOT_FOUND, instead of reading whatever took its place.
     *
     * operator bool is IsValid(): `if (target)` asks whether the object is still there, which
     * costs one call. HasHandle() is the free question of whether there is a handle at all.
     *
     * The client's own kinds are the derived classes, one per wxl_object_type: Item, Container,
     * Unit, Player, GameObject, DynamicObject and Corpse. As<T>() views an Object as one of them
     * when its server type allows it; `wxl::Unit(object)` is the same view unchecked, for code that
     * already knows the kind.
     *
     * A handle names one visibility episode (wxl/services/object.h): an object that leaves view
     * and comes back is another Object with the same Guid(). Event handlers are handed their
     * objects as this class.
     */
    class Object
    {
    public:
        /// The server types this class stands for; WXL_OBJECT_UNKNOWN means every type.
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_UNKNOWN};

        /// Whether an object of that server type -- a wxl_object_type, or the `type` an object
        /// event carries -- may be viewed as this class.
        static constexpr bool Accepts(uint32_t) { return true; }

        constexpr Object() = default;
        explicit constexpr Object(wxl_handle handle) : handle_(handle) {}

        constexpr wxl_handle Handle() const { return handle_; }
        constexpr bool       HasHandle() const { return handle_ != WXL_HANDLE_NONE; }

        /// Whether the handle still resolves. One call; false without the service.
        bool IsValid() const
        {
            const wxl_object_api* api = ObjectService::Raw();
            return api && HasHandle() && api->valid(handle_) == WXL_TRUE;
        }

        explicit operator bool() const { return IsValid(); }

        friend constexpr bool operator==(const Object& a, const Object& b) { return a.handle_ == b.handle_; }
        friend constexpr bool operator!=(const Object& a, const Object& b) { return a.handle_ != b.handle_; }
        friend constexpr bool operator<(const Object& a, const Object& b) { return a.handle_ < b.handle_; }
        friend constexpr bool operator>(const Object& a, const Object& b) { return a.handle_ > b.handle_; }
        friend constexpr bool operator<=(const Object& a, const Object& b) { return a.handle_ <= b.handle_; }
        friend constexpr bool operator>=(const Object& a, const Object& b) { return a.handle_ >= b.handle_; }

        /// The object with that server identity, or a null Object when it is not in view.
        static Object Resolve(wxl_guid guid)
        {
            const wxl_object_api* api = ObjectService::Raw();
            return Object(api && guid != WXL_GUID_NONE ? api->from_guid(guid) : WXL_HANDLE_NONE);
        }

        /// The server identity; WXL_GUID_NONE as a value means the server does not own it.
        Expected<wxl_guid> Guid() const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            const wxl_guid guid = api->guid(handle_);
            if (guid == WXL_GUID_NONE && api->valid(handle_) != WXL_TRUE)
                return Unexpected{WXL_E_NOT_FOUND};
            return guid;
        }

        Expected<wxl_object_type> Type() const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            const wxl_object_type type = static_cast<wxl_object_type>(api->type(handle_));
            if (type == WXL_OBJECT_UNKNOWN && api->valid(handle_) != WXL_TRUE)
                return Unexpected{WXL_E_NOT_FOUND};
            return type;
        }

        Expected<wxl_vec3> Position() const { return Read(&wxl_object_api::position); }

        /// Facing in radians.
        Expected<float> Facing() const { return Read(&wxl_object_api::facing); }

        /// The display name, copied out of the runtime's storage.
        Expected<std::string> Name() const
        {
            Expected<wxl_str> name = Read(&wxl_object_api::name);
            if (!name) return Unexpected{name.Error()};
            return std::string(View(*name));
        }

        /// The server's template id: a creature, item or game-object entry; 0 for a player.
        Expected<uint32_t> Entry() const { return Read(&wxl_object_api::entry); }

        Expected<float> Scale() const { return Read(&wxl_object_api::scale); }

        /**
         * @brief This object as a narrower class, when its server type allows it.
         *
         *     if (std::optional<wxl::Unit> unit = object.As<wxl::Unit>()) ...
         */
        template <class T>
        std::optional<T> As() const
        {
            const Expected<wxl_object_type> type = Type();
            if (!type || !T::Accepts(*type)) return std::nullopt;
            return T(handle_);
        }

    protected:
        wxl_handle handle_ = WXL_HANDLE_NONE;

        /// A getter of the object service, as a member of the table.
        template <class V>
        using Getter = wxl_result(WXL_CALL*)(wxl_handle, V*);

        /// An indexed getter: a slot, a power, a bag position.
        template <class V>
        using GetterAt = wxl_result(WXL_CALL*)(wxl_handle, uint32_t, V*);

        /// One getter through the service, with the three absences told apart.
        template <class V>
        Expected<V> Read(Getter<V> wxl_object_api::*getter) const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!detail::Carries(api, getter)) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            V                out{};
            const wxl_result result = (api->*getter)(handle_, &out);
            if (result != WXL_OK) return Unexpected{result};
            return out;
        }

        template <class V>
        Expected<V> ReadAt(GetterAt<V> wxl_object_api::*getter, uint32_t index) const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!detail::Carries(api, getter)) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            V                out{};
            const wxl_result result = (api->*getter)(handle_, index, &out);
            if (result != WXL_OK) return Unexpected{result};
            return out;
        }
    };

    // What a handler is handed must cost what the handle costs: one register, copied freely.
    static_assert(sizeof(Object) == sizeof(wxl_handle) && std::is_trivially_copyable_v<Object>,
                  "wxl::Object is a handle and nothing more");
} // namespace wxl

namespace std
{
    template <>
    struct hash<wxl::Object>
    {
        size_t operator()(const wxl::Object& object) const noexcept
        {
            return hash<wxl_handle>()(object.Handle());
        }
    };
} // namespace std

#endif // WXL_OBJECTS_OBJECT_HPP
