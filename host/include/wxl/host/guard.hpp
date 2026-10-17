// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The crash guard: a call into mod code that reports a hardware fault instead of taking the
// process down with it.

#ifndef WXL_HOST_GUARD_HPP
#define WXL_HOST_GUARD_HPP

#include <cstddef>
#include <cstdint>

namespace wxl::host
{
    /// A hardware fault the guard caught: the OS exception code and the faulting address.
    struct Fault
    {
        uint32_t  code    = 0;
        uintptr_t address = 0;
    };

    /// What the guard calls, with the context it was handed.
    using GuardedFn = void (*)(void* context);

    /**
     * @brief Calls @p fn(context) and reports whether it returned.
     *
     * On MSVC the call runs under a structured-exception handler: an access violation, an
     * illegal instruction, a stack overflow or any other hardware fault raised inside it is
     * caught, @p fault is filled, and false is returned. A breakpoint, a single-step and a C++
     * exception are left to whoever is looking for them. Elsewhere the call is plain and the
     * function always returns true.
     *
     * The frames between the fault and the guard are abandoned, not unwound: their destructors
     * do not run, locks they hold stay held, and whatever was being written stays as it was.
     */
    bool CallGuarded(GuardedFn fn, void* context, Fault* fault) noexcept;

    /// "access violation", "stack overflow", ... or "exception" for a code without a name.
    const char* FaultName(uint32_t code);

    /// Writes "<name> (0x<code>) at 0x<address>" into @p buffer and returns it.
    const char* DescribeFault(const Fault& fault, char* buffer, size_t capacity);
} // namespace wxl::host

#endif // WXL_HOST_GUARD_HPP
