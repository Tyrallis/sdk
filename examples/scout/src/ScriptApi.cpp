// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "ScriptApi.h"

#include <cmath>
#include <optional>
#include <string>

#include "wxl/objects.hpp"

namespace
{
    /// The argument at @p index as an opcode: a whole number from 0 to 0xFFFF.
    std::optional<uint32_t> OpcodeAt(const wxl::ScriptCall& call, uint32_t index)
    {
        const std::optional<double> number = call.Number(index);
        if (!number || *number < 0.0 || *number > 0xFFFF || std::floor(*number) != *number)
            return std::nullopt;
        return static_cast<uint32_t>(*number);
    }
} // namespace

namespace scout
{
    void ScriptApi::OnScriptVmReady(uint32_t context, void*)
    {
        const wxl::ScriptService script;
        if (!script) return; // wxl.script is optional: without it, Scout keeps its journal to itself

        struct Registration
        {
            const char* name;
            Function*   function;
        };
        const Registration functions[] = {
            {"ScoutCount", &scoutCount_},
            {"ScoutEntry", &scoutEntry_},
            {"ScoutNearby", &scoutNearby_},
            {"ScoutTraffic", &scoutTraffic_},
            {"ScoutDrop", &scoutDrop_},
        };

        for (const Registration& registration : functions)
        {
            const wxl_result result = script.RegisterFunction(registration.name, registration.function);
            if (result != WXL_OK)
                wxl::Log(WXL_LOG_WARN, kTag,
                         std::string("could not register ") + registration.name + ": result " +
                             std::to_string(result));
        }
        if (script.RegisterEvent(kUpdatedEvent) != WXL_OK)
            wxl::Log(WXL_LOG_WARN, kTag, std::string("could not declare ") + kUpdatedEvent);

        open_ = true;
        wxl::Log(WXL_LOG_INFO, kTag,
                 context == WXL_SCRIPT_CONTEXT_GLUE ? "functions registered on the login screens"
                                                    : "functions registered in the game");
    }

    void ScriptApi::OnScriptVmClosing(uint32_t, void*)
    {
        open_ = false;
    }

    void ScriptApi::Announce(const Entry& entry) const
    {
        if (!open_) return;
        wxl::ScriptService().FireEvent(kUpdatedEvent, {wxl::Value::String(Describe(entry.what)),
                                                       wxl::Value::String(entry.who),
                                                       wxl::Value::Number(entry.level)});
    }

    // ScoutCount() -> how many lines the journal holds.
    void ScriptApi::Count(wxl::ScriptCall& call) const
    {
        call.PushNumber(static_cast<double>(journal_.Size()));
    }

    // ScoutEntry([index]) -> what, who, level, whole seconds ago; index 1, the default, is the
    // newest. Nothing past the oldest line.
    void ScriptApi::EntryAt(wxl::ScriptCall& call) const
    {
        double index = 1.0;
        if (call.ArgType(0) != WXL_VALUE_NIL)
        {
            const std::optional<double> given = call.Number(0);
            if (!given || *given < 1.0 || std::floor(*given) != *given)
            {
                call.Fail("ScoutEntry([index]): index is a whole number from 1, the newest line");
                return;
            }
            index = *given;
        }

        const Entry* entry = journal_.FromNewest(static_cast<std::size_t>(index) - 1);
        if (!entry) return;

        call.PushString(Describe(entry->what));
        call.PushString(entry->who);
        call.PushNumber(entry->level);
        call.PushNumber(std::floor(clock_.Now() - entry->at));
    }

    // ScoutNearby() -> how many units the client holds right now, the player included.
    void ScriptApi::Nearby(wxl::ScriptCall& call) const
    {
        call.PushNumber(static_cast<double>(wxl::ObjectService().InView<wxl::Unit>().size()));
    }

    // ScoutTraffic(opcode [, "sent"]) -> messages, bytes of that opcode received, or sent.
    void ScriptApi::TrafficOf(wxl::ScriptCall& call) const
    {
        const std::optional<uint32_t> opcode = OpcodeAt(call, 0);
        if (!opcode)
        {
            call.Fail("ScoutTraffic(opcode [, \"sent\"]): opcode is a whole number from 0 to 0xFFFF");
            return;
        }
        const std::optional<std::string_view> way = call.String(1);
        const Direction direction = way && *way == "sent" ? Direction::Sent : Direction::Received;

        const Tally tally = traffic_.Count(*opcode, direction);
        call.PushNumber(tally.messages);
        call.PushNumber(static_cast<double>(tally.bytes));
    }

    // ScoutDrop(opcode, drop) -> whether that opcode was dropped before. Incoming messages only.
    void ScriptApi::Drop(wxl::ScriptCall& call)
    {
        const std::optional<uint32_t> opcode = OpcodeAt(call, 0);
        const std::optional<bool>     drop   = call.Bool(1);
        if (!opcode || !drop)
        {
            call.Fail("ScoutDrop(opcode, drop): opcode is a whole number from 0 to 0xFFFF, drop "
                      "is true or false");
            return;
        }
        call.PushBool(traffic_.Drop(*opcode, *drop));
    }
} // namespace scout
