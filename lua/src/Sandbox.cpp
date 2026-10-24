// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Sandbox.hpp"

#include <cstring>
#include <fstream>
#include <new>
#include <sstream>

#include "wxl/sdk.hpp"

namespace wxl::lua
{
    namespace
    {
        /// Runs in every state right after the libraries are opened. Receives the C functions
        /// it builds on as arguments: resolve(path) -> full path | nil, reason; loadchunk(full,
        /// name) -> function | nil, error; write(text).
        constexpr const char* kPrelude = R"lua(
local resolve, loadchunk, write = ...

local rawopen, rawlines = io.open, io.lines
local rawload = load

io.popen, io.tmpfile, io.input, io.output, io.read = nil, nil, nil, nil, nil
io.stdin, io.stdout, io.stderr = nil, nil, nil
os.execute, os.exit, os.getenv, os.remove, os.rename, os.setlocale, os.tmpname =
    nil, nil, nil, nil, nil, nil, nil
package.loadlib = nil
package.cpath = ""
package.path = ""
package.preload.ffi = nil
package.loaded.ffi = nil
package.preload["jit.util"], package.preload["jit.profile"] = nil, nil

-- The rest of debug reaches the upvalues below (the unconfined io.open, the load that takes
-- bytecode) and the registry; a mod keeps what it needs to report where it is.
debug = { traceback = debug.traceback, getinfo = debug.getinfo }
package.loaded.debug = debug

io.open = function(path, mode)
    local full, why = resolve(path)
    if not full then return nil, why end
    return rawopen(full, mode)
end

io.lines = function(path, ...)
    if path == nil then error("io.lines() needs a file name", 2) end
    local full, why = resolve(path)
    if not full then error(why, 2) end
    return rawlines(full, ...)
end

io.write = function(...)
    local parts = {}
    for i = 1, select("#", ...) do parts[i] = tostring((select(i, ...))) end
    write(table.concat(parts))
end

loadfile = function(path, mode, env)
    local full, why = resolve(path)
    if not full then return nil, why end
    local chunk, err = loadchunk(full, path)
    if not chunk then return nil, err end
    if env ~= nil then setfenv(chunk, env) end
    return chunk
end

dofile = function(path)
    local chunk, err = loadfile(path)
    if not chunk then error(err, 2) end
    return chunk()
end

load = function(chunk, name, mode, env)
    return rawload(chunk, name, "t", env)
end

loadstring = function(text, name)
    return rawload(text, name, "t")
end

local function search(name)
    local rel = name:gsub("%.", "/")
    local tried = {}
    for _, candidate in ipairs({ rel .. ".lua", rel .. "/init.lua" }) do
        local full = resolve(candidate)
        if full then
            local file = rawopen(full, "rb")
            if file then
                file:close()
                local chunk, err = loadchunk(full, candidate)
                if not chunk then error(err, 0) end
                return chunk
            end
        end
        tried[#tried + 1] = "\n\tno file '" .. candidate .. "' in the mod folder"
    end
    return table.concat(tried)
end

local function preload(name)
    local loader = package.preload[name]
    if loader then return loader end
    return "\n\tno field package.preload['" .. name .. "']"
end

package.loaders = { preload, search }
package.searchers = package.loaders
)lua";

        /// The folder the sandbox is bound to and the tag its lines carry, kept as upvalues.
        struct Bound
        {
            std::filesystem::path directory;
            std::string           tag;
        };

        Bound* BoundOf(lua_State* L)
        {
            return static_cast<Bound*>(lua_touserdata(L, lua_upvalueindex(1)));
        }

        int Resolve(lua_State* L)
        {
            const Bound*          bound = BoundOf(L);
            std::filesystem::path full;
            std::string           reason;
            if (!ResolveInside(bound->directory, luaL_checkstring(L, 1), full, reason))
            {
                lua_pushnil(L);
                lua_pushstring(L, reason.c_str());
                return 2;
            }
            const std::string text = full.u8string();
            lua_pushlstring(L, text.data(), text.size());
            return 1;
        }

        int LoadChunk(lua_State* L)
        {
            const std::filesystem::path file = std::filesystem::u8path(luaL_checkstring(L, 1));
            const std::string           name = luaL_checkstring(L, 2);
            if (LoadFile(L, file, name) != 0)
            {
                lua_pushnil(L);
                lua_insert(L, -2);
                return 2;
            }
            return 1;
        }

        int Write(lua_State* L)
        {
            size_t      size = 0;
            const char* text = luaL_checklstring(L, 1, &size);
            wxl::Log(WXL_LOG_INFO, BoundOf(L)->tag.c_str(), std::string_view(text, size));
            return 0;
        }

        int CollectBound(lua_State* L)
        {
            static_cast<Bound*>(lua_touserdata(L, 1))->~Bound();
            return 0;
        }
    } // namespace

    bool ResolveInside(const std::filesystem::path& directory, const std::string& relative,
                       std::filesystem::path& out, std::string& reason)
    {
        const std::filesystem::path path = std::filesystem::u8path(relative);
        bool leaves = path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory();
        for (const auto& component : path)
            if (component == "..") leaves = true;
        if (leaves)
        {
            reason = "path '" + relative + "' leaves the mod folder";
            return false;
        }
        out = directory / path;
        return true;
    }

    int LoadFile(lua_State* L, const std::filesystem::path& file, const std::string& name)
    {
        std::ifstream stream(file, std::ios::binary);
        if (!stream)
        {
            lua_pushfstring(L, "cannot open %s", name.c_str());
            return LUA_ERRFILE;
        }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        const std::string text  = buffer.str();
        const std::string chunk = "@" + name;
        // Text only: bytecode is not accepted from a mod folder.
        return luaL_loadbufferx(L, text.data(), text.size(), chunk.c_str(), "t");
    }

    void OpenSandbox(lua_State* L, const std::filesystem::path& directory, const std::string& tag)
    {
        const int top = lua_gettop(L);
        luaL_openlibs(L);

        // The bound folder, owned by the state: freed with it through the metatable's __gc.
        Bound* bound = static_cast<Bound*>(lua_newuserdata(L, sizeof(Bound)));
        new (bound) Bound{directory, tag};
        lua_newtable(L);
        lua_pushcfunction(L, &CollectBound);
        lua_setfield(L, -2, "__gc");
        lua_setmetatable(L, -2);
        const int boundIndex = lua_gettop(L);

        if (luaL_loadbuffer(L, kPrelude, std::strlen(kPrelude), "=wxl.lua") != 0)
        {
            wxl::Log(WXL_LOG_ERROR, "lua", std::string("the sandbox prelude does not compile: ") +
                                               lua_tostring(L, -1));
            lua_settop(L, top);
            return;
        }
        lua_pushvalue(L, boundIndex);
        lua_pushcclosure(L, &Resolve, 1);
        lua_pushvalue(L, boundIndex);
        lua_pushcclosure(L, &LoadChunk, 1);
        lua_pushvalue(L, boundIndex);
        lua_pushcclosure(L, &Write, 1);
        if (lua_pcall(L, 3, 0, 0) != 0)
            wxl::Log(WXL_LOG_ERROR, "lua", std::string("the sandbox prelude failed: ") +
                                               lua_tostring(L, -1));
        lua_settop(L, top);
    }
} // namespace wxl::lua
