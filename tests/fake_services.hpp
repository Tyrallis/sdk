// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// Service tables that keep their promises and count what is asked of them, so the C++ layer can
// be checked against the contract without a client: that a read through a stale handle is
// absent rather than zero, that a resource is released exactly once, that a typed call reaches
// the table with its arguments intact.

#ifndef WXL_TEST_FAKE_SERVICES_HPP
#define WXL_TEST_FAKE_SERVICES_HPP

#include <cstddef>
#include <cstring>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "wxl/services/hook.h"
#include "wxl/services/net.h"
#include "wxl/services/object.h"
#include "wxl/services/patch.h"
#include "wxl/services/script.h"
#include "wxl/services/storage.h"
#include "wxl/services/ui.h"

/// The opaque call of the script service, as this fake defines it.
struct wxl_script_call
{
    std::vector<wxl_value> args;
    std::vector<wxl_value> results;
    std::string            error;
};

namespace wxltest
{
    // --- wxl.object -------------------------------------------------------------------------
    //
    // Three objects: 42 the local player (dead), 43 a boar, 44 a chest. Anything else is stale.

    struct FakeObjects
    {
        static constexpr wxl_handle kPlayer = 42;
        static constexpr wxl_handle kBoar   = 43;
        static constexpr wxl_handle kChest  = 44;
        static constexpr wxl_handle kStale  = 7;

        static bool Live(wxl_handle h) { return h == kPlayer || h == kBoar || h == kChest; }
        static bool IsUnit(wxl_handle h) { return h == kPlayer || h == kBoar; }

        static wxl_bool WXL_CALL Valid(wxl_handle h) { return Live(h) ? WXL_TRUE : WXL_FALSE; }

        static wxl_guid WXL_CALL Guid(wxl_handle h)
        {
            if (h == kPlayer) return 0x100;
            if (h == kBoar) return 0x200;
            return WXL_GUID_NONE;
        }

        static uint32_t WXL_CALL Type(wxl_handle h)
        {
            if (h == kPlayer) return WXL_OBJECT_PLAYER;
            if (h == kBoar) return WXL_OBJECT_UNIT;
            if (h == kChest) return WXL_OBJECT_GAMEOBJECT;
            return WXL_OBJECT_UNKNOWN;
        }

        static wxl_handle WXL_CALL FromGuid(wxl_guid guid)
        {
            if (guid == 0x100) return kPlayer;
            if (guid == 0x200) return kBoar;
            return WXL_HANDLE_NONE;
        }

        static wxl_handle WXL_CALL LocalPlayer() { return kPlayer; }

        static wxl_result WXL_CALL Position(wxl_handle h, wxl_vec3* out)
        {
            if (!out) return WXL_E_INVALID;
            if (!Live(h)) return WXL_E_NOT_FOUND;
            *out = wxl_vec3{static_cast<float>(h), 2.0f, 3.0f};
            return WXL_OK;
        }

        static wxl_result WXL_CALL Facing(wxl_handle h, float* out)
        {
            if (!out) return WXL_E_INVALID;
            if (!Live(h)) return WXL_E_NOT_FOUND;
            *out = 1.5f;
            return WXL_OK;
        }

        static wxl_result WXL_CALL Name(wxl_handle h, wxl_str* out)
        {
            if (!out) return WXL_E_INVALID;
            if (!Live(h)) return WXL_E_NOT_FOUND;
            static const char* names[] = {"Thorgrim", "Boar", "Chest"};
            const char*        name    = names[h - kPlayer];
            *out                       = wxl_str{name, static_cast<uint32_t>(std::strlen(name))};
            return WXL_OK;
        }

        static wxl_result UnitRead(wxl_handle h, uint32_t* out, uint32_t player, uint32_t boar)
        {
            if (!out) return WXL_E_INVALID;
            if (!Live(h)) return WXL_E_NOT_FOUND;
            if (!IsUnit(h)) return WXL_E_INVALID;
            *out = h == kPlayer ? player : boar;
            return WXL_OK;
        }

