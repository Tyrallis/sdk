// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#ifndef SCOUT_SCRIPT_API_H
#define SCOUT_SCRIPT_API_H

#include <functional>

#include "wxl/sdk.hpp"
#include "wxl/services/script.hpp"

#include "Journal.h"
#include "Traffic.h"

namespace scout
{
    /// The event an addon registers for to hear of each new line: what, who, level.
    inline constexpr const char* kUpdatedEvent = "SCOUT_UPDATED";

    /**
     * @brief Scout's functions and event on the client's own Lua.
     *
     * A script context loses everything registered on it when it closes, and the client opens
     * a new one for the login screens and another for the game, again and again in a session.
     * So the functions and the event are registered on every ScriptVmReady, whichever context
     * it announces.
     */
    class ScriptApi final : public wxl::LuaScript
    {
    public:
        ScriptApi(const Clock& clock, const Journal& journal, Traffic& traffic)
            : clock_(clock), journal_(journal), traffic_(traffic)
        {
        }

        // The functions below capture `this`: a copy would call into the original.
        ScriptApi(const ScriptApi&)            = delete;
        ScriptApi& operator=(const ScriptApi&) = delete;

        void OnScriptVmReady(uint32_t context, void* native_state) override;
        void OnScriptVmClosing(uint32_t context, void* native_state) override;

        /// Fires kUpdatedEvent with @p entry, when a context is open to hear it.
        void Announce(const Entry& entry) const;

    private:
        // What each function answers, from the arguments it was called with.
        void Count(wxl::ScriptCall& call) const;
        void EntryAt(wxl::ScriptCall& call) const;
        void Nearby(wxl::ScriptCall& call) const;
        void TrafficOf(wxl::ScriptCall& call) const;
        void Drop(wxl::ScriptCall& call);

        /// A function as the script service calls it: (*function)(call).
        using Function = std::function<void(wxl::ScriptCall&)>;

        Function scoutCount_   = [this](wxl::ScriptCall& call) { Count(call); };
        Function scoutEntry_   = [this](wxl::ScriptCall& call) { EntryAt(call); };
        Function scoutNearby_  = [this](wxl::ScriptCall& call) { Nearby(call); };
        Function scoutTraffic_ = [this](wxl::ScriptCall& call) { TrafficOf(call); };
        Function scoutDrop_    = [this](wxl::ScriptCall& call) { Drop(call); };

        const Clock&   clock_;
        const Journal& journal_;
        Traffic&       traffic_;
        bool           open_ = false; ///< a context is open and holds what was registered
    };
} // namespace scout

#endif // SCOUT_SCRIPT_API_H
