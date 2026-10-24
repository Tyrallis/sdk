// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 WarcraftXL
//
// Writes the editor definitions of the Lua API -- a lua-language-server meta file -- from the
// event table of reflect.hpp and the getter table of object_reads.def, so that what an editor
// completes is what wxl.lua serves.
//
//     wxl-lua-types                  the file on stdout
//     wxl-lua-types <file>           written there, only when it differs
//     wxl-lua-types --check <file>   exit 1 when the file differs from what would be written

#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../src/ObjectNames.hpp"
#include "wxl/reflect.hpp"

namespace
{
    using wxl::reflect::Event;
    using wxl::reflect::Field;
    using wxl::reflect::Kind;

    const char* LuaType(Kind kind)
    {
        switch (kind)
        {
        case Kind::u32:
        case Kind::i32:
        case Kind::u64:
        case Kind::i64: return "integer";
        case Kind::f32: return "number";
        case Kind::boolean: return "boolean";
        case Kind::handle: return "wxl.Object|nil";
        case Kind::guid: return "wxl.Guid";
        case Kind::vec3: return "wxl.Vec3";
        case Kind::str: return "string";
        case Kind::ptr: return "lightuserdata|nil";
        case Kind::bytes: return "wxl.Bytes";
        case Kind::answer: return "boolean|nil";
        case Kind::buffer: return "string|nil";
        }
        return "any";
    }

    bool Writable(const Field& field) { return field.kind == Kind::answer || field.kind == Kind::buffer; }

    bool Answerable(const Event& event)
    {
        for (uint32_t i = 0; i < event.field_count; ++i)
            if (Writable(event.fields[i])) return true;
        return false;
    }

    /// "delta: number": a handler's parameters, the members IsExtent leaves, as Lua types.
    std::string Parameters(const Event& event)
    {
        std::string text;
        for (uint32_t i = 0; i < event.field_count; ++i)
        {
            const Field& field = event.fields[i];
            if (wxl::reflect::IsExtent(event, field)) continue;
            if (!text.empty()) text += ", ";
            text += field.name;
            text += ": ";
            text += LuaType(field.kind);
        }
        return text;
    }

    /// What an Override returns: one value per writable member, in member order.
    std::string Returns(const Event& event)
    {
        std::string text;
        for (uint32_t i = 0; i < event.field_count; ++i)
        {
            const Field& field = event.fields[i];
            if (!Writable(field)) continue;
            if (!text.empty()) text += ", ";
            text += field.kind == Kind::answer ? "boolean|nil" : "string|nil";
        }
        return text;
    }

    /// "handled", or "path, then flag": the writable members, for a description.
    std::string WritableNames(const Event& event)
    {
        std::string text;
        for (uint32_t i = 0; i < event.field_count; ++i)
        {
            const Field& field = event.fields[i];
            if (!Writable(field)) continue;
            if (!text.empty()) text += ", then ";
            text += field.name;
        }
        return text;
    }

