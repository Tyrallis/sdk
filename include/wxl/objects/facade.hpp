// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The wxl.object facade, declared: finders, enumeration, the bulk reads, and FieldWatch.

#ifndef WXL_OBJECTS_FACADE_HPP
#define WXL_OBJECTS_FACADE_HPP

// Declares the facade only. Its members are defined in objects/service.hpp once every class they
// return exists; wxl/objects.hpp includes everything in the right order. A per-type header
// includes this one for ObjectService::Raw(), which is all the classes need. Nothing here reaches
// sdk.hpp, which includes wxl/objects/object.hpp: wxl::Object is what a handler is handed.

#include <cstdint>
#include <vector>

#include "wxl/base.hpp"
#include "wxl/expected.hpp"
#include "wxl/objects/fields.hpp"
#include "wxl/services/object.h"

namespace wxl
{
    class Object;
    class Player;

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

        /**
         * @brief One element of a field for a whole view, in its order, in one call.
         *
         *     auto health = wxl::ObjectService().Fields(units, wxl::field::Unit::Health);
         *     // health[i] is units[i]'s, or why it has none
         */
        template <class V, class T>
        std::vector<Expected<T>> Fields(const ObjectView<V>& view, const field::Field<T>& which,
                                        uint32_t element = 0) const;

        /**
         * @brief Asks for ObjectFieldChanged whenever a field changes on objects of a type
         *        (wxl_object_api::watch_field): the field's own kind unless another is named.
         *
         *     wxl::ObjectService().Watch(wxl::field::Unit::Health);                    // units and players
         *     wxl::ObjectService().Watch(wxl::field::Unit::Health, WXL_OBJECT_PLAYER); // players only
         *
         * The watch is this mod's: its handlers receive the field's changes, no other mod's do
         * unless they watch it too, and it lasts until Unwatch or until the mod goes. A
         * wxl::FieldWatch holds one for as long as it lives. WXL_E_UNSUPPORTED for a field the
         * client does not see change (the watch column of update_fields.def) or a runtime older
         * than abi 0.1.9.
         */
        template <class T>
        wxl_result Watch(const field::Field<T>& which) const
        {
            return WatchRaw(which.kind, which.index, which.size);
        }

        template <class T>
        wxl_result Watch(const field::Field<T>& which, wxl_object_type type) const
        {
            return WatchRaw(type, which.index, which.size);
        }

        /// The same by dword index.
        wxl_result WatchRaw(wxl_object_type type, uint32_t first, uint32_t count) const
        {
            const wxl_object_api* api = Raw();
            if (!detail::Carries(api, &wxl_object_api::watch_field)) return WXL_E_UNSUPPORTED;
            return api->watch_field(type, first, count);
        }

        /// Gives back one Watch of the field on that type (wxl_object_api::unwatch_field).
        /// WXL_E_UNSUPPORTED on a runtime older than abi 0.1.10, where a watch lasts as long as
        /// the runtime.
        template <class T>
        wxl_result Unwatch(const field::Field<T>& which) const
        {
            return UnwatchRaw(which.kind, which.index, which.size);
        }

        template <class T>
        wxl_result Unwatch(const field::Field<T>& which, wxl_object_type type) const
        {
            return UnwatchRaw(type, which.index, which.size);
        }

        /// The same by dword index.
        wxl_result UnwatchRaw(wxl_object_type type, uint32_t first, uint32_t count) const
        {
            const wxl_object_api* api = Raw();
            if (!detail::Carries(api, &wxl_object_api::unwatch_field)) return WXL_E_UNSUPPORTED;
            return api->unwatch_field(type, first, count);
        }
    };

    /**
     * @brief A watch of a field, given back when it goes.
     *
     *     class Tracker : public wxl::ObjectScript
     *     {
     *         wxl::FieldWatch health_{wxl::field::Unit::Health};  // from the mod's start to its end
     *         ...
     *     };
     *
     * Holds one ObjectService::Watch from its construction and calls Unwatch in its destructor,
     * or at Reset; Release lets it go without that. Two FieldWatch of the same field are two watches: the changes keep coming
     * until both are gone. Movable, not copyable. Result() says whether the watch was taken:
     * WXL_E_UNSUPPORTED for a field the client does not see change.
     */
    class FieldWatch
    {
    public:
        FieldWatch() = default;

        template <class T>
        explicit FieldWatch(const field::Field<T>& which) : FieldWatch(which.kind, which.index, which.size)
        {
        }

        template <class T>
        FieldWatch(const field::Field<T>& which, wxl_object_type type) : FieldWatch(type, which.index, which.size)
        {
        }

        /// The same by dword index.
        FieldWatch(wxl_object_type type, uint32_t first, uint32_t count)
            : type_(type), first_(first), count_(count), result_(ObjectService().WatchRaw(type, first, count))
        {
        }

        ~FieldWatch() { Reset(); }

        FieldWatch(const FieldWatch&)            = delete;
        FieldWatch& operator=(const FieldWatch&) = delete;

        FieldWatch(FieldWatch&& other) noexcept
            : type_(other.type_), first_(other.first_), count_(other.count_), result_(other.result_)
        {
            other.result_ = WXL_E_NOT_FOUND;
        }

        FieldWatch& operator=(FieldWatch&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                type_         = other.type_;
                first_        = other.first_;
                count_        = other.count_;
                result_       = other.result_;
                other.result_ = WXL_E_NOT_FOUND;
            }
            return *this;
        }

        /// Whether it holds a watch.
        explicit operator bool() const { return result_ == WXL_OK; }

        /// What watch_field answered; WXL_E_NOT_FOUND for an empty FieldWatch.
        wxl_result Result() const { return result_; }

        /// Gives the watch back now; the FieldWatch is empty after it. Safe to repeat.
        void Reset()
        {
            if (result_ == WXL_OK) ObjectService().UnwatchRaw(type_, first_, count_);
            result_ = WXL_E_NOT_FOUND;
        }

        /// Lets go of the watch without giving it back: it lasts until an Unwatch by hand or
        /// until the mod goes. The FieldWatch is empty after it.
        void Release() { result_ = WXL_E_NOT_FOUND; }

    private:
        wxl_object_type type_   = WXL_OBJECT_UNKNOWN;
        uint32_t        first_  = 0;
        uint32_t        count_  = 0;
        wxl_result      result_ = WXL_E_NOT_FOUND;
    };

    /// Whether this runtime can read world objects at all.
    inline bool HasObjects() { return ObjectService::Available(); }
} // namespace wxl

#endif // WXL_OBJECTS_FACADE_HPP
