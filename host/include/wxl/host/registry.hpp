// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The services published to mods, by the runtime and by other mods.

#ifndef WXL_HOST_REGISTRY_HPP
#define WXL_HOST_REGISTRY_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "wxl/abi.h"

namespace wxl::host
{
    /// One published service.
    struct PublishedService
    {
        std::string name;
        uint32_t    version = 0;
        const void* iface   = nullptr; ///< a struct of function pointers, valid for the process
    };

    /**
     * @brief Services keyed on name and version.
     *
     * A lookup matches an exact version only: a service's version is its layout, and a different
     * one is a different struct. Publishing under a key already taken is refused; nothing is
     * ever withdrawn, since a mod may have kept the pointer.
     */
    class ServiceRegistry
    {
    public:
        /// @return WXL_E_INVALID on a null name or pointer, or when the key is already taken.
        wxl_result Publish(std::string_view name, uint32_t version, const void* iface);

        const void* Get(std::string_view name, uint32_t version) const;
        bool        Has(std::string_view name, uint32_t version) const;

        const std::vector<PublishedService>& Entries() const { return entries_; }

    private:
        std::vector<PublishedService> entries_;
    };
} // namespace wxl::host

#endif // WXL_HOST_REGISTRY_HPP