        static wxl_result WXL_CALL Health(wxl_handle h, uint32_t* out) { return UnitRead(h, out, 0, 90); }
        static wxl_result WXL_CALL MaxHealth(wxl_handle h, uint32_t* out) { return UnitRead(h, out, 5000, 120); }
        static wxl_result WXL_CALL Level(wxl_handle h, uint32_t* out) { return UnitRead(h, out, 80, 3); }

        static wxl_result WXL_CALL Target(wxl_handle h, wxl_handle* out)
        {
            if (!out) return WXL_E_INVALID;
            if (!Live(h)) return WXL_E_NOT_FOUND;
            if (!IsUnit(h)) return WXL_E_INVALID;
            *out = h == kPlayer ? kBoar : WXL_HANDLE_NONE;
            return WXL_OK;
        }

        static uint32_t WXL_CALL Enumerate(uint32_t type, wxl_handle* out, uint32_t capacity)
        {
            uint32_t count = 0;
            for (wxl_handle h : {kPlayer, kBoar, kChest})
            {
                if (type != WXL_OBJECT_UNKNOWN && Type(h) != type) continue;
                if (out && count < capacity) out[count] = h;
                ++count;
            }
            return count;
        }

        static wxl_result WXL_CALL Positions(const wxl_handle* objects, uint32_t count, wxl_vec3* out)
        {
            if (!objects || !out) return WXL_E_INVALID;
            for (uint32_t i = 0; i < count; ++i)
                if (Position(objects[i], &out[i]) != WXL_OK) out[i] = wxl_vec3{0.0f, 0.0f, 0.0f};
            return WXL_OK;
        }

        static const wxl_object_api* Table()
        {
            static wxl_object_api api = [] {
                wxl_object_api t{};
                t.struct_size     = sizeof(wxl_object_api);
                t.valid           = &Valid;
                t.guid            = &Guid;
                t.type            = &Type;
                t.from_guid       = &FromGuid;
                t.local_player    = &LocalPlayer;
                t.position        = &Position;
                t.facing          = &Facing;
                t.name            = &Name;
                t.unit_health     = &Health;
                t.unit_max_health = &MaxHealth;
                t.unit_level      = &Level;
                t.unit_target     = &Target;
                t.enumerate       = &Enumerate;
                t.positions       = &Positions;
                return t;
            }();
            return &api;
        }
    };

    // --- wxl.script -------------------------------------------------------------------------

    struct FakeScript
    {
        struct Registered
        {
            wxl_script_fn fn;
            void*         user;
        };

        static std::map<std::string, Registered>& Functions()
        {
            static std::map<std::string, Registered> functions;
            return functions;
        }

        static std::vector<std::string>& Events()
        {
            static std::vector<std::string> events;
            return events;
        }

        static std::vector<std::string>& Chunks()
        {
            static std::vector<std::string> chunks;
            return chunks;
        }

        static uint32_t WXL_CALL Context() { return WXL_SCRIPT_CONTEXT_GAME; }

        static wxl_result WXL_CALL RegisterFunction(const char* name, wxl_script_fn fn, void* user)
        {
            if (!name || !fn) return WXL_E_INVALID;
            if (Functions().count(name)) return WXL_E_INVALID;
            Functions()[name] = Registered{fn, user};
            return WXL_OK;
        }

        static wxl_result WXL_CALL RegisterEvent(const char* name)
        {
            Events().push_back(name);
            return WXL_OK;
        }

        static wxl_result WXL_CALL FireEvent(const char* name, const wxl_value* args, uint32_t count)
        {
            std::string line = name;
            for (uint32_t i = 0; i < count; ++i)
            {
                line += args[i].type == WXL_VALUE_NUMBER ? " n" : args[i].type == WXL_VALUE_STRING ? " s"
                                                           : args[i].type == WXL_VALUE_BOOL     ? " b"
                                                                                                : " -";
            }
            Events().push_back(line);
            return WXL_OK;
        }

