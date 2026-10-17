// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/loader.hpp"

#include <utility>

namespace wxl::host
{
    ModBinary::~ModBinary() { Close(); }

    ModBinary::ModBinary(ModBinary&& other) noexcept
        : loader_(other.loader_), library_(other.library_), entries_(other.entries_),
          path_(std::move(other.path_))
    {
        other.loader_  = nullptr;
        other.library_ = nullptr;
        other.entries_ = EntryPoints{};
    }

    ModBinary& ModBinary::operator=(ModBinary&& other) noexcept
    {
        if (this != &other)
        {
            Close();
            loader_        = other.loader_;
            library_       = other.library_;
            entries_       = other.entries_;
            path_          = std::move(other.path_);
            other.loader_  = nullptr;
            other.library_ = nullptr;
            other.entries_ = EntryPoints{};
        }
        return *this;
    }

    bool ModBinary::Open(LibraryLoader& loader, const std::filesystem::path& path, ModBinary& out,
                         std::string& error)
    {
        std::string reason;
        void*       library = loader.Open(path, reason);
        if (!library)
        {
            error = "cannot load " + path.string() + ": " + reason;
            return false;
        }

        // A function pointer comes back as void*; the cast is what every OS loader API needs.
        EntryPoints entries;
        entries.query  = reinterpret_cast<wxl_mod_query_fn>(loader.Symbol(library, kEntryQuery));
        entries.load   = reinterpret_cast<wxl_mod_load_fn>(loader.Symbol(library, kEntryLoad));
        entries.stage  = reinterpret_cast<wxl_mod_stage_fn>(loader.Symbol(library, kEntryStage));
        entries.unload = reinterpret_cast<wxl_mod_unload_fn>(loader.Symbol(library, kEntryUnload));

        const char* missing = !entries.query ? kEntryQuery : !entries.load ? kEntryLoad : nullptr;
        if (missing)
        {
            loader.Close(library);
            error = path.string() + " does not export " + missing;
            return false;
        }

        out.Close();
        out.loader_  = &loader;
        out.library_ = library;
        out.entries_ = entries;
        out.path_    = path;
        return true;
    }

    void ModBinary::Close()
    {
        if (library_) loader_->Close(library_);
        library_ = nullptr;
        loader_  = nullptr;
        entries_ = EntryPoints{};
    }

    void ModBinary::Abandon()
    {
        library_ = nullptr;
        loader_  = nullptr;
        entries_ = EntryPoints{};
    }
} // namespace wxl::host
