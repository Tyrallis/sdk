// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// Whether a mod may load, decided before any of its code runs, with a reason when it may not.

#ifndef WXL_HOST_ADMISSION_HPP
#define WXL_HOST_ADMISSION_HPP

#include <string>
#include <string_view>

#include "wxl/host/manifest.hpp"
#include "wxl/host/registry.hpp"
#include "wxl/mod.h"

namespace wxl::host
{
    enum class Verdict
    {
        Load,   ///< nothing stands in the way
        Skip,   ///< the mod is not for this runtime; an ordinary outcome, not a fault
        Refuse  ///< the mod wanted to load here and cannot
    };

    /// "load", "skip" or "refuse".
    const char* VerdictName(Verdict verdict);

    /// "WXL_OK", "WXL_E_VERSION", ...
    const char* ResultName(wxl_result result);

    struct Decision
    {
        Verdict     verdict = Verdict::Refuse;
        std::string reason;  ///< empty for Verdict::Load
        std::string binary;  ///< the manifest's binary for the target, when Verdict::Load

        explicit operator bool() const { return verdict == Verdict::Load; }
    };

    /// The contract this runtime implements, from abi.h.
    AbiVersion RuntimeAbi();

    /**
     * @brief The checks a manifest alone can answer.
     *
     * In order: a binary for @p target (none: skip), the abi major equal to this runtime's and
     * the minor no higher, every required service published. The binary path must stay inside
     * the mod folder: no absolute path, no ".." component.
     */
    Decision AdmitManifest(const Manifest& manifest, std::string_view target,
                           const ServiceRegistry& services);

    /**
     * @brief The checks that need what wxl_mod_query returned.
     *
     * A null @p info declines the load. Otherwise the struct must be the size this runtime knows,
     * and its id and abi must say what the manifest says: a binary and a manifest that disagree
     * are not the same mod.
     */
    Decision AdmitInfo(const Manifest& manifest, const wxl_mod_info* info);
} // namespace wxl::host

#endif // WXL_HOST_ADMISSION_HPP
