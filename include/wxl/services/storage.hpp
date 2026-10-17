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
#include "wxl/services/storage.h"

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
            if (!api_ || handle_ == WXL_HANDLE_NONE) return Unexpected{WXL_E_NOT_FOUND};
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
            if (api_) api_->close(handle_);
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
     */
    class StorageService : public detail::Facade<StorageService, wxl_storage_api>
    {
    public:
        static constexpr const char* kName    = WXL_STORAGE_SERVICE;
        static constexpr uint32_t    kVersion = WXL_STORAGE_SERVICE_VERSION;

        bool Exists(std::string_view path) const
        {
            const wxl_storage_api* api = Raw();
            return api && api->exists(Str(path)) == WXL_TRUE;
        }

        /// The file's length, without reading it.
        Expected<uint64_t> Size(std::string_view path) const
        {
            const wxl_storage_api* api = Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};
            uint64_t         size   = 0;
            const wxl_result result = api->size(Str(path), &size);
            if (result != WXL_OK) return Unexpected{result};
            return size;
        }

        /// A whole file.
        Expected<std::vector<uint8_t>> Read(std::string_view path) const
        {
            const wxl_storage_api* api = Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};

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
            if (!api) return File();
            return File(api, api->open(Str(path)));
        }

        /// Adds a directory to the search path. Higher priority wins a name collision.
        wxl_result AddSearchPath(std::string_view directory, int32_t priority = 0) const
        {
            const wxl_storage_api* api = Raw();
            if (!api) return WXL_E_UNSUPPORTED;
            return api->add_search_path(Str(directory), priority);
        }

        /// Adds an archive to the search path.
        wxl_result MountArchive(std::string_view path, int32_t priority = 0) const
        {
            const wxl_storage_api* api = Raw();
            if (!api) return WXL_E_UNSUPPORTED;
            return api->mount_archive(Str(path), priority);
        }

        /// The paths matching a pattern, `*` standing for any run of characters.
        std::vector<std::string> Find(std::string_view pattern) const
        {
            std::vector<std::string> found;
            const wxl_storage_api*   api = Raw();
            if (!api) return found;

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
