// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl-object-members: the members of the C++ object facades, and the guides' tables of object
// members, from object_reads.def.
//
//   wxl-object-members <sdk>            writes include/wxl/objects/generated/*.inc and the tables
//                                       between the markers of docs/building-a-mod.md and
//                                       docs/lua-mods.md
//   wxl-object-members --check <sdk>    says which of them differs from what it would write
//
// One row is one method of one facade class, documented from the row, and one line of the
// table of its kind in each guide, under its C++ name and under its Lua name.

#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <tuple>
#include <string>
#include <vector>

#include "ObjectNames.hpp"
#include "wxl/reflect.hpp"

namespace
{
    using wxl::lua::names::RowInfo;

    /// A facade class: its name, its file, the kinds tokens whose rows it carries.
    struct Class
    {
        const char* name;
        const char* file;
        const char* kinds[2];
        const char* heading; ///< for the guides' tables
    };

    const Class kClasses[] = {
        {"Object", "object", {"Any", nullptr}, "Every object"},
        {"Unit", "unit", {"Units", nullptr}, "Units, players included"},
        {"Player", "player", {"Players", "Active"}, "Players"},
        {"Item", "item", {"Items", nullptr}, "Items, containers included"},
        {"Container", "container", {"Containers", nullptr}, "Containers"},
        {"GameObject", "game_object", {"GameObjects", nullptr}, "Game objects"},
        {"DynamicObject", "dynamic_object", {"Dynamics", nullptr}, "Dynamic objects"},
        {"Corpse", "corpse", {"Corpses", nullptr}, "Corpses"},
    };

    bool Carries(const Class& c, const RowInfo& row)
    {
        for (const char* kinds : c.kinds)
            if (kinds && std::strcmp(kinds, row.kinds_name) == 0) return true;
        return false;
    }

    bool Is(const char* a, const char* b) { return std::strcmp(a, b) == 0; }

    bool Active(const RowInfo& row) { return Is(row.kinds_name, "Active"); }

    bool Newer(const RowInfo& row)
    {
        return row.since[0] != 0 || row.since[1] != 1 || row.since[2] != 5;
    }

    std::string Since(const RowInfo& row)
    {
        return std::to_string(row.since[0]) + "." + std::to_string(row.since[1]) + "." +
               std::to_string(row.since[2]);
    }

    /// The name of a row's argument, in the facade and the guides.
    std::string ArgName(const RowInfo& row)
    {
        static const std::map<std::string, std::string> names = {
            {"distance", "other"},          {"distance_to", "point"},
            {"unit_reaction_to", "other"},  {"unit_power", "power"},
            {"unit_max_power", "power"},    {"unit_aura", "slot"},
            {"unit_has_aura", "spell_id"},  {"unit_knows_spell", "spell_id"},
            {"unit_speed", "type"},         {"player_visible_item", "slot"},
            {"player_inventory_item", "slot"}, {"player_backpack_item", "slot"},
            {"player_skill", "line"},       {"player_skill_of", "skill_id"},
            {"player_party_member_guid", "slot"}, {"player_raid_member_guid", "slot"},
            {"item_enchant", "slot"},       {"container_slot", "slot"},
            {"container_slot_item", "slot"},
        };
        const auto found = names.find(row.member);
        return found == names.end() ? "value" : found->second;
    }

    /// "wxl.Reaction" for wxl_reaction, as wxl-lua-types names an enumeration's alias.
    std::string AliasOf(const char* type)
    {
        std::string alias = "wxl.";
        if (std::strncmp(type, "wxl_", 4) == 0) type += 4;
        bool upper = true;
        for (; *type; ++type)
        {
            if (*type == '_')
            {
                upper = true;
                continue;
            }
            alias += upper && *type >= 'a' && *type <= 'z' ? static_cast<char>(*type - 'a' + 'A') : *type;
            upper = false;
        }
        return alias;
    }

    bool IsEnum(const char* cxx) { return wxl::reflect::FindEnum(cxx) != nullptr; }

