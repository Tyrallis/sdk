// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// A value or the wxl_result that stood in for it.

#ifndef WXL_EXPECTED_HPP
#define WXL_EXPECTED_HPP

#include <optional>
#include <type_traits>
#include <utility>

#include "wxl/abi.h"

namespace wxl
{
    /// The failed half of an Expected: `return wxl::Unexpected{WXL_E_NOT_FOUND};`.
    struct Unexpected
    {
        wxl_result error;
    };

    /**
     * @brief Either a value or the wxl_result that explains its absence.
     *
     * Every read through a service answers with one of these. The three outcomes a bare value
     * could not separate are distinct here:
     *
     *     wxl::Expected<uint32_t> hp = unit.Health();
     *     if (hp)                  // a value, possibly 0: the unit is dead
     *     else if (hp.IsNotFound())    // the handle is stale: the unit is gone
     *     else if (hp.IsUnsupported()) // this runtime has no object service
     *
     * Value(), operator* and operator-> require HasValue(); ValueOr() never does.
     */
    template <class T>
    class Expected
    {
        static_assert(!std::is_reference_v<T>, "Expected holds a value, not a reference");

    public:
        using value_type = T;

        Expected(const T& value) : value_(value) {}
        Expected(T&& value) : value_(std::move(value)) {}

        /// WXL_OK is not an absence; it is recorded as WXL_E_FAILED.
        Expected(Unexpected failure) : error_(failure.error == WXL_OK ? WXL_E_FAILED : failure.error)
        {
        }

        bool     HasValue() const { return value_.has_value(); }
        explicit operator bool() const { return HasValue(); }

        /// WXL_OK while a value is held.
        wxl_result Error() const { return error_; }

        bool IsUnsupported() const { return error_ == WXL_E_UNSUPPORTED; }
        bool IsNotFound() const { return error_ == WXL_E_NOT_FOUND; }
        bool IsInvalid() const { return error_ == WXL_E_INVALID; }

        T&        Value() & { return *value_; }
        const T&  Value() const& { return *value_; }
        T&&       Value() && { return std::move(*value_); }
        T&        operator*() & { return *value_; }
        const T&  operator*() const& { return *value_; }
        T&&       operator*() && { return std::move(*value_); }
        T*        operator->() { return &*value_; }
        const T*  operator->() const { return &*value_; }

        template <class U>
        T ValueOr(U&& fallback) const&
        {
            return value_ ? *value_ : static_cast<T>(std::forward<U>(fallback));
        }

        template <class U>
        T ValueOr(U&& fallback) &&
        {
            return value_ ? std::move(*value_) : static_cast<T>(std::forward<U>(fallback));
        }

        /// The value alone, for code that does not care why it is missing.
        std::optional<T> ToOptional() const& { return value_; }
        std::optional<T> ToOptional() && { return std::move(value_); }

    private:
        std::optional<T> value_;
        wxl_result       error_ = WXL_OK;
    };
} // namespace wxl

#endif // WXL_EXPECTED_HPP
