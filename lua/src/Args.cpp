// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "Args.hpp"

#include <cstring>
#include <string>

#include "Objects.hpp"

namespace wxl::lua
{
    namespace
    {
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

        bool Writable(const reflect::Field& field)
        {
            return field.kind == reflect::Kind::answer || field.kind == reflect::Kind::buffer;
        }
    } // namespace

    int PushParameters(lua_State* L, const reflect::Event& event, const void* args)
    {
        int pushed = 0;
        for (uint32_t i = 0; i < event.field_count; ++i)
        {
            if (reflect::IsExtent(event, event.fields[i])) continue;
            PushField(L, event.fields[i], args);
            ++pushed;
        }
        return pushed;
    }

    void PushField(lua_State* L, const reflect::Field& field, const void* args)
    {
        using reflect::Kind;
        // An enumeration's value by its name; a number the headers do not name, as the number.
        if (field.enumeration)
        {
            const uint32_t value = ReadAt<uint32_t>(args, field.offset);
            if (const char* name = reflect::NameOf(*field.enumeration, value))
                lua_pushstring(L, name);
            else
                lua_pushnumber(L, value);
            return;
        }
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

    std::vector<const reflect::Field*> WritableOf(const reflect::Event& event)
    {
        std::vector<const reflect::Field*> fields;
        for (uint32_t i = 0; i < event.field_count; ++i)
            if (Writable(event.fields[i])) fields.push_back(&event.fields[i]);
        return fields;
    }

    bool Answerable(const reflect::Event& event)
    {
        for (uint32_t i = 0; i < event.field_count; ++i)
            if (Writable(event.fields[i])) return true;
        return false;
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
