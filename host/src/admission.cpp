// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/admission.hpp"

#include <cstddef>
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

    AbiVersion RuntimeAbi()
    {
        return AbiVersion{WXL_ABI_VERSION_MAJOR, WXL_ABI_VERSION_MINOR, WXL_ABI_VERSION_FEATURE};
    }

    std::string AbiRefusal(const AbiVersion& needed, const AbiVersion& served)
    {
        const std::string wanted = "needs abi " + AbiText(needed);
        const std::string has    = AbiText(served);
        if (needed.major != served.major)
            return wanted + "; this runtime implements " + has +
                   " and a different major cannot be reconciled";
        if (needed.minor != served.minor && served.major == 0)
            return wanted + "; this runtime implements " + has +
                   ", and while the major is 0 a different minor is a different contract";
        if (needed.minor > served.minor)
            return wanted + ", a later stage than this runtime's " + has;
        if (needed.minor == served.minor && needed.feature > served.feature)
            return wanted + ", newer than this runtime's " + has + ": it uses what was appended after " +
                   has;
        return std::string();
    }

    namespace
    {
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
            // A manifest with a "lua" entry and nothing to load natively is a script mod:
            // wxl.lua finds it by the same manifest and runs it.
            if (!manifest.lua.empty())
                decision.reason = "script mod (lua: " + manifest.lua + "): wxl.lua serves it";
            else decision.reason = "no binary for " + std::string(target);
            return decision;
        }
        if (!StaysInside(*binary))
            return Refuse("binary path '" + *binary + "' leaves the mod folder");

        if (std::string reason = AbiRefusal(manifest.abi, RuntimeAbi()); !reason.empty())
            return Refuse(std::move(reason));

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

        // Every field read below is in the 0.1.5 layout; a binary built against later headers
        // may carry more, which is not read. Short of abi_feature is the two-part numbering's
        // layout (on x86: on x64 abi_feature took the tail padding and abi_minor tells instead).
        if (info->struct_size < sizeof(wxl_mod_info))
        {
            if (info->struct_size >= offsetof(wxl_mod_info, abi_feature))
                return Refuse("wxl_mod_info.struct_size is " + std::to_string(info->struct_size) +
                              ", without abi_feature: the binary was built with the two-part abi "
                              "numbering, before 0.1.5; rebuild it against the current SDK");
            return Refuse("wxl_mod_info.struct_size is " + std::to_string(info->struct_size) +
                          ", expected at least " + std::to_string(sizeof(wxl_mod_info)));
        }

        if (!info->id) return Refuse("wxl_mod_info.id is NULL");
        if (std::strcmp(info->id, manifest.id.c_str()) != 0)
            return Refuse(std::string("the binary reports id '") + info->id +
                          "' but the manifest says '" + manifest.id + "'");

        if (info->abi_major != manifest.abi.major || info->abi_minor != manifest.abi.minor)
            return Refuse("the binary reports abi " + std::to_string(info->abi_major) + "." +
                          std::to_string(info->abi_minor) + " but the manifest says " +
                          AbiText(manifest.abi) + ": they are different contracts");
        if (info->abi_feature < manifest.abi.feature)
            return Refuse("the binary was built against abi " +
                          AbiText(AbiVersion{info->abi_major, info->abi_minor, info->abi_feature}) +
                          " but the manifest asks for " + AbiText(manifest.abi) +
                          ": a binary cannot need what its headers did not carry");

        Decision decision;
        decision.verdict = Verdict::Load;
        return decision;
    }
} // namespace wxl::host