        static wxl_result WXL_CALL Run(wxl_str chunk, const char* name)
        {
            Chunks().push_back(std::string(name) + ": " + std::string(chunk.data, chunk.size));
            return WXL_OK;
        }

        static uint32_t WXL_CALL ArgCount(wxl_script_call* call)
        {
            return static_cast<uint32_t>(call->args.size());
        }

        static const wxl_value* Arg(wxl_script_call* call, uint32_t index, uint32_t type)
        {
            if (index >= call->args.size() || call->args[index].type != type) return nullptr;
            return &call->args[index];
        }

        static wxl_bool WXL_CALL ArgNumber(wxl_script_call* call, uint32_t index, double* out)
        {
            const wxl_value* value = Arg(call, index, WXL_VALUE_NUMBER);
            if (!value) return WXL_FALSE;
            *out = value->as.number;
            return WXL_TRUE;
        }

        static wxl_bool WXL_CALL ArgString(wxl_script_call* call, uint32_t index, wxl_str* out)
        {
            const wxl_value* value = Arg(call, index, WXL_VALUE_STRING);
            if (!value) return WXL_FALSE;
            *out = value->as.string;
            return WXL_TRUE;
        }

        static wxl_bool WXL_CALL ArgBool(wxl_script_call* call, uint32_t index, wxl_bool* out)
        {
            const wxl_value* value = Arg(call, index, WXL_VALUE_BOOL);
            if (!value) return WXL_FALSE;
            *out = value->as.boolean;
            return WXL_TRUE;
        }

        static uint32_t WXL_CALL ArgType(wxl_script_call* call, uint32_t index)
        {
            return index < call->args.size() ? call->args[index].type : WXL_VALUE_NIL;
        }

        static void WXL_CALL PushNumber(wxl_script_call* call, double value)
        {
            wxl_value v{};
            v.type      = WXL_VALUE_NUMBER;
            v.as.number = value;
            call->results.push_back(v);
        }

        static void WXL_CALL PushString(wxl_script_call* call, wxl_str value)
        {
            wxl_value v{};
            v.type      = WXL_VALUE_STRING;
            v.as.string = value;
            call->results.push_back(v);
        }

        static void WXL_CALL PushBool(wxl_script_call* call, wxl_bool value)
        {
            wxl_value v{};
            v.type       = WXL_VALUE_BOOL;
            v.as.boolean = value;
            call->results.push_back(v);
        }

        static void WXL_CALL PushNil(wxl_script_call* call)
        {
            wxl_value v{};
            v.type = WXL_VALUE_NIL;
            call->results.push_back(v);
        }

        static void WXL_CALL Fail(wxl_script_call* call, wxl_str message)
        {
            call->error = std::string(message.data, message.size);
        }

        /// Invokes a registered function the way the VM would.
        static bool Invoke(const char* name, wxl_script_call& call)
        {
            const auto found = Functions().find(name);
            if (found == Functions().end()) return false;
            found->second.fn(&call, found->second.user);
            return true;
        }

        static const wxl_script_api* Table()
        {
            static wxl_script_api api = [] {
                wxl_script_api t{};
                t.struct_size       = sizeof(wxl_script_api);
                t.context           = &Context;
                t.register_function = &RegisterFunction;
                t.register_event    = &RegisterEvent;
                t.fire_event        = &FireEvent;
                t.run               = &Run;
                t.arg_count         = &ArgCount;
                t.arg_number        = &ArgNumber;
                t.arg_string        = &ArgString;
                t.arg_bool          = &ArgBool;
                t.arg_type          = &ArgType;
                t.push_number       = &PushNumber;
                t.push_string       = &PushString;
                t.push_bool         = &PushBool;
                t.push_nil          = &PushNil;
                t.fail              = &Fail;
                return t;
            }();
            return &api;
        }
    };

