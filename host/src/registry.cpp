// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/registry.hpp"

namespace wxl::host
{
    wxl_result ServiceRegistry::Publish(std::string_view name, uint32_t version, const void* iface)
    {
        if (name.empty() || !iface) return WXL_E_INVALID;
        if (Has(name, version)) return WXL_E_INVALID;
        entries_.push_back(PublishedService{std::string(name), version, iface});
        return WXL_OK;
    }

    const void* ServiceRegistry::Get(std::string_view name, uint32_t version) const
    {
        for (const PublishedService& entry : entries_)
            if (entry.version == version && entry.name == name) return entry.iface;
        return nullptr;
    }

    bool ServiceRegistry::Has(std::string_view name, uint32_t version) const
    {
        return Get(name, version) != nullptr;
    }
} // namespace wxl::host
