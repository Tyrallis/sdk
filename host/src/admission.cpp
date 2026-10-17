// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/admission.hpp"

#include <cstring>
#include <filesystem>

namespace wxl::host
{
    const char* VerdictName(Verdict verdict)
    {
        switch (verdict)
        {
        case Verdict::Load: return "load";
        case Verdict::Skip: return "skip";
        case Verdict::Refuse: return "refuse";
        }
        return "?";
    }

    const char* ResultName(wxl_result result)
    {
        switch (result)
        {
        case WXL_OK: return "WXL_OK";
        case WXL_E_UNSUPPORTED: return "WXL_E_UNSUPPORTED";
        case WXL_E_VERSION: return "WXL_E_VERSION";
        case WXL_E_INVALID: return "WXL_E_INVALID";
        case WXL_E_NOT_FOUND: return "WXL_E_NOT_FOUND";
        case WXL_E_FAILED: return "WXL_E_FAILED";
        }
        return "an unknown wxl_result";
    }

    AbiVersion RuntimeAbi() { return AbiVersion{WXL_ABI_VERSION_MAJOR, WXL_ABI_VERSION_MINOR}; }

    namespace
    {
        std::string AbiText(uint32_t major, uint32_t minor)
        {
            return std::to_string(major) + "." + std::to_string(minor);
        }

        Decision Refuse(std::string reason)
        {
            Decision decision;
            decision.verdict = Verdict::Refuse;
            decision.reason  = std::move(reason);
            return decision;
        }

        /// A path a manifest may name: relative, and never leaving the folder.
        bool StaysInside(const std::string& path)
        {
            const std::filesystem::path p(path);
            if (p.is_absolute() || p.has_root_name() || p.has_root_directory()) return false;
            for (const auto& component : p)
                if (component == "..") return false;
            return true;
        }
    } // namespace

    Decision AdmitManifest(const Manifest& manifest, std::string_view target,
                           const ServiceRegistry& services)
    {
        const std::string* binary = manifest.BinaryFor(target);
        if (!binary)
        {
            Decision decision;
            decision.verdict = Verdict::Skip;
            decision.reason  = "no binary for " + std::string(target);
            return decision;
        }
        if (!StaysInside(*binary))
            return Refuse("binary path '" + *binary + "' leaves the mod folder");

        const AbiVersion runtime = RuntimeAbi();
        if (manifest.abi.major != runtime.major)
            return Refuse("built against abi " + AbiText(manifest.abi.major, manifest.abi.minor) +
                          "; this runtime implements " + AbiText(runtime.major, runtime.minor) +
                          " and a different major cannot be reconciled");
        if (manifest.abi.minor > runtime.minor)
            return Refuse("built against abi " + AbiText(manifest.abi.major, manifest.abi.minor) +
                          ", newer than this runtime's " + AbiText(runtime.major, runtime.minor));

        for (const ServiceRef& ref : manifest.required)
        {
            if (!services.Has(ref.service, ref.version))
                return Refuse("requires service " + ref.service + " version " +
                              std::to_string(ref.version) +
                              ", which this runtime does not have");
        }

        Decision decision;
        decision.verdict = Verdict::Load;
        decision.binary  = *binary;
        return decision;
    }

    Decision AdmitInfo(const Manifest& manifest, const wxl_mod_info* info)
    {
        if (!info) return Refuse("wxl_mod_query returned NULL: the mod declined to load");

        // One layout exists so far. When a field is appended, the sizes of every earlier minor
        // belong here too, so a mod built against one is still read correctly.
        if (info->struct_size != sizeof(wxl_mod_info))
            return Refuse("wxl_mod_info.struct_size is " + std::to_string(info->struct_size) +
                          ", expected " + std::to_string(sizeof(wxl_mod_info)));

        if (!info->id) return Refuse("wxl_mod_info.id is NULL");
        if (std::strcmp(info->id, manifest.id.c_str()) != 0)
            return Refuse(std::string("the binary reports id '") + info->id +
                          "' but the manifest says '" + manifest.id + "'");

        if (info->abi_major != manifest.abi.major || info->abi_minor != manifest.abi.minor)
            return Refuse("the binary reports abi " + AbiText(info->abi_major, info->abi_minor) +
                          " but the manifest says " +
                          AbiText(manifest.abi.major, manifest.abi.minor));

        Decision decision;
        decision.verdict = Verdict::Load;
        return decision;
    }
} // namespace wxl::host
