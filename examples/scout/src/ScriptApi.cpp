// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "ScriptApi.h"

#include <cstdio>

#include "wxl/objects.hpp"

namespace
{
    // What the registered functions below read. They are plain functions, because that is what
    // the client's scripting calls in the end, and a plain function has no `this`: the state it
    // works on has to be reachable from file scope. One mod, one instance, so a pointer each.
    scout::Journal*  g_journal = nullptr;
    scout::NetWatch* g_net     = nullptr;

    /// ScoutCount() -> how many lines the journal is holding.
    void ScoutCount(wxl::ScriptCall& call)
    {
        call.PushNumber(g_journal ? static_cast<double>(g_journal->Size()) : 0.0);
    }

    /// ScoutLast() -> what, who, level, seconds. Returns nil when nothing has happened yet.
    void ScoutLast(wxl::ScriptCall& call)
    {
        const scout::Entry* last = g_journal ? g_journal->Last() : nullptr;
        if (!last)
        {
            call.PushNil();
            return;
        }
        call.PushString(scout::Journal::Describe(last->what));
        call.PushString(last->who);
        call.PushNumber(last->level);
        call.PushNumber(last->at);
    }

    /// ScoutNearby() -> how many units the client is tracking right now.
    void ScoutNearby(wxl::ScriptCall& call)
    {
        // A snapshot of handles, taken now. Asking the client directly rather than keeping a
        // count of our own means the answer cannot drift out of step with it.
        const wxl::ObjectService objects;
        call.PushNumber(static_cast<double>(objects.InView<wxl::Unit>().size()));
    }

    /// ScoutTraffic(opcode [, "sent"]) -> how many messages of that opcode crossed.
    void ScoutTraffic(wxl::ScriptCall& call)
    {
        const std::optional<double> opcode = call.Number(0);
        if (!opcode || *opcode < 0.0)
        {
            // The call still returns normally here; the script side fails once it unwinds,
            // with this message, which is how a Lua error is raised from C++ safely.
            call.Fail("ScoutTraffic(opcode [, \"sent\"]): opcode must be a number");
            return;
        }

        const std::optional<std::string_view> side     = call.String(1);
        const bool                            incoming = !side || *side != "sent";
        const uint32_t                        counted =
            g_net ? g_net->Count(static_cast<uint32_t>(*opcode), incoming) : 0u;
        call.PushNumber(counted);
    }

    /// ScoutMute(opcode, on) -> whether it was already muted. Incoming messages only.
    void ScoutMute(wxl::ScriptCall& call)
    {
        const std::optional<double> opcode = call.Number(0);
        if (!opcode || *opcode < 0.0)
        {
            call.Fail("ScoutMute(opcode, on): opcode must be a number");
            return;
        }
        if (!g_net)
        {
            call.PushBool(false);
            return;
        }
        const bool on = call.Bool(1).value_or(true);
        call.PushBool(g_net->Mute(static_cast<uint32_t>(*opcode), on));
    }
} // namespace

namespace scout
{
    ScriptApi::ScriptApi(Journal& journal, NetWatch& net)
    {
        g_journal = &journal;
        g_net     = &net;
    }

    void ScriptApi::OnScriptVmReady(const wxl_script_vm_args& args)
    {
        const wxl::ScriptService script;
        if (!script)
        {
            // No scripting on this runtime. Everything else Scout does still works, which is
            // why wxl.script is "optional" in mod.json and not "requires".
            return;
        }

        // Both contexts are served. The login screen is a script context too, and a mod that
        // only registers on the game one is simply absent from half the session.
        const bool glue = args.context == WXL_SCRIPT_CONTEXT_GLUE;

        script.RegisterEvent(kUpdatedEvent);
        script.RegisterFunction<&ScoutCount>("ScoutCount");
        script.RegisterFunction<&ScoutLast>("ScoutLast");
        script.RegisterFunction<&ScoutNearby>("ScoutNearby");
        script.RegisterFunction<&ScoutTraffic>("ScoutTraffic");
        script.RegisterFunction<&ScoutMute>("ScoutMute");
        registered_ = true;

        wxl::Log(WXL_LOG_INFO, kTag,
                 glue ? "script API is up on the login screen" : "script API is up in the game");
    }

    void ScriptApi::OnScriptVmClosing(const wxl_script_vm_args&)
    {
        // The context is about to be torn down and everything on it goes with it. There is
        // nothing to undo -- only a note to self that the functions are gone until the next one.
        registered_ = false;
    }

    void ScriptApi::Announce() const
    {
        if (!registered_) return;

        const Entry* last = g_journal ? g_journal->Last() : nullptr;
        if (!last) return;

        const wxl::ScriptService script;
        script.FireEvent(kUpdatedEvent, {wxl::Value::String(Journal::Describe(last->what)),
                                         wxl::Value::String(last->who),
                                         wxl::Value::Number(last->level)});
    }
} // namespace scout
