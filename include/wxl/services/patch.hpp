// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.patch: bytes written into the client's image, and put back.

#ifndef WXL_SERVICE_PATCH_HPP
#define WXL_SERVICE_PATCH_HPP

#include <cstddef>
#include <cstring>
#include <vector>

#include "wxl/sdk.hpp"
#include "wxl/services/patch.h"

namespace wxl
{
    /**
     * @brief An applied patch, restored when this goes out of scope.
     *
     * Move-only. Release() leaves the bytes in place for code that restores them by hand. Several
     * patches are restored in reverse order of apply: declare them in apply order as members, or
     * hold them in a PatchSet.
     */
    class Patch
    {
    public:
        Patch() = default;
        Patch(const wxl_patch_api* api, wxl_patch token) : api_(api), token_(token) {}

        Patch(Patch&& other) noexcept : api_(other.api_), token_(other.Release()) {}
        Patch& operator=(Patch&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                api_   = other.api_;
                token_ = other.Release();
            }
            return *this;
        }

        Patch(const Patch&)            = delete;
        Patch& operator=(const Patch&) = delete;

        ~Patch() { Reset(); }

        explicit operator bool() const { return token_ != WXL_PATCH_NONE; }
        wxl_patch Get() const { return token_; }

        /// Gives up ownership without restoring.
        wxl_patch Release()
        {
            const wxl_patch token = token_;
            token_                = WXL_PATCH_NONE;
            return token;
        }

        /// Restores now. Safe to call more than once.
        void Reset()
        {
            if (token_ == WXL_PATCH_NONE) return;
            if (api_) api_->restore(token_);
            token_ = WXL_PATCH_NONE;
        }

    private:
        const wxl_patch_api* api_   = nullptr;
        wxl_patch            token_ = WXL_PATCH_NONE;
    };

    /**
     * @brief The typed facade over wxl.patch.
     *
     * An empty Patch means the write was refused, and the runtime's log says why. Every call
     * answers its absence value while the runtime does not publish the service.
     */
    class PatchService : public detail::Facade<PatchService, wxl_patch_api>
    {
    public:
        static constexpr const char* kName    = WXL_PATCH_SERVICE;
        static constexpr uint32_t    kVersion = WXL_PATCH_SERVICE_VERSION;

        /// Writes @p size bytes at @p address, provided the memory holds @p expected first.
        Patch Apply(const char* name, uintptr_t address, const void* expected, const void* bytes,
                    uint32_t size) const
        {
            const wxl_patch_api* api = Raw();
            if (!api) return Patch();
            return Patch(api, api->apply(name, address, expected, bytes, size));
        }

        /// The same, with the two byte arrays sized by the compiler.
        template <size_t N>
        Patch Apply(const char* name, uintptr_t address, const uint8_t (&expected)[N],
                    const uint8_t (&bytes)[N]) const
        {
            return Apply(name, address, expected, bytes, static_cast<uint32_t>(N));
        }

        /// Whether the memory at @p address holds @p expected right now, without writing.
        bool Matches(uintptr_t address, const void* expected, uint32_t size) const
        {
            const wxl_patch_api* api = Raw();
            return api && api->matches(address, expected, size) == WXL_TRUE;
        }

        template <size_t N>
        bool Matches(uintptr_t address, const uint8_t (&expected)[N]) const
        {
            return Matches(address, expected, static_cast<uint32_t>(N));
        }

        /// The first match of a byte pattern in the image's read-only data after @p after, or 0.
        uintptr_t Find(const void* pattern, uint32_t size, uintptr_t after = 0) const
        {
            const wxl_patch_api* api = Raw();
            return api ? api->find(pattern, size, after) : 0;
        }

        /// The first whole string literal, terminator included, after @p after, or 0.
        uintptr_t FindLiteral(const char* literal, uintptr_t after = 0) const
        {
            if (!literal) return 0;
            return Find(literal, static_cast<uint32_t>(std::strlen(literal)) + 1, after);
        }
    };

    /**
     * @brief A group of patches applied together and restored together, in reverse order.
     *
     * Apply() answers false when a write is refused, so a mod can stop and let the destructor
     * undo what went before:
     *
     *     wxl::PatchSet edits;
     *     for (const Edit& edit : kEdits)
     *         if (!edits.Apply(edit.name, edit.address, edit.stock, edit.patched, edit.size))
     *             return WXL_E_FAILED;    // what was applied is restored on the way out
     *     m_edits = std::move(edits);     // keep them for the mod's lifetime
     */
    class PatchSet
    {
    public:
        PatchSet() = default;

        PatchSet(PatchSet&&) noexcept            = default;
        PatchSet& operator=(PatchSet&&) noexcept = default;
        PatchSet(const PatchSet&)                = delete;
        PatchSet& operator=(const PatchSet&)     = delete;

        ~PatchSet() { RestoreAll(); }

        /// Applies one patch and keeps it. False when it was refused; nothing else changes.
        bool Apply(const char* name, uintptr_t address, const void* expected, const void* bytes,
                   uint32_t size)
        {
            Patch patch = PatchService().Apply(name, address, expected, bytes, size);
            if (!patch) return false;
            patches_.push_back(std::move(patch));
            return true;
        }

        template <size_t N>
        bool Apply(const char* name, uintptr_t address, const uint8_t (&expected)[N],
                   const uint8_t (&bytes)[N])
        {
            return Apply(name, address, expected, bytes, static_cast<uint32_t>(N));
        }

        /// Adds a patch applied elsewhere. An empty one is ignored.
        void Add(Patch&& patch)
        {
            if (patch) patches_.push_back(std::move(patch));
        }

        /// Restores every patch, last applied first.
        void RestoreAll()
        {
            while (!patches_.empty()) patches_.pop_back();
        }

        /// Gives up ownership of every patch without restoring any.
        void Release()
        {
            for (Patch& patch : patches_) patch.Release();
            patches_.clear();
        }

        size_t Size() const { return patches_.size(); }
        bool   Empty() const { return patches_.empty(); }

    private:
        std::vector<Patch> patches_;
    };
} // namespace wxl

#endif // WXL_SERVICE_PATCH_HPP