    std::vector<std::string> Families(const Event* events, uint32_t count)
    {
        std::vector<std::string> families;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::string family = std::string(events[i].family) + "Script";
            bool              seen   = false;
            for (const std::string& known : families)
                if (known == family) seen = true;
            if (!seen) families.push_back(family);
        }
        return families;
    }

    /// One row of object_reads.def, as the generator sees it.
    struct Row
    {
        const char* member;
        uint32_t    kinds;
        const char* type;
        bool        indexed;
    };

    const char* ValueType(const char* type)
    {
        if (std::strcmp(type, "uint32_t") == 0) return "integer";
        if (std::strcmp(type, "float") == 0) return "number";
        if (std::strcmp(type, "wxl_guid") == 0) return "string";
        if (std::strcmp(type, "wxl_vec3") == 0) return "wxl.Vec3";
        return "any";
    }

    std::string Generate()
    {
        std::ostringstream out;
        uint32_t           count  = 0;
        const Event*       events = wxl::reflect::Events(&count);
        const std::vector<std::string> families = Families(events, count);

        out << "---@meta wxl\n"
               "-- The Lua API of wxl.lua, for lua-language-server. Generated from the SDK's event\n"
               "-- table (reflect.hpp) and object table (object_reads.def) by wxl-lua-types; do not\n"
               "-- edit by hand.\n\n";

        out << "---A guid, as \"0x\" and sixteen hex digits: nothing is lost and it keys a table.\n"
               "---@alias wxl.Guid string\n\n"
               "---An event's bytes, copied into a string.\n"
               "---@alias wxl.Bytes string\n\n"
               "---@class wxl.Vec3\n---@field x number\n---@field y number\n---@field z number\n\n";

        // --- the families -------------------------------------------------------------------
        out << "-- The script families. A function defined on one of these tables as On<Event> is\n"
               "-- subscribed when the entry file has run and called with the event's parameters;\n"
               "-- on an event that can be answered its return values are taken, as Override\n"
               "-- takes them.\n\n";
        for (const std::string& family : families)
        {
            const std::string stem = family.substr(0, family.size() - 6);
            out << "---@class " << family << "\n";
            for (uint32_t i = 0; i < count; ++i)
            {
                const Event& event = events[i];
                if (stem != event.family) continue;
                out << "---@field On" << event.name << " fun(self: " << family;
                if (!Parameters(event).empty()) out << ", " << Parameters(event);
                out << ")";
                if (Answerable(event))
                    out << ": " << Returns(event) << " answers through " << WritableNames(event);
                out << "\n";
            }
            out << family << " = {}\n\n";
        }

        // --- Observe, Override, Unobserve ---------------------------------------------------
        out << "---@alias wxl.Family ";
        for (size_t i = 0; i < families.size(); ++i) out << (i ? "|\"" : "\"") << families[i] << "\"";
        out << "\n\n";

        out << "---Calls `handler` with the event's parameters whenever it is raised. Returns the\n"
               "---handler's id.\n";
        for (uint32_t i = 0; i < count; ++i)
        {
            const Event& event = events[i];
            out << "---@overload fun(family: \"" << event.family << "Script\", method: \"On" << event.name
                << "\", handler: fun(" << Parameters(event) << ")): integer\n";
        }
        out << "---@param family wxl.Family\n---@param method string\n---@param handler function\n"
               "---@return integer id\nfunction Observe(family, method, handler) end\n\n";

        out << "---Like Observe, on an event that can be answered. The handler returns one value per\n"
               "---member it answers through, in member order: true sets an answer, a string fills a\n"
               "---buffer, nil leaves the member as it is. Taken on the main thread only.\n";
        for (uint32_t i = 0; i < count; ++i)
        {
            const Event& event = events[i];
            if (!Answerable(event)) continue;
            out << "---@overload fun(family: \"" << event.family << "Script\", method: \"On" << event.name
                << "\", handler: fun(" << Parameters(event) << "): " << Returns(event)
                << "): integer\n";
        }
        out << "---@param family wxl.Family\n---@param method string\n---@param handler function\n"
               "---@return integer id\nfunction Override(family, method, handler) end\n\n";

        out << "---Drops a handler by the id Observe or Override returned.\n"
               "---@param id integer\n---@return boolean dropped\nfunction Unobserve(id) end\n\n";

        // --- registerForEvent ---------------------------------------------------------------
        out << "---@alias wxl.CallbackName \"onInit\"|\"onUpdate\"|\"onDraw\"|\"onShutdown\"\n\n"
               "---Registers a callback: onInit at WXL_STAGE_READY (or right after a reload),\n"
               "---onUpdate(delta) at every World.Update, onDraw at every Render.UiRenderEnd,\n"
               "---onShutdown at WXL_STAGE_SHUTDOWN and before a reload.\n"
               "---@overload fun(event: \"onInit\", fn: fun())\n"
               "---@overload fun(event: \"onUpdate\", fn: fun(delta: number))\n"
               "---@overload fun(event: \"onDraw\", fn: fun())\n"
               "---@overload fun(event: \"onShutdown\", fn: fun())\n"
               "---@param event wxl.CallbackName\n---@param fn function\n"
               "function registerForEvent(event, fn) end\n\n";

        // --- objects ------------------------------------------------------------------------
        out << "---@alias wxl.ObjectType \"Item\"|\"Container\"|\"Unit\"|\"Player\"|\"GameObject\"|\"DynamicObject\"|\"Corpse\"|\"Unknown\"\n\n";
        out << "---A world object, by handle. Every getter answers the value, or nil and why:\n"
               "---\"gone\" for an object that left, \"unsupported\" for a runtime without the read, or\n"
               "---what the object is not (\"a GameObject has no Health\").\n"
               "---@class wxl.Object\n"
               "local Object = {}\n\n"
               "---Whether the handle still resolves.\n---@return boolean\nfunction Object:Valid() end\n\n"
               "---The server's id for the object, as \"0x\" and sixteen hex digits.\n---@return string|nil guid, string|nil reason\nfunction Object:Guid() end\n\n"
               "---@return wxl.ObjectType|nil type, string|nil reason\nfunction Object:Type() end\n\n"
               "---@return integer|nil type, string|nil reason\nfunction Object:TypeId() end\n\n"
               "---@return string|nil name, string|nil reason\nfunction Object:Name() end\n\n"
               "---What a unit targets; nil when nothing in view.\n---@return wxl.Object|nil target, string|nil reason\nfunction Object:Target() end\n\n"
               "---The handle itself, as \"0x\" and sixteen hex digits: for keys and logs.\n---@return string\nfunction Object:Handle() end\n\n";

        std::vector<Row> rows = {
#define WXL_OBJECT_READ(kinds, member, Method, type) Row{#member, wxl::lua::names::k##kinds, #type, false},
#define WXL_OBJECT_READ_AT(kinds, member, Method, type) Row{#member, wxl::lua::names::k##kinds, #type, true},
#include "wxl/runtime/object_reads.def"
#undef WXL_OBJECT_READ_AT
#undef WXL_OBJECT_READ
        };

        // Rows sharing a Lua name (Flags, DisplayId, ...) become one method, its kinds joined.
        std::vector<wxl::lua::names::Member> members;
        for (const Row& row : rows) members.push_back(wxl::lua::names::Member{row.member, row.kinds});
        const std::vector<std::string> luaNames = wxl::lua::names::LuaNames(members);

        std::vector<std::string>                       order;
        std::map<std::string, std::vector<const Row*>> byName;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            if (!byName.count(luaNames[i])) order.push_back(luaNames[i]);
            byName[luaNames[i]].push_back(&rows[i]);
        }
        for (const std::string& name : order)
        {
            const std::vector<const Row*>& group = byName[name];
            std::string                    kinds;
            for (const Row* row : group)
            {
                if (!kinds.empty()) kinds += "; ";
                kinds += wxl::lua::names::KindsText(row->kinds);
            }
            out << "---For " << kinds << " (" << group[0]->member;
            for (size_t i = 1; i < group.size(); ++i) out << ", " << group[i]->member;
            out << ").\n";
            if (group[0]->indexed) out << "---@param index integer\n";
            out << "---@return " << ValueType(group[0]->type) << "|nil value, string|nil reason\n";
            out << "function Object:" << name << "(" << (group[0]->indexed ? "index" : "") << ") end\n\n";
        }

        // --- the wxl table ------------------------------------------------------------------
        out << "---@class wxl.ModInfo\n---@field id string\n---@field name string\n---@field version string\n"
               "---@field directory string\n\n"
               "---@class wxl.RuntimeInfo\n---@field id string\n---@field name string\n---@field version integer\n"
               "---@field abi string the contract this mod is served, \"major.minor.feature\"\n"
               "---@field abi_major integer\n---@field abi_minor integer\n---@field abi_feature integer\n\n"
               "---@class wxl.object\n"
               "local object = {}\n\n"
               "---The object the local player controls, or nil while out of the world.\n"
               "---@return wxl.Object|nil\nfunction object.LocalPlayer() end\n\n"
               "---The object carrying a guid, or nil when it is not in view.\n"
               "---@param guid string|integer\n---@return wxl.Object|nil\nfunction object.FromGuid(guid) end\n\n"
               "---The objects of a type in view, or every object.\n"
               "---@param type? wxl.ObjectType|integer\n---@return wxl.Object[]\nfunction object.InView(type) end\n\n"
               "---@class wxl\n"
               "---@field mod wxl.ModInfo this mod, from its manifest or its folder\n"
               "---@field runtime wxl.RuntimeInfo the runtime that loaded wxl.lua\n"
               "---@field object wxl.object\n"
               "---@field events table<wxl.Family, string[]> every family's events, as On<Event>\n"
               "wxl = {}\n\n"
               "---Writes a line to the runtime's log, tagged with the mod's id.\n"
               "---@param level \"trace\"|\"debug\"|\"info\"|\"warn\"|\"error\"\n"
               "---@param ... any\nfunction wxl.log(level, ...) end\n";

        return out.str();
    }

    std::string ReadAll(const char* path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) return std::string();
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        std::string text = buffer.str();
        // Line endings are not a difference.
        std::string normalised;
        for (const char c : text)
            if (c != '\r') normalised += c;
        return normalised;
    }
} // namespace

int main(int argc, char** argv)
{
    const std::string text = Generate();

    if (argc == 1)
    {
        std::fwrite(text.data(), 1, text.size(), stdout);
        return 0;
    }
    if (argc == 3 && std::strcmp(argv[1], "--check") == 0)
    {
        if (ReadAll(argv[2]) == text)
        {
            std::printf("%s is what the generator writes\n", argv[2]);
            return 0;
        }
        std::printf("%s differs from what the generator writes; build wxl-lua-types-gen, or run "
                    "wxl-lua-types %s\n",
                    argv[2], argv[2]);
        return 1;
    }
    if (argc == 2)
    {
        if (ReadAll(argv[1]) == text) return 0;
        std::ofstream stream(argv[1], std::ios::binary);
        if (!stream)
        {
            std::printf("cannot write %s\n", argv[1]);
            return 1;
        }
        stream << text;
        std::printf("wrote %s\n", argv[1]);
        return 0;
    }
    std::printf("usage: wxl-lua-types [--check] [<file>]\n");
    return 2;
}
