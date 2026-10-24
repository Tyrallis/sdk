// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// What a mod's state may reach: the standard libraries less what leaves the process, and files
// inside the mod's own folder only.

#ifndef WXL_LUA_SANDBOX_HPP
#define WXL_LUA_SANDBOX_HPP

#include <filesystem>
#include <string>

#include "Lua.hpp"

namespace wxl::lua
{
    /**
     * @brief Opens the standard libraries in a state and closes the doors.
     *
     * Removed: os.execute, os.exit, os.getenv, os.remove, os.rename, os.setlocale, os.tmpname,
     * io.popen, io.tmpfile, io.input, io.output, io.read, io.stdin/stdout/stderr, package.loadlib,
     * ffi. io.open, io.lines, loadfile, dofile and require resolve inside @p directory and refuse
     * a path that leaves it; load and loadstring take text only. io.write goes to the log.
     *
     * Leaves the stack as it found it. @p directory is the mod's folder; @p tag is what its log
     * lines are tagged with.
     */
    void OpenSandbox(lua_State* L, const std::filesystem::path& directory, const std::string& tag);

    /// A path relative to a mod's folder, resolved; false with @p reason when it leaves the
    /// folder: absolute, rooted, or with a ".." component.
    bool ResolveInside(const std::filesystem::path& directory, const std::string& relative,
                       std::filesystem::path& out, std::string& reason);

    /// Reads a file as a Lua chunk and leaves the function on the stack, or an error message.
    /// Returns what luaL_loadbuffer returns. @p name is the chunk name tracebacks show.
    int LoadFile(lua_State* L, const std::filesystem::path& file, const std::string& name);
} // namespace wxl::lua

#endif // WXL_LUA_SANDBOX_HPP