    /// What a Lua method answers, as the editor definitions write it.
    std::string LuaAnswer(const RowInfo& row)
    {
        if (IsEnum(row.cxx)) return AliasOf(row.cxx);
        if (Is(row.type, "wxl_bool")) return "boolean";
        if (Is(row.type, "wxl_handle")) return "wxl.Object";
        if (Is(row.type, "wxl_guid")) return "wxl.Guid";
        if (Is(row.type, "wxl_vec3")) return "wxl.Vec3";
        if (Is(row.type, "wxl_aura")) return "wxl.Aura";
        if (Is(row.type, "wxl_skill")) return "wxl.Skill";
        if (Is(row.type, "float")) return "number";
        return "integer";
    }

    /// The lines a row's documentation adds to its own: who may ask, from which abi.
    std::vector<std::string> Notes(const RowInfo& row, bool lua)
    {
        std::vector<std::string> notes;
        if (Active(row))
            notes.push_back(lua ? "Only the active player has it: \"invalid\" for another."
                                : "The active player only: WXL_E_INVALID for another player.");
        if (Is(row.source, "cache"))
            notes.push_back(lua ? "\"failed\" until the client has the item's record from the server."
                                : "WXL_E_FAILED until the client has the item's record from the server.");
        if (Newer(row))
            notes.push_back(lua ? "Came with abi " + Since(row) + "."
                                : "Came with abi " + Since(row) + ": WXL_E_UNSUPPORTED on an older runtime.");
        return notes;
    }

    /// The parameter as the facade declares it.
    std::string CxxParam(const RowInfo& row)
    {
        const std::string type = row.arg;
        return type + " " + ArgName(row);
    }

    /// The argument as the getter takes it: an object as its handle, a number or an
    /// enumeration as a uint32_t, a point as it is.
    std::string ArgToC(const RowInfo& row)
    {
        if (Is(row.arg_c, "wxl_handle")) return ArgName(row) + ".Handle()";
        if (Is(row.arg_c, "uint32_t")) return "static_cast<uint32_t>(" + ArgName(row) + ")";
        return ArgName(row);
    }

    std::string Header(const Class& c)
    {
        std::ostringstream out;
        out << "// SPDX-License-Identifier: MIT\n// Copyright (c) 2026 WarcraftXL\n//\n"
            << "// The members of wxl::" << c.name
            << " that read the object, one per row of object_reads.def: written by\n"
            << "// wxl-object-members (sdk/tools/objects), not by hand. Change the table and run the tool.\n"
            << "//\n// Included inside the class, in its public part.\n";
        for (const RowInfo& row : wxl::lua::names::Rows())
        {
            if (!Carries(c, row)) continue;
            out << "\n        /// " << row.doc << "\n";
            for (const std::string& note : Notes(row, false)) out << "        /// " << note << "\n";
            if (!*row.arg_c)
                out << "        Expected<" << row.cxx << "> " << row.cxx_name << "() const { return ReadAs<"
                    << row.cxx << ">(&wxl_object_api::" << row.member << "); }\n";
            else
                out << "        Expected<" << row.cxx << "> " << row.cxx_name << "(" << CxxParam(row)
                    << ") const\n        {\n            return ReadAtAs<" << row.cxx << ">(&wxl_object_api::"
                    << row.member << ", " << ArgToC(row) << ");\n        }\n";
        }
        return out.str();
    }

    std::string Cell(std::string text)
    {
        std::string out;
        for (char ch : text) out += ch == '|' ? std::string("\\|") : std::string(1, ch);
        return out;
    }

    std::string CxxTable()
    {
        std::ostringstream out;
        for (const Class& c : kClasses)
        {
            out << "\n**`wxl::" << c.name << "`**: " << c.heading << "\n\n| Method | Answers | What |\n|---|---|---|\n";
            for (const RowInfo& row : wxl::lua::names::Rows())
            {
                if (!Carries(c, row)) continue;
                std::string what = row.doc;
                for (const std::string& note : Notes(row, false)) what += " " + note;
                out << "| `" << row.cxx_name << "(" << (*row.arg_c ? CxxParam(row) : "") << ")` | `"
                    << row.cxx << "` | " << Cell(what) << " |\n";
            }
        }
        return out.str();
    }