    // --- wxl.storage ------------------------------------------------------------------------

    struct FakeStorage
    {
        static std::map<std::string, std::string>& Files()
        {
            static std::map<std::string, std::string> files = {
                {"Interface\\hello.txt", "hello"},
                {"Interface\\big.bin", std::string(1000, 'x')},
            };
            return files;
        }

        static int& Opens()
        {
            static int opens = 0;
            return opens;
        }

        static int& Closes()
        {
            static int closes = 0;
            return closes;
        }

        static std::string Key(wxl_str path) { return std::string(path.data, path.size); }

        static wxl_bool WXL_CALL Exists(wxl_str path)
        {
            return Files().count(Key(path)) ? WXL_TRUE : WXL_FALSE;
        }

        static wxl_result WXL_CALL Size(wxl_str path, uint64_t* out)
        {
            const auto found = Files().find(Key(path));
            if (found == Files().end()) return WXL_E_NOT_FOUND;
            *out = found->second.size();
            return WXL_OK;
        }

        static wxl_result WXL_CALL Read(wxl_str path, void* buffer, uint64_t capacity, uint64_t* out_read)
        {
            const auto found = Files().find(Key(path));
            if (found == Files().end()) return WXL_E_NOT_FOUND;
            *out_read = found->second.size();
            if (capacity < found->second.size()) return WXL_E_INVALID;
            std::memcpy(buffer, found->second.data(), found->second.size());
            return WXL_OK;
        }

        static wxl_handle WXL_CALL Open(wxl_str path)
        {
            const auto found = Files().find(Key(path));
            if (found == Files().end()) return WXL_HANDLE_NONE;
            ++Opens();
            return static_cast<wxl_handle>(std::distance(Files().begin(), found) + 1);
        }

        static wxl_result WXL_CALL ReadAt(wxl_handle file, uint64_t offset, void* buffer, uint64_t size,
                                          uint64_t* out_read)
        {
            if (file == WXL_HANDLE_NONE || file > Files().size()) return WXL_E_NOT_FOUND;
            auto it = Files().begin();
            std::advance(it, static_cast<std::ptrdiff_t>(file - 1));
            const std::string& data = it->second;
            if (offset >= data.size())
            {
                *out_read = 0;
                return WXL_OK;
            }
            const uint64_t n = data.size() - offset < size ? data.size() - offset : size;
            std::memcpy(buffer, data.data() + offset, static_cast<size_t>(n));
            *out_read = n;
            return WXL_OK;
        }

        static void WXL_CALL Close(wxl_handle) { ++Closes(); }

        static wxl_result WXL_CALL AddSearchPath(wxl_str, int32_t) { return WXL_OK; }
        static wxl_result WXL_CALL MountArchive(wxl_str, int32_t) { return WXL_OK; }

        static uint32_t WXL_CALL Find(wxl_str pattern, wxl_str* out, uint32_t capacity)
        {
            const std::string prefix = Key(pattern).substr(0, Key(pattern).find('*'));
            uint32_t          count  = 0;
            for (const auto& entry : Files())
            {
                if (entry.first.compare(0, prefix.size(), prefix) != 0) continue;
                if (out && count < capacity)
                    out[count] = wxl_str{entry.first.c_str(), static_cast<uint32_t>(entry.first.size())};
                ++count;
            }
            return count;
        }

        static const wxl_storage_api* Table()
        {
            static wxl_storage_api api = [] {
                wxl_storage_api t{};
                t.struct_size     = sizeof(wxl_storage_api);
                t.exists          = &Exists;
                t.size            = &Size;
                t.read            = &Read;
                t.open            = &Open;
                t.read_at         = &ReadAt;
                t.close           = &Close;
                t.add_search_path = &AddSearchPath;
                t.mount_archive   = &MountArchive;
                t.find            = &Find;
                return t;
            }();
            return &api;
        }
    };

    // --- wxl.net ----------------------------------------------------------------------------

