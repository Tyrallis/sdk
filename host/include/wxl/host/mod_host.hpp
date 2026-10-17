// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The runtime's handle on its mods: discovery, admission, loading, stages, events, services and
// the wxl_host table handed to every mod. Everything two runtimes would otherwise write twice.

#ifndef WXL_HOST_MOD_HOST_HPP
#define WXL_HOST_MOD_HOST_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "wxl/host.h"
#include "wxl/host/admission.hpp"
#include "wxl/host/discovery.hpp"
#include "wxl/host/event_bus.hpp"
#include "wxl/host/guard.hpp"
#include "wxl/host/loader.hpp"
#include "wxl/host/log.hpp"
#include "wxl/host/manifest.hpp"
#include "wxl/host/registry.hpp"
#include "wxl/mod.h"

namespace wxl::host
{
    /// What a runtime says about itself and what it offers. Read once, at construction.
    struct HostConfig
    {
        std::filesystem::path mods_directory;

        std::string runtime_id;          ///< "core", "client", "engine"
        std::string runtime_name;        ///< for logs
        uint32_t    runtime_version = 0; ///< opaque to a mod
        std::string target = CurrentTarget(); ///< which "binaries" entry to load

        LogSink*      log_sink    = nullptr; ///< not owned; null drops every line
        wxl_log_level log_minimum = WXL_LOG_TRACE;

        /// The events this runtime raises. A subscribe to any other gets WXL_SUBSCRIPTION_NONE.
        std::vector<wxl_event> events;

        /// Published before any mod loads, so a manifest's `requires` can be answered.
        std::vector<PublishedService> services;

        /// Null means the OS loader. A test injects its own.
        LibraryLoader* loader = nullptr;
    };

    /// A mod whose wxl_mod_load returned WXL_OK.
    struct LoadedMod
    {
        std::string           id;
        std::string           name;
        uint32_t              version = 0;
        std::string           directory_name;
        std::filesystem::path directory;
        std::filesystem::path binary;
        Manifest              manifest;
        bool                  has_stage  = false; ///< exports wxl_mod_stage
        bool                  has_unload = false; ///< exports wxl_mod_unload
        bool                  disabled   = false; ///< faulted; no more of its code is called
        std::string           fault;              ///< what it was doing when it faulted
    };

    /// A folder that did not become a loaded mod, and why.
    struct RefusedMod
    {
        std::string           id; ///< empty when the manifest could not be read
        std::string           directory_name;
        std::filesystem::path directory;
        Verdict               verdict = Verdict::Refuse; ///< Skip or Refuse
        std::string           reason;
    };

    /**
     * @brief The mod host.
     *
     * One per process: the wxl_host table is plain C function pointers with nowhere to carry an
     * instance, so the live host is the one the table routes to. Construct it with the runtime's
     * configuration, call @ref LoadAll once the mods directory is known, @ref Stage as the boot
     * advances, @ref Raise at the runtime's own seams, and @ref Shutdown on the way out.
     *
     * Every call into a mod -- its four entry points and every event handler -- runs under the
     * crash guard. A mod that faults is logged with what it was doing and disabled: its
     * subscriptions are dropped, its later stages and its unload are skipped, its binary stays
     * mapped, and the host goes on. The services it published stay published, since another mod
     * may hold the pointer.
     *
     * Every method runs on the thread that calls it; the host does not lock.
     */
    class ModHost
    {
    public:
        /// A mod as the host refers to it from inside a call: its position in @ref Loaded plus
        /// one, so that 0 means nobody.
        using ModToken = uint32_t;

        /// Told when a mod is disabled, after its subscriptions are dropped.
        using DisableFn = void (*)(void* user, ModToken token, const char* id);

        explicit ModHost(HostConfig config);
        ~ModHost();

        ModHost(const ModHost&)            = delete;
        ModHost& operator=(const ModHost&) = delete;

        /**
         * @brief Finds, admits and loads every mod in the mods directory, in load order.
         *
         * Each outcome is logged and kept for @ref Loaded and @ref Refused. A second call does
         * nothing: mods load once.
         */
        void LoadAll();

        /**
         * @brief Reports a boot stage to every loaded mod that exports wxl_mod_stage.
         *
         * Load order, except WXL_STAGE_SHUTDOWN, which runs in reverse. A disabled mod is skipped.
         */
        void Stage(wxl_stage stage);

