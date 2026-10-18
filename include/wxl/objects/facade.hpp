// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 WarcraftXL
//
// The wxl.object facade, declared: finders, enumeration and the bulk reads.

#ifndef WXL_OBJECTS_FACADE_HPP
#define WXL_OBJECTS_FACADE_HPP

// Declares the facade only. Its members are defined in objects/service.hpp once every class they
// return exists; wxl/objects.hpp includes everything in the right order. A per-type header
// includes this one for ObjectService::Raw(), which is all the classes need.

#include <cstdint>
#include <vector>

#include "wxl/sdk.hpp"
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
    };

    /// Whether this runtime can read world objects at all.
    inline bool HasObjects() { return ObjectService::Available(); }
} // namespace wxl

#endif // WXL_OBJECTS_FACADE_HPP