    struct FakeNet
    {
        static std::set<uint32_t>& Claimed()
        {
            static std::set<uint32_t> claimed;
            return claimed;
        }

        static int& Releases()
        {
            static int releases = 0;
            return releases;
        }

        static std::vector<std::pair<uint32_t, uint32_t>>& Sent()
        {
            static std::vector<std::pair<uint32_t, uint32_t>> sent;
            return sent;
        }

        static uint32_t WXL_CALL State() { return WXL_NET_AUTHENTICATED; }

        static wxl_result WXL_CALL Send(uint32_t opcode, const void*, uint32_t size)
        {
            Sent().emplace_back(opcode, size);
            return WXL_OK;
        }

        static wxl_result WXL_CALL Claim(uint32_t opcode)
        {
            if (Claimed().count(opcode)) return WXL_E_INVALID;
            Claimed().insert(opcode);
            return WXL_OK;
        }

        static wxl_result WXL_CALL Release(uint32_t opcode)
        {
            ++Releases();
            return Claimed().erase(opcode) ? WXL_OK : WXL_E_NOT_FOUND;
        }

        static uint32_t WXL_CALL ReservedLimit() { return 0x51F; }

        static const wxl_net_api* Table()
        {
            static wxl_net_api api = [] {
                wxl_net_api t{};
                t.struct_size           = sizeof(wxl_net_api);
                t.state                 = &State;
                t.send                  = &Send;
                t.claim_opcode          = &Claim;
                t.release_opcode        = &Release;
                t.reserved_opcode_limit = &ReservedLimit;
                return t;
            }();
            return &api;
        }
    };

    // --- wxl.ui -----------------------------------------------------------------------------

    struct FakeUi
    {
        struct Registered
        {
            wxl_panel_fn fn;
            void*        user;
        };

        static std::map<std::string, Registered>& Panels()
        {
            static std::map<std::string, Registered> panels;
            return panels;
        }

        static int& Removes()
        {
            static int removes = 0;
            return removes;
        }

        static std::vector<std::string>& Drawn()
        {
            static std::vector<std::string> drawn;
            return drawn;
        }

        static wxl_result WXL_CALL AddPanel(const char* title, wxl_panel_fn fn, void* user)
        {
            if (Panels().count(title)) return WXL_E_INVALID;
            Panels()[title] = Registered{fn, user};
            return WXL_OK;
        }

        static wxl_result WXL_CALL RemovePanel(const char* title)
        {
            ++Removes();
            return Panels().erase(title) ? WXL_OK : WXL_E_NOT_FOUND;
        }

        static wxl_bool WXL_CALL IsOpen() { return WXL_TRUE; }
        static void WXL_CALL Text(wxl_str text) { Drawn().push_back(std::string(text.data, text.size)); }
        static void WXL_CALL Separator() { Drawn().push_back("---"); }
        static void WXL_CALL SameLine() {}
        static wxl_bool WXL_CALL Button(const char* label)
        {
            Drawn().push_back(std::string("[") + label + "]");
            return WXL_TRUE;
        }
        static wxl_bool WXL_CALL Checkbox(const char*, wxl_bool* value)
        {
            *value = *value ? WXL_FALSE : WXL_TRUE;
            return WXL_TRUE;
        }
        static wxl_bool WXL_CALL SliderFloat(const char*, float* value, float, float max)
        {
            *value = max;
            return WXL_TRUE;
        }
        static wxl_bool WXL_CALL SliderInt(const char*, int32_t* value, int32_t, int32_t max)
        {
            *value = max;
            return WXL_TRUE;
        }
        static wxl_bool WXL_CALL ColorEdit(const char*, float rgba[4])
        {
            rgba[3] = 0.5f;
            return WXL_TRUE;
        }
        static wxl_bool WXL_CALL Combo(const char*, int32_t* index, const char* const*, int32_t count)
        {
            *index = count - 1;
            return WXL_TRUE;
        }
        static wxl_bool WXL_CALL CollapsingHeader(const char*) { return WXL_TRUE; }
        static wxl_bool WXL_CALL InputText(const char*, char* buffer, uint32_t capacity)
        {
            const char* typed = "typed";
            uint32_t    i     = 0;
            for (; typed[i] && i + 1 < capacity; ++i) buffer[i] = typed[i];
            buffer[i] = '\0';
            return WXL_TRUE;
        }

