// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/guard.hpp"

#include <cstdio>

#ifdef _WIN32
    #include <windows.h>
#endif
#ifdef _MSC_VER
    #include <malloc.h>
#endif

namespace wxl::host
{
    namespace
    {
#ifdef _MSC_VER
        /// The exception codes the guard leaves alone: a debugger's, and a C++ throw, which the
        /// C++ handler nearer the throw owns.
        constexpr DWORD kCxxException = 0xE06D7363u;

        int Filter(DWORD code, EXCEPTION_POINTERS* info, Fault* fault)
        {
            if (code == EXCEPTION_BREAKPOINT || code == EXCEPTION_SINGLE_STEP || code == kCxxException)
                return EXCEPTION_CONTINUE_SEARCH;
            if (fault)
            {
                const EXCEPTION_RECORD* record = info ? info->ExceptionRecord : nullptr;
                fault->code    = code;
                fault->address = record ? reinterpret_cast<uintptr_t>(record->ExceptionAddress) : 0;
                if (record && (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR) &&
                    record->NumberParameters >= 2)
                {
                    fault->access     = static_cast<uint32_t>(record->ExceptionInformation[0]);
                    fault->target     = static_cast<uintptr_t>(record->ExceptionInformation[1]);
                    fault->has_target = true;
                }
            }
            return EXCEPTION_EXECUTE_HANDLER;
        }

        /// The handler itself, kept out of CallGuarded. A noexcept function's terminate is a
        /// __finally scope over the same call as the __except; Windows stops unwinding at the
        /// __except the unwind targets, Wine (and so Proton) runs every __finally the target
        /// is not inside, and the terminate with it. Here nothing is noexcept: the unwind ends
        /// in this frame, and CallGuarded's terminate stays for a C++ exception.
        __declspec(noinline) bool Guarded(GuardedFn fn, void* context, Fault* fault)
        {
            __try
            {
                fn(context);
            }
            __except (Filter(GetExceptionCode(), GetExceptionInformation(), fault))
            {
                // The guard page is consumed by a stack overflow; without this the next one is fatal.
                if (fault && fault->code == EXCEPTION_STACK_OVERFLOW) _resetstkoflw();
                return false;
            }
            return true;
        }
#endif
    } // namespace

    bool CallGuarded(GuardedFn fn, void* context, Fault* fault) noexcept
    {
        if (!fn) return true;
#ifdef _MSC_VER
        return Guarded(fn, context, fault);
#else
        fn(context);
        return true;
#endif
    }

    const char* FaultName(uint32_t code)
    {
        switch (code)
        {
        case 0xC0000005u: return "access violation";
        case 0xC000001Du: return "illegal instruction";
        case 0xC0000094u: return "integer division by zero";
        case 0xC00000FDu: return "stack overflow";
        case 0xC0000409u: return "fast-fail (stack buffer overrun or an invalid parameter)";
        case 0xC000008Cu: return "array bounds exceeded";
        case 0xC0000096u: return "privileged instruction";
        case 0xC000008Eu: return "floating-point division by zero";
        case 0xC0000025u: return "non-continuable exception";
        case 0xC0000026u: return "invalid disposition";
        case 0x80000002u: return "datatype misalignment";
        case 0xC0000006u: return "in-page error";
        default:          return "exception";
        }
    }

    const char* DescribeAddress(uintptr_t address, char* buffer, size_t capacity)
    {
        if (!buffer || capacity == 0) return "";
#ifdef _WIN32
        HMODULE module = nullptr;
        wchar_t path[MAX_PATH];
        if (address && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                          reinterpret_cast<LPCWSTR>(address), &module))
        {
            const DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
            if (length > 0 && length < MAX_PATH)
            {
                const wchar_t* name = path;
                for (const wchar_t* at = path; *at; ++at)
                    if (*at == L'\\' || *at == L'/') name = at + 1;
                char utf8[MAX_PATH * 3];
                const int bytes =
                    WideCharToMultiByte(CP_UTF8, 0, name, -1, utf8, sizeof utf8, nullptr, nullptr);
                if (bytes > 0)
                {
                    std::snprintf(buffer, capacity, "%s+0x%llX", utf8,
                                  static_cast<unsigned long long>(
                                      address - reinterpret_cast<uintptr_t>(module)));
                    return buffer;
                }
            }
        }
#endif
        std::snprintf(buffer, capacity, "0x%llX", static_cast<unsigned long long>(address));
        return buffer;
    }

    const char* DescribeFault(const Fault& fault, char* buffer, size_t capacity)
    {
        if (!buffer || capacity == 0) return "";
        char where[160];
        DescribeAddress(fault.address, where, sizeof where);
        char touched[64] = "";
        if (fault.has_target)
            std::snprintf(touched, sizeof touched, " %s 0x%llX",
                          fault.access == 1 ? "writing" : fault.access == 8 ? "executing" : "reading",
                          static_cast<unsigned long long>(fault.target));
        std::snprintf(buffer, capacity, "%s (0x%08X)%s at %s", FaultName(fault.code),
                      static_cast<unsigned>(fault.code), touched, where);
        return buffer;
    }
} // namespace wxl::host
