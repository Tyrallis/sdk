// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The OS behind LibraryLoader. Windows only; another platform adds its own block here.

#include "wxl/host/loader.hpp"

#if defined(_WIN32)

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>

namespace wxl::host
{
    namespace
    {
        /// The text of the last Win32 error, trailing newline removed.
        std::string LastErrorText()
        {
            const DWORD code = ::GetLastError();
            char*       text = nullptr;
            const DWORD size = ::FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<char*>(&text), 0, nullptr);

            std::string message = size && text ? std::string(text, size) : std::string();
            if (text) ::LocalFree(text);
            while (!message.empty() && (message.back() == '\n' || message.back() == '\r' ||
                                        message.back() == '.' || message.back() == ' '))
                message.pop_back();
            if (message.empty()) message = "error " + std::to_string(code);
            return message;
        }

        class Win32Loader final : public LibraryLoader
        {
        public:
            void* Open(const std::filesystem::path& path, std::string& error) override
            {
                // The binary's own folder joins the search path, so a mod can ship the DLLs it
                // depends on next to itself.
                HMODULE module = ::LoadLibraryExW(path.c_str(), nullptr,
                                                  LOAD_WITH_ALTERED_SEARCH_PATH);
                if (!module) error = LastErrorText();
                return module;
            }

            void* Symbol(void* library, const char* name) override
            {
                return reinterpret_cast<void*>(
                    ::GetProcAddress(static_cast<HMODULE>(library), name));
            }

            void Close(void* library) override { ::FreeLibrary(static_cast<HMODULE>(library)); }
        };
    } // namespace

    LibraryLoader& PlatformLoader()
    {
        static Win32Loader loader;
        return loader;
    }
} // namespace wxl::host

#else
    #error "wxl-host: no LibraryLoader for this platform; add one to platform.cpp"
#endif
