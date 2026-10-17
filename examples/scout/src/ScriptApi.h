// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// Scout's side of the client's own scripting. Everything here is about one rule: a script
// context is created and destroyed several times in a session -- once for the login screen,
// again for the game interface, again every time you go back out -- and nothing registered on
// one survives into the next. So this class registers from scratch on every ScriptVmReady.

#ifndef SCOUT_SCRIPT_API_H
#define SCOUT_SCRIPT_API_H

#include "wxl/sdk.hpp"
#include "wxl/services/script.hpp"

#include "Journal.h"
#include "Watchers.h"

namespace scout
{
    /// The event an addon listens to with `frame:RegisterEvent("SCOUT_UPDATED")`.
    inline constexpr const char* kUpdatedEvent = "SCOUT_UPDATED";

    class ScriptApi final : public wxl::LuaScript
    {
    public:
        ScriptApi(Journal& journal, NetWatch& net);

        void OnScriptVmReady(const wxl_script_vm_args& args) override;
        void OnScriptVmClosing(const wxl_script_vm_args& args) override;

        /// Tells the interface the journal grew, if there is an interface to tell.
        void Announce() const;

        /// Whether a context is live and our functions are on it.
        bool Registered() const { return registered_; }

    private:
        bool registered_ = false;
    };
} // namespace scout

#endif // SCOUT_SCRIPT_API_H
