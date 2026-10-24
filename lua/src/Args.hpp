// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// An event's args as Lua sees them: one value per parameter of the handler, read from the C
// struct through the field table of reflect.hpp, and the members an Override answers through.
// Nothing here names an event or a member: a row added to fields.def reaches Lua without a
// change below.

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
     * @brief Pushes a handler's parameters: every member reflect::IsExtent leaves, in order.
     *
     * Each is a copy -- a number, a string, a table, an object over a handle -- so nothing a
     * handler keeps reads the struct after the call. Returns how many were pushed.
     */
    int PushParameters(lua_State* L, const reflect::Event& event, const void* args);

    /// Pushes one member of an args struct as a Lua value.
    void PushField(lua_State* L, const reflect::Field& field, const void* args);

    /// The members an Override answers through, answers and buffers, in member order.
    std::vector<const reflect::Field*> WritableOf(const reflect::Event& event);

    /// Whether an event has a member an Override answers through.
    bool Answerable(const reflect::Event& event);

    /// Writes WXL_TRUE through an answer member. False when the runtime passed no flag.
    bool WriteAnswer(const reflect::Field& field, const void* args);

    /// Writes a NUL-terminated string into a buffer member. False when the runtime passed no
    /// buffer or the text does not fit; @p capacity receives what the buffer takes.
    bool WriteBuffer(const reflect::Field& field, const void* args, std::string_view text,
                     uint32_t& capacity);

    /**
     * @brief A deep copy of an event's args, for an event raised off the main thread.
     *
     * The struct is copied, then every pointer member is pointed into storage owned here: a
     * str's text, a bytes member's extent, an answer's flag, a buffer's capacity. The flag and
     * the buffer hold what they held when the copy was made, so a handler reads them as it would
     * on the main thread; no answer is written to them, which is what "not taken" means.
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
