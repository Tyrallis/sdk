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
     * @brief Whether a runtime serving @p served can take a mod that needs @p needed.
     *
     * The rule of abi.h: the same major; while the major is 0 the same minor, after it a minor no
     * higher; within the same major.minor a feature no higher.
     * @return empty when it can, otherwise the reason, e.g. "needs abi 0.1.7, newer than this
     *         runtime's 0.1.5: it uses what was appended after 0.1.5".
     */
    std::string AbiRefusal(const AbiVersion& needed, const AbiVersion& served);

    /**
     * @brief The checks a manifest alone can answer.
     *
     * In order: a binary for @p target (none: skip; a manifest with "lua" and no binary is a
     * script mod, skipped with a reason naming wxl.lua), the abi the manifest needs served by
     * this runtime (AbiRefusal), every required service published. The binary path must stay
     * inside the mod folder: no absolute path, no ".." component.
     */
    Decision AdmitManifest(const Manifest& manifest, std::string_view target,
                           const ServiceRegistry& services);

    /**
     * @brief The checks that need what wxl_mod_query returned.
     *
     * A null @p info declines the load. Otherwise the struct must reach at least as far as this
     * runtime reads, its id must be the manifest's, its major.minor the manifest's, and its feature
     * no lower than the one the manifest asks for: a binary and a manifest that disagree are not
     * the same mod. A binary built before the three-part numbering is refused with a reason that
     * says so.
     */
    Decision AdmitInfo(const Manifest& manifest, const wxl_mod_info* info);
} // namespace wxl::host

#endif // WXL_HOST_ADMISSION_HPP
