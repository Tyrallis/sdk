// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/guard.hpp"

#include <cstdio>

#ifdef _MSC_VER
    #include <windows.h>
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
                fault->code    = code;
                fault->address = info && info->ExceptionRecord
                                     ? reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress)
                                     : 0;
            }
            return EXCEPTION_EXECUTE_HANDLER;
        }
#endif
    } // namespace

    bool CallGuarded(GuardedFn fn, void* context, Fault* fault) noexcept
    {
        if (!fn) return true;
#ifdef _MSC_VER
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

    const char* DescribeFault(const Fault& fault, char* buffer, size_t capacity)
    {
        if (!buffer || capacity == 0) return "";
        std::snprintf(buffer, capacity, "%s (0x%08X) at 0x%llX", FaultName(fault.code),
                      static_cast<unsigned>(fault.code),
                      static_cast<unsigned long long>(fault.address));
        return buffer;
    }
} // namespace wxl::host
