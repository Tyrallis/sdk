// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 WarcraftXL

#include "Args.hpp"

#include <cstring>
#include <string>

#include "Objects.hpp"

namespace wxl::lua
{
    namespace
    {
        constexpr const char* kArgsMeta = "wxl.args";

        template <class T>
        T ReadAt(const void* args, uint32_t offset)
        {
            T value;
            std::memcpy(&value, static_cast<const uint8_t*>(args) + offset, sizeof value);
            return value;
        }

        template <class T>
        void WriteAt(void* args, uint32_t offset, const T& value)
        {
            std::memcpy(static_cast<uint8_t*>(args) + offset, &value, sizeof value);
        }

        /// The extent of a bytes or buffer member: the u32 member its row names.
        uint32_t ExtentOf(const reflect::Field& field, const void* args)
        {
            return field.extent ? ReadAt<uint32_t>(args, field.extent_offset) : 0;
        }

        const reflect::Field* FirstOfKind(const reflect::Event& event, reflect::Kind kind)
        {
            for (uint32_t i = 0; i < event.field_count; ++i)
                if (event.fields[i].kind == kind) return &event.fields[i];
            return nullptr;
        }

        ArgsBox* CheckBox(lua_State* L, int index)
        {
            return static_cast<ArgsBox*>(luaL_checkudata(L, index, kArgsMeta));
        }

        /// The box, with its args still valid.
        ArgsBox* LiveBox(lua_State* L, int index)
        {
            ArgsBox* box = CheckBox(L, index);
            if (!box->args)
                luaL_error(L, "the args of On%s are only valid while the handler runs",
                           box->event->name);
            return box;
        }

        std::string MemberList(const reflect::Event& event)
        {
            std::string names;
            for (uint32_t i = 0; i < event.field_count; ++i)
            {
                if (!names.empty()) names += ", ";
                names += event.fields[i].name;
            }
            return names;
        }

        int ArgsIndex(lua_State* L)
        {
            ArgsBox*    box  = LiveBox(L, 1);
            const char* name = luaL_checkstring(L, 2);
            if (const reflect::Field* field = reflect::FieldOf(*box->event, name))
            {
                PushField(L, *field, box->args);
                return 1;
            }
            return luaL_error(L, "On%s args have no member '%s'; the members are %s",
                              box->event->name, name, MemberList(*box->event).c_str());
        }

        int ArgsNewIndex(lua_State* L)
        {
            ArgsBox*              box   = LiveBox(L, 1);
            const char*           name  = luaL_checkstring(L, 2);
            const reflect::Field* field = reflect::FieldOf(*box->event, name);
            if (!field)
                return luaL_error(L, "On%s args have no member '%s'; the members are %s",
                                  box->event->name, name, MemberList(*box->event).c_str());
            if (field->kind != reflect::Kind::answer && field->kind != reflect::Kind::buffer)
                return luaL_error(L, "On%s args: '%s' is read-only", box->event->name, name);
            if (!box->writable)
                return luaL_error(L, "On%s args: '%s' can only be written from an Override "
                                     "handler on the main thread",
                                  box->event->name, name);

            if (field->kind == reflect::Kind::answer)
            {
                if (!lua_isboolean(L, 3))
                    return luaL_error(L, "On%s args: '%s' takes a boolean", box->event->name, name);
                // WXL_TRUE is a request, honoured unless another handler already decided;
                // WXL_FALSE is never written back.
                if (lua_toboolean(L, 3) && !WriteAnswer(*field, box->args))
                    return luaL_error(L, "On%s args: the runtime passed no '%s' flag",
                                      box->event->name, name);
                return 0;
            }

            size_t      size = 0;
            const char* text = luaL_checklstring(L, 3, &size);
            uint32_t    capacity = 0;
            if (!WriteBuffer(*field, box->args, std::string_view(text, size), capacity))
                return luaL_error(L, "On%s args: '%s' takes at most %d bytes; %d were given",
                                  box->event->name, name, static_cast<int>(capacity ? capacity - 1 : 0),
                                  static_cast<int>(size));
            return 0;
        }

