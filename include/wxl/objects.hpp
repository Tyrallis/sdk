// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// World objects as C++ objects.
//
// A handle plus accessors. Each read is one call through the object service and answers with an
// Expected: the value, or the wxl_result that stood in for it. Finders (Local, FromGuid, Target)
// answer with an object that may be null; reads answer with an Expected.
//
// For a per-frame pass over thousands of objects, prefer ObjectService::Positions over a loop of
// single reads.

#ifndef WXL_OBJECTS_HPP
#define WXL_OBJECTS_HPP

#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "wxl/sdk.hpp"
#include "wxl/services/object.h"

namespace wxl
{
    class Object;
    class Unit;
    class Player;
    class GameObject;

    template <class T>
    class ObjectView;

    /// The typed facade over wxl.object: finders, enumeration and the bulk reads.
    class ObjectService : public detail::Facade<ObjectService, wxl_object_api>
    {
    public:
        static constexpr const char* kName    = WXL_OBJECT_SERVICE;
        static constexpr uint32_t    kVersion = WXL_OBJECT_SERVICE_VERSION;

        /// The unit this client controls; null while out of the world or without the service.
        inline Player LocalPlayer() const;

        /// The object with that server identity; null when it is not in view.
        inline Object FromGuid(wxl_guid guid) const;

        /**
         * @brief Everything of a class currently in view, as a snapshot.
         *
         *     for (wxl::Unit unit : wxl::ObjectService().InView<wxl::Unit>()) ...
         */
        template <class T = Object>
        ObjectView<T> InView() const;

        /// Everything of one server type, or of every type for WXL_OBJECT_UNKNOWN.
        inline ObjectView<Object> InView(wxl_object_type type) const;

        /// Positions for many handles at once; a stale handle reads as the origin.
        inline wxl_result Positions(const wxl_handle* objects, uint32_t count, wxl_vec3* out) const;

        /// Positions for a whole view, in its order.
        template <class T>
        Expected<std::vector<wxl_vec3>> Positions(const ObjectView<T>& view) const;
    };

    /// Whether this runtime can read world objects at all.
    inline bool HasObjects() { return ObjectService::Available(); }

    /**
     * @brief Anything the client holds in the world.
     *
     * A value: copy it, compare it, keep it across frames, put it in a std::set or a
     * std::unordered_map. A handle whose subject is gone answers IsValid() with false, and every
     * read on it answers with WXL_E_NOT_FOUND, instead of reading whatever took its place.
     *
     * operator bool is IsValid(): `if (target)` asks whether the object is still there, which
     * costs one call. HasHandle() is the free question of whether there is a handle at all.
     */
    class Object
    {
    public:
        /// The server types this class stands for; WXL_OBJECT_UNKNOWN means every type.
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_UNKNOWN};

        /// Whether an object of that server type may be viewed as this class.
        static constexpr bool Accepts(wxl_object_type) { return true; }

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