        /// Calls wxl_mod_unload in reverse load order and frees the binaries. A disabled mod's
        /// unload is not called and its binary is left mapped.
        void UnloadAll();

        /// WXL_STAGE_SHUTDOWN, then @ref UnloadAll.
        void Shutdown();

        /// Dispatches an event. What the runtime calls at its own seams.
        void Raise(wxl_event event, const void* args);

        /**
         * @brief Disables a mod after it faulted: logs what it was doing, drops its
         *        subscriptions, skips its remaining entry points, tells the disable handler.
         *
         * @param what  what the mod was doing, for the log: "in wxl_mod_stage(early)".
         */
        void Disable(ModToken token, std::string_view what);

        /// Called after a mod is disabled. A runtime uses it to withdraw what the mod registered
        /// with its services.
        void SetDisableHandler(DisableFn handler, void* user)
        {
            disableFn_   = handler;
            disableUser_ = user;
        }

        /**
         * @brief The mod whose code is running, or 0 for nobody.
         *
         * Set around every entry point and every handler the host calls. A runtime that calls
         * into a mod by another route (a script function, say) brackets the call with
         * @ref SwapCurrentMod so that what the mod does inside is attributed to it.
         */
        ModToken CurrentMod() const { return currentMod_; }

        /// Sets the current mod and returns the previous one, to put back after the call.
        ModToken SwapCurrentMod(ModToken token)
        {
            const ModToken previous = currentMod_;
            currentMod_             = token;
            return previous;
        }

        /// The loaded mod a token names, or null for 0 or a token past the end.
        const LoadedMod* ModOf(ModToken token) const;

        /// The table every mod receives. Valid for the host's lifetime.
        const wxl_host* Table() const { return &table_; }

        const std::vector<LoadedMod>&  Loaded() const { return loaded_; }
        const std::vector<RefusedMod>& Refused() const { return refused_; }

        const HostConfig&      Config() const { return config_; }
        const Logger&          Log() const { return logger_; }
        ServiceRegistry&       Services() { return services_; }
        const ServiceRegistry& Services() const { return services_; }
        EventBus&              Events() { return events_; }
        const EventBus&        Events() const { return events_; }

    private:
        static ModHost* current_;

        static void WXL_CALL                    ThunkLog(wxl_log_level level, const char* tag,
                                                         wxl_str message);
        static const wxl_runtime_info* WXL_CALL ThunkRuntime(void);
        static wxl_bool WXL_CALL                ThunkHasService(const char* name, uint32_t version);
        static const void* WXL_CALL             ThunkGetService(const char* name, uint32_t version);
        static wxl_result WXL_CALL ThunkPublishService(const char* name, uint32_t version,
                                                       const void* iface);
        static wxl_subscription WXL_CALL ThunkSubscribe(wxl_event event, wxl_event_fn handler,
                                                        void* user, int32_t priority);
        static void WXL_CALL             ThunkUnsubscribe(wxl_subscription subscription);
        static void WXL_CALL             ThunkEmit(wxl_event event, const void* args);

        static void OnHandlerFault(void* user, uint32_t owner, wxl_event event, const Fault& fault);

        void Refuse(const std::string& id, const std::string& directoryName,
                    const std::filesystem::path& directory, Verdict verdict,
                    const std::string& reason);
        void TryLoad(const ModCandidate& candidate);

        /// Calls one entry point of a mod under the guard, as that mod. False when it faulted;
        /// @p description then says how.
        bool CallEntry(ModToken token, GuardedFn fn, void* context, char* description,
                       size_t capacity);

        HostConfig       config_;
        Logger           logger_;
        ServiceRegistry  services_;
        EventBus         events_;
        wxl_runtime_info info_{};
        wxl_host         table_{};

        std::vector<LoadedMod>  loaded_;
        std::vector<ModBinary>  binaries_; ///< index-aligned with loaded_
        std::vector<RefusedMod> refused_;
        bool                    loadedAll_  = false;
        ModToken                currentMod_ = 0;
        DisableFn               disableFn_   = nullptr;
        void*                   disableUser_ = nullptr;
    };
} // namespace wxl::host

#endif // WXL_HOST_MOD_HOST_HPP
