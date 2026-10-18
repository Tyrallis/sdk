// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 WarcraftXL
//
// An event's args as Lua sees them: a userdata over the C struct, read member by member through
// the field table of reflect.hpp. Nothing here names an event or a member: a row added to
// fields.def reaches Lua without a change below.

#ifndef WXL_LUA_ARGS_HPP
#define WXL_LUA_ARGS_HPP

#include <cstdint>
#include <string_view>
#include <vector>

#include "Lua.hpp"
#include "wxl/reflect.hpp"

namespace wxl::lua
{
    /**
     * @brief What the args userdata holds.
     *
     * @ref args points at the struct the runtime handed over and is valid for the handler's
     * call only; the dispatcher nulls it when the handler returns, so a userdata a handler kept
     * raises a Lua error instead of reading freed memory.
     */
    struct ArgsBox
    {
        const reflect::Event* event;
        const void*           args;
        bool                  writable; ///< an Override handler may write answer and buffer members
    };

    /// Registers the args metatable in a state.
    void OpenArgs(lua_State* L);

    /// Pushes a new args userdata and returns it, so the caller can null @ref ArgsBox::args
    /// after the call.
    ArgsBox* PushArgs(lua_State* L, const reflect::Event& event, const void* args, bool writable);

    /// Pushes one member of an args struct as a Lua value.
    void PushField(lua_State* L, const reflect::Field& field, const void* args);

    /// The first answer member of an event's args, or null; the same for the first buffer.
    const reflect::Field* AnswerOf(const reflect::Event& event);
    const reflect::Field* BufferOf(const reflect::Event& event);

    /// Writes WXL_TRUE through an answer member. False when the runtime passed no flag.
    bool WriteAnswer(const reflect::Field& field, const void* args);

    /// Writes a NUL-terminated string into a buffer member. False when the runtime passed no
    /// buffer or the text does not fit; @p capacity receives what the buffer takes.
    bool WriteBuffer(const reflect::Field& field, const void* args, std::string_view text,
                     uint32_t& capacity);

    /// wxl.totable(args): every member of the args as a plain table.
    int ToTable(lua_State* L);

    /**
     * @brief A deep copy of an event's args, for an event raised off the main thread.
     *
     * The struct is copied, then every pointer member is pointed into storage owned here: a
     * str's text, a bytes member's extent, an answer's flag, a buffer's capacity. Handlers may
     * write the flag and the buffer; nobody reads them back, which is what "not taken" means.
     */
    class ArgsCopy
    {
    public:
        ArgsCopy(const reflect::Event& event, const void* args);

        ArgsCopy(ArgsCopy&&) noexcept            = default;
        ArgsCopy& operator=(ArgsCopy&&) noexcept = default;
        ArgsCopy(const ArgsCopy&)                = delete;
        ArgsCopy& operator=(const ArgsCopy&)     = delete;

        const void* Args() const { return bytes_.data(); }

    private:
        std::vector<uint8_t> bytes_;
    };
} // namespace wxl::lua

#endif // WXL_LUA_ARGS_HPP
