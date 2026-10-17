// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// Getting a mod's binary into the process and its entry points out of it.

#ifndef WXL_HOST_LOADER_HPP
#define WXL_HOST_LOADER_HPP

#include <filesystem>
#include <string>

#include "wxl/mod.h"

namespace wxl::host
{
    /**
     * @brief Opens binaries and resolves symbols: the one place the OS is called.
     *
     * @ref PlatformLoader is the real one. A test substitutes its own to hand the host
     * in-process "binaries" without a file on disk.
     */
    class LibraryLoader
    {
    public:
        virtual ~LibraryLoader() = default;

        /// @return an opaque handle, or nullptr with @p error set to the OS's reason.
        virtual void* Open(const std::filesystem::path& path, std::string& error) = 0;

        /// @return the exported symbol, or nullptr when the binary has none by that name.
        virtual void* Symbol(void* library, const char* name) = 0;

        virtual void Close(void* library) = 0;
    };

    /// The loader for this OS. Only Windows is implemented; another platform fails to compile.
    LibraryLoader& PlatformLoader();

    /// The four entry points of mod.h. The last two are optional and stay null when absent.
    struct EntryPoints
    {
        wxl_mod_query_fn  query  = nullptr;
        wxl_mod_load_fn   load   = nullptr;
        wxl_mod_stage_fn  stage  = nullptr;
        wxl_mod_unload_fn unload = nullptr;
    };

    /// Symbol names, as a runtime resolves them.
    constexpr const char* kEntryQuery  = "wxl_mod_query";
    constexpr const char* kEntryLoad   = "wxl_mod_load";
    constexpr const char* kEntryStage  = "wxl_mod_stage";
    constexpr const char* kEntryUnload = "wxl_mod_unload";

    /**
     * @brief A loaded binary and its entry points. Closes the binary when destroyed.
     *
     * Opening resolves the entry points and fails when a required one is missing, so a
     * successfully opened ModBinary always has @ref EntryPoints::query and
     * @ref EntryPoints::load. No mod code has run at that point.
     */
    class ModBinary
    {
    public:
        ModBinary() = default;
        ~ModBinary();

        ModBinary(ModBinary&& other) noexcept;
        ModBinary& operator=(ModBinary&& other) noexcept;
        ModBinary(const ModBinary&)            = delete;
        ModBinary& operator=(const ModBinary&) = delete;

        /// @return false with @p error set; @p out is untouched then.
        static bool Open(LibraryLoader& loader, const std::filesystem::path& path, ModBinary& out,
                         std::string& error);

        /// Unloads the binary. Idempotent.
        void Close();

        explicit operator bool() const { return library_ != nullptr; }

        const EntryPoints&           Entries() const { return entries_; }
        const std::filesystem::path& Path() const { return path_; }

    private:
        LibraryLoader*        loader_  = nullptr;
        void*                 library_ = nullptr;
        EntryPoints           entries_;
        std::filesystem::path path_;
    };
} // namespace wxl::host

#endif // WXL_HOST_LOADER_HPP