        int ArgsToString(lua_State* L)
        {
            ArgsBox* box = CheckBox(L, 1);
            lua_pushfstring(L, "On%s args%s", box->event->name, box->args ? "" : " (expired)");
            return 1;
        }
    } // namespace

    void OpenArgs(lua_State* L)
    {
        luaL_newmetatable(L, kArgsMeta);
        lua_pushcfunction(L, &ArgsIndex);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, &ArgsNewIndex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, &ArgsToString);
        lua_setfield(L, -2, "__tostring");
        lua_pushliteral(L, "wxl.args");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 1);
    }

    ArgsBox* PushArgs(lua_State* L, const reflect::Event& event, const void* args, bool writable)
    {
        ArgsBox* box  = static_cast<ArgsBox*>(lua_newuserdata(L, sizeof(ArgsBox)));
        box->event    = &event;
        box->args     = args;
        box->writable = writable;
        luaL_getmetatable(L, kArgsMeta);
        lua_setmetatable(L, -2);
        return box;
    }

    void PushField(lua_State* L, const reflect::Field& field, const void* args)
    {
        using reflect::Kind;
        switch (field.kind)
        {
        case Kind::u32: lua_pushnumber(L, ReadAt<uint32_t>(args, field.offset)); return;
        case Kind::i32: lua_pushnumber(L, ReadAt<int32_t>(args, field.offset)); return;
        case Kind::u64:
            lua_pushnumber(L, static_cast<lua_Number>(ReadAt<uint64_t>(args, field.offset)));
            return;
        case Kind::i64:
            lua_pushnumber(L, static_cast<lua_Number>(ReadAt<int64_t>(args, field.offset)));
            return;
        case Kind::f32: lua_pushnumber(L, ReadAt<float>(args, field.offset)); return;
        case Kind::boolean: lua_pushboolean(L, ReadAt<wxl_bool>(args, field.offset) != 0); return;
        case Kind::handle: PushObject(L, ReadAt<wxl_handle>(args, field.offset)); return;
        case Kind::guid: PushGuid(L, ReadAt<wxl_guid>(args, field.offset)); return;
        case Kind::vec3:
        {
            const wxl_vec3 v = ReadAt<wxl_vec3>(args, field.offset);
            lua_createtable(L, 0, 3);
            lua_pushnumber(L, v.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, v.y);
            lua_setfield(L, -2, "y");
            lua_pushnumber(L, v.z);
            lua_setfield(L, -2, "z");
            return;
        }
        case Kind::str:
        {
            const wxl_str s = ReadAt<wxl_str>(args, field.offset);
            lua_pushlstring(L, s.data ? s.data : "", s.data ? s.size : 0);
            return;
        }
        case Kind::ptr:
        {
            void* p = ReadAt<void*>(args, field.offset);
            if (p) lua_pushlightuserdata(L, p);
            else lua_pushnil(L);
            return;
        }
        case Kind::bytes:
        {
            const void* data = ReadAt<const void*>(args, field.offset);
            const uint32_t size = data ? ExtentOf(field, args) : 0;
            lua_pushlstring(L, static_cast<const char*>(data ? data : ""), size);
            return;
        }
        case Kind::answer:
        {
            const wxl_bool* flag = ReadAt<const wxl_bool*>(args, field.offset);
            if (flag) lua_pushboolean(L, *flag != 0);
            else lua_pushnil(L);
            return;
        }
        case Kind::buffer:
        {
            const char*    buffer   = ReadAt<const char*>(args, field.offset);
            const uint32_t capacity = buffer ? ExtentOf(field, args) : 0;
            if (!buffer)
            {
                lua_pushnil(L);
                return;
            }
            uint32_t length = 0;
            while (length < capacity && buffer[length]) ++length;
            lua_pushlstring(L, buffer, length);
            return;
        }
        }
        lua_pushnil(L);
    }

    const reflect::Field* AnswerOf(const reflect::Event& event)
    {
        return FirstOfKind(event, reflect::Kind::answer);
    }

    const reflect::Field* BufferOf(const reflect::Event& event)
    {
        return FirstOfKind(event, reflect::Kind::buffer);
    }

    bool WriteAnswer(const reflect::Field& field, const void* args)
    {
        wxl_bool* flag = ReadAt<wxl_bool*>(args, field.offset);
        if (!flag) return false;
        *flag = WXL_TRUE;
        return true;
    }

    bool WriteBuffer(const reflect::Field& field, const void* args, std::string_view text,
                     uint32_t& capacity)
    {
        char* buffer = ReadAt<char*>(args, field.offset);
        capacity     = buffer ? ExtentOf(field, args) : 0;
        if (!buffer || capacity == 0 || text.size() + 1 > capacity) return false;
        std::memcpy(buffer, text.data(), text.size());
        buffer[text.size()] = '\0';
        return true;
    }

    int ToTable(lua_State* L)
    {
        ArgsBox* box = LiveBox(L, 1);
        lua_createtable(L, 0, static_cast<int>(box->event->field_count));
        for (uint32_t i = 0; i < box->event->field_count; ++i)
        {
            const reflect::Field& field = box->event->fields[i];
            PushField(L, field, box->args);
            lua_setfield(L, -2, field.name);
        }
        return 1;
    }

    // --- The copy for the queue ------------------------------------------------------------

    namespace
    {
        size_t RoundUp(size_t size) { return (size + 7) & ~size_t(7); }
    }

    ArgsCopy::ArgsCopy(const reflect::Event& event, const void* args)
    {
        using reflect::Kind;

        size_t total = RoundUp(event.args_size);
        for (uint32_t i = 0; i < event.field_count; ++i)
        {
            const reflect::Field& field = event.fields[i];
            switch (field.kind)
            {
            case Kind::str: total += RoundUp(size_t(ReadAt<wxl_str>(args, field.offset).size) + 1); break;
            case Kind::bytes: total += RoundUp(ExtentOf(field, args)); break;
            case Kind::answer: total += RoundUp(sizeof(wxl_bool)); break;
            case Kind::buffer: total += RoundUp(ExtentOf(field, args)); break;
            default: break;
            }
        }

        bytes_.assign(total, 0);
        std::memcpy(bytes_.data(), args, event.args_size);
        void*  copy   = bytes_.data();
        size_t cursor = RoundUp(event.args_size);

        for (uint32_t i = 0; i < event.field_count; ++i)
        {
            const reflect::Field& field = event.fields[i];
            uint8_t* const        slot  = bytes_.data() + cursor;
            switch (field.kind)
            {
            case Kind::str:
            {
                const wxl_str s = ReadAt<wxl_str>(args, field.offset);
                if (s.data && s.size) std::memcpy(slot, s.data, s.size);
                WriteAt(copy, field.offset, wxl_str{reinterpret_cast<const char*>(slot), s.data ? s.size : 0});
                cursor += RoundUp(size_t(s.data ? s.size : 0) + 1);
                break;
            }
            case Kind::bytes:
            {
                const void*    data = ReadAt<const void*>(args, field.offset);
                const uint32_t size = ExtentOf(field, args);
                if (data && size) std::memcpy(slot, data, size);
                WriteAt(copy, field.offset, static_cast<const void*>(data ? slot : nullptr));
                cursor += RoundUp(size);
                break;
            }
            case Kind::answer:
            {
                const wxl_bool* flag = ReadAt<const wxl_bool*>(args, field.offset);
                wxl_bool        own  = flag ? *flag : WXL_FALSE;
                std::memcpy(slot, &own, sizeof own);
                WriteAt(copy, field.offset, reinterpret_cast<wxl_bool*>(slot));
                cursor += RoundUp(sizeof(wxl_bool));
                break;
            }
            case Kind::buffer:
            {
                const char*    buffer   = ReadAt<const char*>(args, field.offset);
                const uint32_t capacity = ExtentOf(field, args);
                if (buffer && capacity) std::memcpy(slot, buffer, capacity);
                WriteAt(copy, field.offset, reinterpret_cast<char*>(buffer ? slot : nullptr));
                cursor += RoundUp(capacity);
                break;
            }
            default: break;
            }
        }
    }
} // namespace wxl::lua