        /// Draws every panel, as the overlay would.
        static void DrawAll()
        {
            for (auto& entry : Panels()) entry.second.fn(entry.second.user);
        }

        static wxl_ui_api Make()
        {
            wxl_ui_api t{};
            t.struct_size       = sizeof(wxl_ui_api);
            t.add_panel         = &AddPanel;
            t.is_open           = &IsOpen;
            t.text              = &Text;
            t.separator         = &Separator;
            t.same_line         = &SameLine;
            t.button            = &Button;
            t.checkbox          = &Checkbox;
            t.slider_float      = &SliderFloat;
            t.slider_int        = &SliderInt;
            t.color_edit        = &ColorEdit;
            t.combo             = &Combo;
            t.collapsing_header = &CollapsingHeader;
            t.input_text        = &InputText;
            t.remove_panel      = &RemovePanel;
            return t;
        }

        static const wxl_ui_api* Table()
        {
            static wxl_ui_api api = Make();
            return &api;
        }

        /// A table from a runtime built before remove_panel was appended.
        static const wxl_ui_api* OlderTable()
        {
            static wxl_ui_api api = [] {
                wxl_ui_api t   = Make();
                t.struct_size  = offsetof(wxl_ui_api, remove_panel);
                t.remove_panel = nullptr;
                return t;
            }();
            return &api;
        }
    };

    // --- wxl.hook and wxl.hook.raw ----------------------------------------------------------

    struct FakeHooks
    {
        static int& Attaches()
        {
            static int attaches = 0;
            return attaches;
        }

        static int& Detaches()
        {
            static int detaches = 0;
            return detaches;
        }

        static std::set<wxl_hook>& Live()
        {
            static std::set<wxl_hook> live;
            return live;
        }

        static void* WXL_CALL Engine() { return nullptr; }

        static uint32_t WXL_CALL Points(wxl_hook_point* out, uint32_t capacity)
        {
            if (out && capacity >= 1)
            {
                out[0].name      = wxl_str{"Thing", 5};
                out[0].signature = wxl_str{"int(int)", 8};
            }
            return 1;
        }

        static wxl_bool WXL_CALL HasPoint(const char* name)
        {
            return std::strcmp(name, "Thing") == 0 ? WXL_TRUE : WXL_FALSE;
        }

        static wxl_hook WXL_CALL Attach(const char* name, void* detour, void** original, int32_t)
        {
            if (!HasPoint(name) || !detour) return WXL_HOOK_NONE;
            ++Attaches();
            if (original) *original = reinterpret_cast<void*>(&Engine);
            const wxl_hook hook = static_cast<wxl_hook>(Attaches());
            Live().insert(hook);
            return hook;
        }

        static wxl_hook WXL_CALL AttachAt(const char*, uintptr_t target, void* detour, void** original,
                                          int32_t)
        {
            if (!target || !detour) return WXL_HOOK_NONE;
            ++Attaches();
            if (original) *original = reinterpret_cast<void*>(&Engine);
            const wxl_hook hook = static_cast<wxl_hook>(Attaches());
            Live().insert(hook);
            return hook;
        }

        static wxl_result WXL_CALL Detach(wxl_hook hook)
        {
            ++Detaches();
            return Live().erase(hook) ? WXL_OK : WXL_E_NOT_FOUND;
        }

        static uintptr_t WXL_CALL ImageBase() { return 0x400000; }
        static uint32_t WXL_CALL ImageBuild() { return 12340; }

