// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// Finding mods on disk and deciding the order they load in.

#ifndef WXL_HOST_DISCOVERY_HPP
#define WXL_HOST_DISCOVERY_HPP

#include <filesystem>
#include <string>
#include <vector>

#include "wxl/host/manifest.hpp"

namespace wxl::host
{
    /// A folder with a readable manifest. Nothing of it has been loaded yet.
    struct ModCandidate
    {
        std::string           directory_name; ///< the folder's own name, the tie-breaker
        std::filesystem::path directory;      ///< the folder, mods directory included
        Manifest              manifest;
    };

    /// A folder that was found and could not be used, with the reason.
    struct DiscoveryProblem
    {
        std::string           directory_name;
        std::filesystem::path directory;
        std::string           id; ///< the manifest's id when it could be read, else empty
        std::string           message;
    };

    struct Discovery
    {
        std::vector<ModCandidate>     mods;     ///< in directory-name order
        std::vector<DiscoveryProblem> problems; ///< unreadable manifests, duplicate ids
    };

    /**
     * @brief Scans a mods directory: every sub-folder holding a mod.json.
     *
     * Folders are visited in byte-wise name order regardless of what the filesystem returns, so
     * two scans of the same tree agree. A manifest that does not parse is a problem, not a stop.
     * Two folders claiming the same id keep the first and report the second. A missing or
     * unreadable mods directory yields nothing.
     */
    Discovery Discover(const std::filesystem::path& mods_directory);

    struct LoadOrder
    {
        std::vector<ModCandidate>     ordered;  ///< what loads, in order
        std::vector<DiscoveryProblem> excluded; ///< caught in a cycle, or waiting on one
    };

    /**
     * @brief Orders candidates so that every `loadAfter` names something earlier.
     *
     * `loadAfter` is a soft constraint: an id nobody provides is ignored. Among mods nothing
     * orders, directory-name order holds, so the result is the same on every run. A cycle
     * excludes its members, each with a message naming the whole cycle, and excludes whatever
     * loads after one of them.
     */
    LoadOrder OrderByLoadAfter(const std::vector<ModCandidate>& mods);
} // namespace wxl::host

#endif // WXL_HOST_DISCOVERY_HPP
