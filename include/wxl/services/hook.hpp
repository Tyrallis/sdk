// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facades over wxl.hook and wxl.hook.raw: detours by name and by address.

#ifndef WXL_SERVICE_HOOK_HPP
#define WXL_SERVICE_HOOK_HPP

#include <string>
#include <type_traits>
#include <vector>

#include "wxl/sdk.hpp"
#include "wxl/services/hook.h"
#include "wxl/services/rows.hpp"

namespace wxl::api::facade
{
    template <>
    struct Methods<wxl_hook_api>
    {
#define WXL_API_TABLE "wxl/services/hook.def"
#include "wxl/services/methods.inc"
    };

    template <>
    struct Methods<wxl_hook_raw_api>
    {
#define WXL_API_TABLE "wxl/services/hook_raw.def"
#include "wxl/services/methods.inc"
    };
} // namespace wxl::api::facade

namespace wxl
{
    /**
     * @brief An attached detour, detached when this goes out of scope.
     *
     * Move-only. Release() leaves the detour attached for code that detaches it by hand. The
     * chain closes over a detached detour; links behind it keep working.
     */
    class Hook
    {
    public:
        using Detach = wxl_result(WXL_CALL*)(wxl_hook);

        Hook() = default;
        Hook(Detach detach, wxl_hook token) : detach_(detach), token_(token) {}

        Hook(Hook&& other) noexcept : detach_(other.detach_), token_(other.Release()) {}
        Hook& operator=(Hook&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                detach_ = other.detach_;
                token_  = other.Release();
            }
            return *this;
        }

        Hook(const Hook&)            = delete;
        Hook& operator=(const Hook&) = delete;

        ~Hook() { Reset(); }

        explicit operator bool() const { return token_ != WXL_HOOK_NONE; }
        wxl_hook Get() const { return token_; }

        /// Gives up ownership without detaching.
        wxl_hook Release()
        {
            const wxl_hook token = token_;
            token_               = WXL_HOOK_NONE;
            return token;
        }

        /// Detaches now. Safe to call more than once.
        void Reset()
        {
            if (token_ == WXL_HOOK_NONE) return;
            if (detach_) detach_(token_);
            token_ = WXL_HOOK_NONE;
        }

    private:
        Detach   detach_ = nullptr;
        wxl_hook token_  = WXL_HOOK_NONE;
    };

    /// A hook point a runtime names, copied out of the runtime's storage.
    struct HookPoint
    {
        std::string name;
        std::string signature;
    };

    /**
     * @brief The typed facade over wxl.hook: detours on points the runtime names.
     *
     * An empty Hook means the point is not named by this runtime, or the service is absent.
     * HasPoint(name), whether this runtime names a point, is a row of hook.def.
     */
    class HookService : public detail::Facade<HookService, wxl_hook_api>,
                        public api::facade::Methods<wxl_hook_api>
    {
    public:
        static constexpr const char* kName    = WXL_HOOK_SERVICE;
        static constexpr uint32_t    kVersion = WXL_HOOK_SERVICE_VERSION;

        /// The hook points this runtime names.
        std::vector<HookPoint> Points() const
        {
            std::vector<HookPoint> points;
            const wxl_hook_api*    api = Raw();
            if (!api) return points;

            const uint32_t count = api->points(nullptr, 0);
            if (!count) return points;

            std::vector<wxl_hook_point> raw(count);
            for (wxl_hook_point& point : raw) point.struct_size = sizeof(wxl_hook_point);
            const uint32_t written = api->points(raw.data(), count);
            points.reserve(written < count ? written : count);
            for (uint32_t i = 0; i < written && i < count; ++i)
                points.push_back(HookPoint{std::string(View(raw[i].name)), std::string(View(raw[i].signature))});
            return points;
        }

        /// Attaches a raw detour; @p original receives the next link in the chain.
        Hook Attach(const char* name, void* detour, void** original,
                    int32_t priority = WXL_HOOK_PRIORITY_DEFAULT) const
        {
            const wxl_hook_api* api = Raw();
            if (!api || !name || !detour) return Hook();
            return Hook(api->detach, api->attach(name, detour, original, priority));
        }

        /**
         * @brief Attaches a detour with its signature kept.
         *
         *     using Fn = int(WXL_CALL*)(int);
         *     static Fn s_original = nullptr;
         *     wxl::Hook hook = hooks.Attach("Thing", &MyDetour, &s_original);
         */
        template <class Fn, std::enable_if_t<std::is_function_v<Fn>, int> = 0>
        Hook Attach(const char* name, Fn* detour, Fn** original,
                    int32_t priority = WXL_HOOK_PRIORITY_DEFAULT) const
        {
            return Attach(name, reinterpret_cast<void*>(detour), reinterpret_cast<void**>(original),
                          priority);
        }
    };

    /**
     * @brief The typed facade over wxl.hook.raw: detours at an address.
     *
     * Published only by a runtime that loads into a binary whose addresses mean something. A mod
     * that uses it is pinned to one binary and says so in its manifest. ImageBase(), the image
     * the addresses are relative to, and ImageBuild(), the build it reports, are rows of
     * hook_raw.def: each an Expected.
     */
    class RawHookService : public detail::Facade<RawHookService, wxl_hook_raw_api>,
                           public api::facade::Methods<wxl_hook_raw_api>
    {
    public:
        static constexpr const char* kName    = WXL_HOOK_RAW_SERVICE;
        static constexpr uint32_t    kVersion = WXL_HOOK_RAW_SERVICE_VERSION;

        /// Attaches a raw detour at an address; @p name is a label for the log.
        Hook AttachAt(const char* name, uintptr_t target, void* detour, void** original,
                      int32_t priority = WXL_HOOK_PRIORITY_DEFAULT) const
        {
            const wxl_hook_raw_api* api = Raw();
            if (!api || !detour) return Hook();
            return Hook(api->detach, api->attach_at(name, target, detour, original, priority));
        }

        /// Attaches a detour at an address with its signature kept.
        template <class Fn, std::enable_if_t<std::is_function_v<Fn>, int> = 0>
        Hook AttachAt(const char* name, uintptr_t target, Fn* detour, Fn** original,
                      int32_t priority = WXL_HOOK_PRIORITY_DEFAULT) const
        {
            return AttachAt(name, target, reinterpret_cast<void*>(detour),
                            reinterpret_cast<void**>(original), priority);
        }
    };
} // namespace wxl

#endif // WXL_SERVICE_HOOK_HPP
