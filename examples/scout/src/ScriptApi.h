// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#ifndef SCOUT_SCRIPT_API_H
#define SCOUT_SCRIPT_API_H

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

        // The script side calls Scout's functions on this object: a copy would not be the one
        // registered.
        ScriptApi(const ScriptApi&)            = delete;
        ScriptApi& operator=(const ScriptApi&) = delete;

        void OnScriptVmReady(wxl_script_context context, void* native_state) override;
        void OnScriptVmClosing(wxl_script_context context, void* native_state) override;

        /// Fires kUpdatedEvent with @p entry, when a context is open to hear it.
        void Announce(const Entry& entry) const;

    private:
        // What each function answers, from the arguments it was called with.
        void Count(wxl::ScriptCall& call) const;
        void EntryAt(wxl::ScriptCall& call) const;
        void Nearby(wxl::ScriptCall& call) const;
        void TrafficOf(wxl::ScriptCall& call) const;
        void Drop(wxl::ScriptCall& call);

        /// Makes the member function @p Method a global Lua function named @p name, called on
        /// this object, and logs it when the script service refuses.
        template <auto Method>
        void Register(const wxl::ScriptService& script, const char* name)
        {
            const wxl_result result = script.RegisterFunction<Method>(name, this);
            if (result != WXL_OK)
                wxl::Log(WXL_LOG_WARN, kTag, "could not register {}: {}", name, wxl::ResultName(result));
        }

        const Clock&   clock_;
        const Journal& journal_;
        Traffic&       traffic_;
        bool           open_ = false; ///< a context is open and holds what was registered
    };
} // namespace scout

#endif // SCOUT_SCRIPT_API_H