        /// One getter through the service, with the three absences told apart.
        template <class V>
        Expected<V> Read(Getter<V> wxl_object_api::*getter) const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!api || !(api->*getter)) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            V                out{};
            const wxl_result result = (api->*getter)(handle_, &out);
            if (result != WXL_OK) return Unexpected{result};
            return out;
        }
    };

    /// Something alive: a creature, an NPC, a player.
    class Unit : public Object
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_UNIT, WXL_OBJECT_PLAYER};
        static constexpr bool            Accepts(wxl_object_type type)
        {
            return type == WXL_OBJECT_UNIT || type == WXL_OBJECT_PLAYER;
        }

        using Object::Object;

        /// A value of 0 is a dead unit; a missing value is a gone one.
        Expected<uint32_t> Health() const { return Read(&wxl_object_api::unit_health); }
        Expected<uint32_t> MaxHealth() const { return Read(&wxl_object_api::unit_max_health); }
        Expected<uint32_t> Level() const { return Read(&wxl_object_api::unit_level); }

        /// Health as a fraction in 0..1; 0 when the unit has no maximum to compare against.
        Expected<float> HealthFraction() const
        {
            const Expected<uint32_t> max = MaxHealth();
            if (!max) return Unexpected{max.Error()};
            const Expected<uint32_t> health = Health();
            if (!health) return Unexpected{health.Error()};
            return *max ? static_cast<float>(*health) / static_cast<float>(*max) : 0.0f;
        }

        /// What the unit targets. A null Unit as the value means it targets nothing.
        Expected<Unit> Target() const
        {
            Expected<wxl_handle> target = Read(&wxl_object_api::unit_target);
            if (!target) return Unexpected{target.Error()};
            return Unit(*target);
        }
    };

    /// A player-controlled unit.
    class Player : public Unit
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_PLAYER};
        static constexpr bool Accepts(wxl_object_type type) { return type == WXL_OBJECT_PLAYER; }

        using Unit::Unit;

        /// The unit this client controls; null while out of the world or without the service.
        static Player Local() { return ObjectService().LocalPlayer(); }
    };

    /// A placed, interactable thing: a door, a chest, a node.
    class GameObject : public Object
    {
    public:
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_GAMEOBJECT};
        static constexpr bool Accepts(wxl_object_type type) { return type == WXL_OBJECT_GAMEOBJECT; }

        using Object::Object;
    };

    /**
     * @brief A snapshot of the objects of one class that were in view when it was taken.
     *
     * Holds handles, not objects: an element is built on access, so an object that left between
     * the snapshot and the visit reads as gone rather than as garbage. Iterates in a range-for and
     * with <algorithm>; the iterator yields T by value.
     */
    template <class T>
    class ObjectView
    {
    public:
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = void;
            using reference         = T;

            iterator() = default;
            explicit iterator(const wxl_handle* at) : at_(at) {}

            T         operator*() const { return T(*at_); }
            iterator& operator++()
            {
                ++at_;
                return *this;
            }
            iterator operator++(int)
            {
                iterator before = *this;
                ++at_;
                return before;
            }
            bool operator==(const iterator& other) const { return at_ == other.at_; }
            bool operator!=(const iterator& other) const { return at_ != other.at_; }

        private:
            const wxl_handle* at_ = nullptr;
        };

        using value_type     = T;
        using const_iterator = iterator;

        ObjectView() = default;
        explicit ObjectView(std::vector<wxl_handle> handles) : handles_(std::move(handles)) {}

        iterator begin() const { return iterator(handles_.data()); }
        iterator end() const { return iterator(handles_.data() + handles_.size()); }

        size_t size() const { return handles_.size(); }
        bool   empty() const { return handles_.empty(); }
        T      operator[](size_t index) const { return T(handles_[index]); }

        /// The handles themselves, for the bulk calls.
        const std::vector<wxl_handle>& Handles() const { return handles_; }

        /// The same objects, materialised.
        std::vector<T> ToVector() const { return std::vector<T>(begin(), end()); }

    private:
        std::vector<wxl_handle> handles_;
    };

    // --- ObjectService, defined once Object and its classes exist ------------------------

    inline Player ObjectService::LocalPlayer() const
    {
        const wxl_object_api* api = Raw();
        return Player(api ? api->local_player() : WXL_HANDLE_NONE);
    }

    inline Object ObjectService::FromGuid(wxl_guid guid) const
    {
        const wxl_object_api* api = Raw();
        return Object(api && guid != WXL_GUID_NONE ? api->from_guid(guid) : WXL_HANDLE_NONE);
    }

    namespace detail
    {
        /// Appends every handle of one server type to @p into.
        inline void EnumerateInto(const wxl_object_api* api, wxl_object_type type,
                                  std::vector<wxl_handle>& into)
        {
            const uint32_t count = api->enumerate(type, nullptr, 0);
            if (!count) return;
            const size_t first = into.size();
            into.resize(first + count);
            const uint32_t written = api->enumerate(type, into.data() + first, count);
            into.resize(first + (written < count ? written : count));
        }
    } // namespace detail

    template <class T>
    ObjectView<T> ObjectService::InView() const
    {
        const wxl_object_api* api = Raw();
        if (!api) return ObjectView<T>();
        std::vector<wxl_handle> handles;
        for (wxl_object_type kind : T::kKinds) detail::EnumerateInto(api, kind, handles);
        return ObjectView<T>(std::move(handles));
    }

    inline ObjectView<Object> ObjectService::InView(wxl_object_type type) const
    {
        const wxl_object_api* api = Raw();
        if (!api) return ObjectView<Object>();
        std::vector<wxl_handle> handles;
        detail::EnumerateInto(api, type, handles);
        return ObjectView<Object>(std::move(handles));
    }

    inline wxl_result ObjectService::Positions(const wxl_handle* objects, uint32_t count,
                                               wxl_vec3* out) const
    {
        const wxl_object_api* api = Raw();
        if (!api) return WXL_E_UNSUPPORTED;
        return api->positions(objects, count, out);
    }

    template <class T>
    Expected<std::vector<wxl_vec3>> ObjectService::Positions(const ObjectView<T>& view) const
    {
        std::vector<wxl_vec3> out(view.size());
        if (view.empty()) return out;
        const wxl_result result =
            Positions(view.Handles().data(), static_cast<uint32_t>(view.size()), out.data());
        if (result != WXL_OK) return Unexpected{result};
        return out;
    }
} // namespace wxl

/// Objects hash by handle, so they key an unordered_map or an unordered_set.
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

    template <>
    struct hash<wxl::Unit> : hash<wxl::Object>
    {
    };

    template <>
    struct hash<wxl::Player> : hash<wxl::Object>
    {
    };

    template <>
    struct hash<wxl::GameObject> : hash<wxl::Object>
    {
    };
} // namespace std

#endif // WXL_OBJECTS_HPP