    std::string LuaTable()
    {
        const std::vector<RowInfo>&     rows  = wxl::lua::names::Rows();
        const std::vector<std::string>& names = wxl::lua::names::RowNames();
        std::ostringstream              out;
        for (const Class& c : kClasses)
        {
            out << "\n**" << c.heading << "**\n\n| Method | Answers | What |\n|---|---|---|\n";
            for (size_t i = 0; i < rows.size(); ++i)
            {
                const RowInfo& row = rows[i];
                if (!Carries(c, row)) continue;
                std::string what = row.doc;
                for (const std::string& note : Notes(row, true)) what += " " + note;
                const std::string arg = *row.arg_c ? ArgName(row) : "";
                out << "| `" << names[i] << "(" << arg << ")` | `" << Cell(LuaAnswer(row)) << "` | "
                    << Cell(what) << " |\n";
            }
        }
        return out.str();
    }

    std::string ReadAll(const std::string& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) return std::string();
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        std::string text;
        for (char ch : buffer.str())
            if (ch != '\r') text += ch;
        return text;
    }

    /// The text of a guide with the block between its markers replaced; empty when a marker is
    /// missing.
    std::string Spliced(const std::string& text, const std::string& name, const std::string& block)
    {
        const std::string begin = "<!-- object-members:" + name + " (written by wxl-object-members) -->";
        const std::string end   = "<!-- /object-members:" + name + " -->";
        const size_t      from  = text.find(begin);
        const size_t      to    = text.find(end);
        if (from == std::string::npos || to == std::string::npos || to < from) return std::string();
        return text.substr(0, from + begin.size()) + "\n" + block + "\n" + text.substr(to);
    }

    struct Output
    {
        std::string path;
        std::string text;
    };
} // namespace

int main(int argc, char** argv)
{
    const bool check = argc == 3 && std::strcmp(argv[1], "--check") == 0;
    if (!check && argc != 2)
    {
        std::printf("usage: wxl-object-members [--check] <sdk>\n");
        return 2;
    }
    const std::string sdk = argv[argc - 1];

    const std::string clash = wxl::lua::names::Clash(
        [] {
            std::vector<wxl::lua::names::Member> members;
            for (const RowInfo& row : wxl::lua::names::Rows())
                members.push_back(wxl::lua::names::Member{row.member, row.kinds});
            return members;
        }(),
        wxl::lua::names::RowNames());
    if (!clash.empty())
    {
        std::printf("the object methods' Lua names are ambiguous: %s\n", clash.c_str());
        return 1;
    }

    std::vector<Output> outputs;
    for (const Class& c : kClasses)
        outputs.push_back(Output{sdk + "/include/wxl/objects/generated/" + c.file + ".inc", Header(c)});
    for (const auto& [guide, name, table] :
         {std::make_tuple("docs/building-a-mod.md", "cxx", CxxTable()), std::make_tuple("docs/lua-mods.md", "lua", LuaTable())})
    {
        const std::string path = sdk + "/" + guide;
        const std::string text = Spliced(ReadAll(path), name, table);
        if (text.empty())
        {
            std::printf("%s has no object-members:%s markers\n", path.c_str(), name);
            return 1;
        }
        outputs.push_back(Output{path, text});
    }

    int differ = 0;
    for (const Output& output : outputs)
    {
        if (ReadAll(output.path) == output.text) continue;
        if (check)
        {
            std::printf("%s differs from what wxl-object-members writes; run it on the sdk\n", output.path.c_str());
            ++differ;
            continue;
        }
        std::ofstream stream(output.path, std::ios::binary);
        if (!stream)
        {
            std::printf("cannot write %s\n", output.path.c_str());
            return 1;
        }
        stream << output.text;
        std::printf("wrote %s\n", output.path.c_str());
    }
    if (check && !differ) std::printf("the object members are what object_reads.def says\n");
    return differ ? 1 : 0;
}
