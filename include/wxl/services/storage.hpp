// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.storage: the client's files, whatever they are packed in.

#ifndef WXL_SERVICE_STORAGE_HPP
#define WXL_SERVICE_STORAGE_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "wxl/sdk.hpp"
#include "wxl/services/rows.hpp"
#include "wxl/services/storage.h"

namespace wxl::api::facade
{
    template <>
    struct Methods<wxl_storage_api>
    {
#define WXL_API_TABLE "wxl/services/storage.def"
#include "wxl/services/methods.inc"
    };
} // namespace wxl::api::facade

namespace wxl
{
    /**
     * @brief An open file, closed when this goes out of scope.
     *
     * Move-only. Release() hands the handle back for code that closes it by hand.
     */
    class File
    {
    public:
        File() = default;
        File(const wxl_storage_api* api, wxl_handle handle) : api_(api), handle_(handle) {}

        File(File&& other) noexcept : api_(other.api_), handle_(other.Release()) {}
        File& operator=(File&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                api_    = other.api_;
                handle_ = other.Release();
            }
            return *this;
        }

        File(const File&)            = delete;
        File& operator=(const File&) = delete;

        ~File() { Reset(); }

        explicit operator bool() const { return handle_ != WXL_HANDLE_NONE; }
        wxl_handle Get() const { return handle_; }

        /// Reads up to @p size bytes at @p offset; the value is how many were read.
        Expected<uint64_t> ReadAt(uint64_t offset, void* buffer, uint64_t size) const
        {
            if (!detail::Carries(api_, &wxl_storage_api::read_at))
                return Unexpected{WXL_E_UNSUPPORTED};
            if (handle_ == WXL_HANDLE_NONE) return Unexpected{WXL_E_NOT_FOUND};
            uint64_t         read   = 0;
            const wxl_result result = api_->read_at(handle_, offset, buffer, size, &read);
            if (result != WXL_OK) return Unexpected{result};
            return read;
        }

        /// Gives up ownership without closing.
        wxl_handle Release()
        {
            const wxl_handle handle = handle_;
            handle_                 = WXL_HANDLE_NONE;
            return handle;
        }

        /// Closes now. Safe to call more than once.
        void Reset()
        {
            if (handle_ == WXL_HANDLE_NONE) return;
            if (detail::Carries(api_, &wxl_storage_api::close)) api_->close(handle_);
            handle_ = WXL_HANDLE_NONE;
        }

    private:
        const wxl_storage_api* api_    = nullptr;
        wxl_handle             handle_ = WXL_HANDLE_NONE;
    };

    /**
     * @brief The typed facade over wxl.storage.
     *
     * Paths are the client's own: backslash-separated, case-insensitive, relative to the data
     * root. Every call answers WXL_E_UNSUPPORTED while the runtime does not publish the service.
     *
     * The plain methods are rows of storage.def, and answer as storage.h says:
     *
     *   Exists(path)                        bool                whether the name resolves
     *   Size(path)                          Expected<uint64_t>  its length, without reading it
     *   AddSearchPath(directory, priority)  wxl_result          a folder joins the search: the
     *                                                           higher priority wins a name, the
     *                                                           client's own data is 0
     *   MountArchive(path, priority)        wxl_result          an archive joins the search
     *
     * The rest are written here, for what C++ does better than the raw table: a whole file into
     * a vector, a File that closes itself, a list of names.
     */
    class StorageService : public detail::Facade<StorageService, wxl_storage_api>,
                           public api::facade::Methods<wxl_storage_api>
    {
    public:
        static constexpr const char* kName    = WXL_STORAGE_SERVICE;
        static constexpr uint32_t    kVersion = WXL_STORAGE_SERVICE_VERSION;

        /// A whole file.
        Expected<std::vector<uint8_t>> Read(std::string_view path) const
        {
            const wxl_storage_api* api = Raw();
            if (!detail::Carries(api, &wxl_storage_api::read))
                return Unexpected{WXL_E_UNSUPPORTED};

            const Expected<uint64_t> size = Size(path);
            if (!size) return Unexpected{size.Error()};

            std::vector<uint8_t> bytes(static_cast<size_t>(*size));
            uint64_t             read   = 0;
            wxl_result           result = api->read(Str(path), bytes.data(), bytes.size(), &read);
            if (result == WXL_E_INVALID && read > bytes.size())
            {
                // The file grew between the two calls: once more, at the size it now reports.
                bytes.resize(static_cast<size_t>(read));
                result = api->read(Str(path), bytes.data(), bytes.size(), &read);
            }
            if (result != WXL_OK) return Unexpected{result};
            bytes.resize(static_cast<size_t>(read));
            return bytes;
        }

        /// A whole file, as text.
        Expected<std::string> ReadText(std::string_view path) const
        {
            Expected<std::vector<uint8_t>> bytes = Read(path);
            if (!bytes) return Unexpected{bytes.Error()};
            return std::string(bytes->begin(), bytes->end());
        }

        /// Opens a file for streaming; an empty File means it was not found.
        File Open(std::string_view path) const
        {
            const wxl_storage_api* api = Raw();
            if (!detail::Carries(api, &wxl_storage_api::open)) return File();
            return File(api, api->open(Str(path)));
        }

        /// The paths matching a pattern, `*` standing for any run of characters.
        std::vector<std::string> Find(std::string_view pattern) const
        {
            std::vector<std::string> found;
            const wxl_storage_api*   api = Raw();
            if (!detail::Carries(api, &wxl_storage_api::find)) return found;

            const uint32_t count = api->find(Str(pattern), nullptr, 0);
            if (!count) return found;

            std::vector<wxl_str> slices(count);
            const uint32_t       written = api->find(Str(pattern), slices.data(), count);
            found.reserve(written < count ? written : count);
            for (uint32_t i = 0; i < written && i < count; ++i) found.emplace_back(View(slices[i]));
            return found;
        }
    };
} // namespace wxl

#endif // WXL_SERVICE_STORAGE_HPP
