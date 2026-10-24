// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The wxl.object facade, defined: what objects/facade.hpp declared, once every class exists.

#ifndef WXL_OBJECTS_SERVICE_HPP
#define WXL_OBJECTS_SERVICE_HPP

#include "wxl/objects/facade.hpp"
#include "wxl/objects/object.hpp"
#include "wxl/objects/player.hpp"
#include "wxl/objects/view.hpp"

namespace wxl
{
    inline Player ObjectService::LocalPlayer() const { return Player::Local(); }

    inline Object ObjectService::FromGuid(wxl_guid guid) const { return Object::Resolve(guid); }

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

    template <class V, class T>
    std::vector<Expected<T>> ObjectService::Fields(const ObjectView<V>& view,
                                                   const field::Field<T>& which, uint32_t element) const
    {
        std::vector<Expected<T>> out;
        out.reserve(view.size());
        const wxl_object_api* api = Raw();
        if (!detail::Carries(api, &wxl_object_api::read_field_many) || element >= which.Elements())
        {
            const wxl_result why = element >= which.Elements() && api ? WXL_E_INVALID : WXL_E_UNSUPPORTED;
            for (size_t i = 0; i < view.size(); ++i) out.push_back(Unexpected{why});
            return out;
        }
        const uint32_t           count  = static_cast<uint32_t>(view.size());
        const uint32_t           dwords = which.Dwords();
        std::vector<uint32_t>    values(size_t(count) * dwords);
        std::vector<wxl_result>  results(count, WXL_OK);
        if (count)
            api->read_field_many(view.Handles().data(), count, which.IndexOf(element), dwords,
                                 values.data(), results.data());
        for (uint32_t i = 0; i < count; ++i)
        {
            // An object of another kind holds something else at that index.
            wxl_result result = results[i];
            if (result == WXL_OK && !field::Carries(api->type(view.Handles()[i]), which.kind))
                result = WXL_E_INVALID;
            if (result == WXL_OK) out.push_back(field::Decode<T>(values.data() + size_t(i) * dwords));
            else out.push_back(Unexpected{result});
        }
        return out;
    }
} // namespace wxl

#endif // WXL_OBJECTS_SERVICE_HPP
