// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.m2file: reshaping a model file the client has parsed.

#ifndef WXL_SERVICE_M2FILE_HPP
#define WXL_SERVICE_M2FILE_HPP

#include <cstdint>

#include "wxl/sdk.hpp"
#include "wxl/services/m2file.h"
#include "wxl/services/rows.hpp"

namespace wxl::api::facade
{
    template <>
    struct Methods<wxl_m2file_api>
    {
#define WXL_API_TABLE "wxl/services/m2file.def"
#include "wxl/services/methods.inc"
    };
} // namespace wxl::api::facade

namespace wxl
{
    /**
     * @brief The typed facade over wxl.m2file, used from the handlers of the model loader's
     *        events (OnModelParsed, OnSkinProfileParsed, OnModelSequenceLoad).
     *
     *     void OnModelParsed(wxl_str path, void* file, void* header, uint32_t) override
     *     {
     *         auto* textures = static_cast<uint8_t*>(header) + kTexturesArray; // an M2Array
     *         void* more     = wxl::M2FileService::AllocNear(textures, 4 * 16);
     *         wxl::M2FileService::SetArray(textures, more, 4);
     *     }
     *
     *   AllocNear(anchor, size)       zeroed memory an array at `anchor` can point at
     *   AnimateBones(file)            the shadow pass animates every bone of the file's models
     *   ArrayData(array)              where an array's elements are, as this runtime keeps them
     *   SetArray(array, data, count)  points an array at elements, as this runtime keeps them
     *   Elements<T>(array)            ArrayData, typed, with the count
     *
     * A missing service answers null, nothing, or WXL_E_UNSUPPORTED.
     */
    class M2FileService : public detail::Facade<M2FileService, wxl_m2file_api>,
                          public api::facade::Methods<wxl_m2file_api>
    {
    public:
        static constexpr const char* kName    = WXL_M2FILE_SERVICE;
        static constexpr uint32_t    kVersion = WXL_M2FILE_SERVICE_VERSION;

        /// The elements of a parsed array and how many there are: its first word.
        template <class T>
        struct Span
        {
            T*       data  = nullptr;
            uint32_t count = 0;

            T*       begin() const { return data; }
            T*       end() const { return data ? data + count : data; }
            uint32_t size() const { return data ? count : 0; }
        };

        /// The parsed array at @p array as elements of @p T; empty without the service.
        template <class T>
        static Span<T> Elements(const void* array)
        {
            Span<T> out;
            if (!array) return out;
            out.data  = static_cast<T*>(ArrayData(array));
            out.count = *static_cast<const uint32_t*>(array);
            return out;
        }
    };
} // namespace wxl

#endif // WXL_SERVICE_M2FILE_HPP