        static const wxl_hook_api* Table()
        {
            static wxl_hook_api api = [] {
                wxl_hook_api t{};
                t.struct_size = sizeof(wxl_hook_api);
                t.points      = &Points;
                t.has_point   = &HasPoint;
                t.attach      = &Attach;
                t.detach      = &Detach;
                return t;
            }();
            return &api;
        }

        static const wxl_hook_raw_api* RawTable()
        {
            static wxl_hook_raw_api api = [] {
                wxl_hook_raw_api t{};
                t.struct_size = sizeof(wxl_hook_raw_api);
                t.image_base  = &ImageBase;
                t.image_build = &ImageBuild;
                t.attach_at   = &AttachAt;
                t.detach      = &Detach;
                return t;
            }();
            return &api;
        }
    };

    // --- wxl.patch --------------------------------------------------------------------------
    //
    // An image of sixteen bytes at address 0x1000, journaled the way a runtime would.

    struct FakePatch
    {
        static constexpr uintptr_t kBase = 0x1000;

        struct Journal
        {
            uintptr_t            address;
            std::vector<uint8_t> saved;
        };

        static std::vector<uint8_t>& Image()
        {
            static std::vector<uint8_t> image(16, 0x90);
            return image;
        }

        static std::map<wxl_patch, Journal>& Live()
        {
            static std::map<wxl_patch, Journal> live;
            return live;
        }

        static std::vector<wxl_patch>& RestoreOrder()
        {
            static std::vector<wxl_patch> order;
            return order;
        }

        static int& Applies()
        {
            static int applies = 0;
            return applies;
        }

        static bool InImage(uintptr_t address, uint32_t size)
        {
            return address >= kBase && address + size <= kBase + Image().size();
        }

        static wxl_patch WXL_CALL Apply(const char* name, uintptr_t address, const void* expected,
                                        const void* bytes, uint32_t size)
        {
            if (!name || !expected || !bytes || !InImage(address, size)) return WXL_PATCH_NONE;
            uint8_t* at = Image().data() + (address - kBase);
            if (std::memcmp(at, expected, size) != 0) return WXL_PATCH_NONE;
            Journal journal{address, std::vector<uint8_t>(at, at + size)};
            std::memcpy(at, bytes, size);
            const wxl_patch patch = static_cast<wxl_patch>(++Applies());
            Live()[patch]         = std::move(journal);
            return patch;
        }

        static wxl_result WXL_CALL Restore(wxl_patch patch)
        {
            RestoreOrder().push_back(patch);
            const auto found = Live().find(patch);
            if (found == Live().end()) return WXL_E_NOT_FOUND;
            std::memcpy(Image().data() + (found->second.address - kBase), found->second.saved.data(),
                        found->second.saved.size());
            Live().erase(found);
            return WXL_OK;
        }

        static wxl_bool WXL_CALL Matches(uintptr_t address, const void* expected, uint32_t size)
        {
            if (!InImage(address, size)) return WXL_FALSE;
            return std::memcmp(Image().data() + (address - kBase), expected, size) == 0 ? WXL_TRUE
                                                                                         : WXL_FALSE;
        }

        static uintptr_t WXL_CALL Find(const void* pattern, uint32_t size, uintptr_t after)
        {
            const std::vector<uint8_t>& image = Image();
            for (size_t i = after ? after - kBase + 1 : 0; i + size <= image.size(); ++i)
                if (std::memcmp(image.data() + i, pattern, size) == 0) return kBase + i;
            return 0;
        }

        static const wxl_patch_api* Table()
        {
            static wxl_patch_api api = [] {
                wxl_patch_api t{};
                t.struct_size = sizeof(wxl_patch_api);
                t.apply       = &Apply;
                t.restore     = &Restore;
                t.matches     = &Matches;
                t.find        = &Find;
                return t;
            }();
            return &api;
        }
    };
} // namespace wxltest

#endif // WXL_TEST_FAKE_SERVICES_HPP
