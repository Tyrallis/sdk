// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/mod_host.hpp"

#include <cstdio>
#include <string_view>
#include <utility>

namespace wxl::host
{
    ModHost* ModHost::current_ = nullptr;

    namespace
    {
        std::string_view View(wxl_str str)
        {
            return str.data ? std::string_view(str.data, str.size) : std::string_view();
        }

        const char* StageName(wxl_stage stage)
        {
            switch (stage)
            {
            case WXL_STAGE_EARLY: return "early";
            case WXL_STAGE_STORAGE: return "storage";
            case WXL_STAGE_GRAPHICS: return "graphics";
            case WXL_STAGE_SCRIPT: return "script";
            case WXL_STAGE_READY: return "ready";
            case WXL_STAGE_SHUTDOWN: return "shutdown";
            }
            return "?";
        }

        // The entry points as the guard calls them: one plain function per entry, reading its
        // arguments and writing its result through a context block.

        struct QueryCall
        {
            wxl_mod_query_fn    fn;
            const wxl_mod_info* result;
        };

        void InvokeQuery(void* context)
        {
            QueryCall* call = static_cast<QueryCall*>(context);
            call->result    = call->fn();
        }

        struct LoadCall
        {
            wxl_mod_load_fn fn;
            const wxl_host* table;
            wxl_result      result;
        };

        void InvokeLoad(void* context)
        {
            LoadCall* call = static_cast<LoadCall*>(context);
            call->result   = call->fn(call->table);
        }

        struct StageCall
        {
            wxl_mod_stage_fn fn;
            wxl_stage        stage;
        };

        void InvokeStage(void* context)
        {
            StageCall* call = static_cast<StageCall*>(context);
            call->fn(call->stage);
        }

        void InvokeUnload(void* context)
        {
            static_cast<wxl_mod_unload_fn>(context)();
        }
    } // namespace

    ModHost::ModHost(HostConfig config)
        : config_(std::move(config)), logger_(config_.log_sink, config_.log_minimum),
          events_(config_.events, &logger_)
    {
        current_ = this;

        events_.SetOwnerSlot(&currentMod_);
        events_.SetFaultHandler(&ModHost::OnHandlerFault, this);

        info_.struct_size = sizeof(info_);
        info_.id          = wxl_str{config_.runtime_id.data(),
                           static_cast<uint32_t>(config_.runtime_id.size())};
        info_.name        = wxl_str{config_.runtime_name.data(),
                             static_cast<uint32_t>(config_.runtime_name.size())};
        info_.version     = config_.runtime_version;
        info_.abi_major   = WXL_ABI_VERSION_MAJOR;
        info_.abi_minor   = WXL_ABI_VERSION_MINOR;

        table_.struct_size     = sizeof(table_);
        table_.abi_major       = WXL_ABI_VERSION_MAJOR;
        table_.abi_minor       = WXL_ABI_VERSION_MINOR;
        table_.log             = &ModHost::ThunkLog;
        table_.runtime         = &ModHost::ThunkRuntime;
        table_.has_service     = &ModHost::ThunkHasService;
        table_.get_service     = &ModHost::ThunkGetService;
        table_.publish_service = &ModHost::ThunkPublishService;
        table_.subscribe       = &ModHost::ThunkSubscribe;
        table_.unsubscribe     = &ModHost::ThunkUnsubscribe;
        table_.emit            = &ModHost::ThunkEmit;

        for (const PublishedService& service : config_.services)
        {
            if (services_.Publish(service.name, service.version, service.iface) != WXL_OK)
                logger_.Warn(kHostTag, "runtime service " + service.name + " version " +
                                           std::to_string(service.version) +
                                           " was not published: null or already taken");
        }
    }

    ModHost::~ModHost()
    {
        if (!loaded_.empty())
        {
            logger_.Warn(kHostTag, "host destroyed with mods loaded; unloading without a "
                                   "shutdown stage");
            UnloadAll();
        }
        if (current_ == this) current_ = nullptr;
    }

    // --- The table -------------------------------------------------------------------------

    void WXL_CALL ModHost::ThunkLog(wxl_log_level level, const char* tag, wxl_str message)
    {
        if (current_) current_->logger_.Write(level, tag ? tag : "?", View(message));
    }

    const wxl_runtime_info* WXL_CALL ModHost::ThunkRuntime(void)
    {
        return current_ ? &current_->info_ : nullptr;
    }

    wxl_bool WXL_CALL ModHost::ThunkHasService(const char* name, uint32_t version)
    {
        if (!current_ || !name) return WXL_FALSE;
        return current_->services_.Has(name, version) ? WXL_TRUE : WXL_FALSE;
    }

    const void* WXL_CALL ModHost::ThunkGetService(const char* name, uint32_t version)
    {
        if (!current_ || !name) return nullptr;
        return current_->services_.Get(name, version);
    }

    wxl_result WXL_CALL ModHost::ThunkPublishService(const char* name, uint32_t version,
                                                     const void* iface)
    {
        if (!current_) return WXL_E_FAILED;
        if (!name) return WXL_E_INVALID;
        return current_->services_.Publish(name, version, iface);
    }

    wxl_subscription WXL_CALL ModHost::ThunkSubscribe(wxl_event event, wxl_event_fn handler,
                                                      void* user, int32_t priority)
    {
        if (!current_) return WXL_SUBSCRIPTION_NONE;
        return current_->events_.Subscribe(event, handler, user, priority, current_->currentMod_);
    }

    void WXL_CALL ModHost::ThunkUnsubscribe(wxl_subscription subscription)
    {
        if (current_) current_->events_.Unsubscribe(subscription);
    }

    void WXL_CALL ModHost::ThunkEmit(wxl_event event, const void* args)
    {
        if (current_) current_->events_.Emit(event, args);
    }

    // --- Faults ----------------------------------------------------------------------------

    void ModHost::OnHandlerFault(void* user, uint32_t owner, wxl_event event, const Fault&)
    {
        ModHost* host = static_cast<ModHost*>(user);
        if (!host) return;
        if (owner == 0)
        {
            host->logger_.Error(kHostTag, std::string("the faulting handler for ") + EventName(event) +
                                              " belongs to no mod; nothing is disabled");
            return;
        }
        host->Disable(owner, std::string("in its handler for ") + EventName(event));
    }

    const LoadedMod* ModHost::ModOf(ModToken token) const
    {
        if (token == 0 || token > loaded_.size()) return nullptr;
        return &loaded_[token - 1];
    }

    void ModHost::Disable(ModToken token, std::string_view what)
    {
        if (token == 0 || token > loaded_.size()) return;
        LoadedMod& mod = loaded_[token - 1];
        if (mod.disabled) return;

        mod.disabled = true;
        mod.fault    = std::string(what);
        events_.DropOwner(token);
        logger_.Error(kHostTag, "mod " + mod.id + " faulted " + mod.fault +
                                    "; it is disabled: its handlers are dropped, its remaining "
                                    "stages and its unload are skipped, its binary stays mapped, "
                                    "and what it published or wrote stays as it was");
        if (disableFn_) disableFn_(disableUser_, token, mod.id.c_str());
    }

    bool ModHost::CallEntry(ModToken token, GuardedFn fn, void* context, char* description,
                            size_t capacity)
    {
        const ModToken previous = SwapCurrentMod(token);
        Fault          fault;
        const bool     returned = CallGuarded(fn, context, &fault);
        SwapCurrentMod(previous);
        if (!returned) DescribeFault(fault, description, capacity);
        return returned;
    }

    // --- Loading ---------------------------------------------------------------------------

    void ModHost::Refuse(const std::string& id, const std::string& directoryName,
                        const std::filesystem::path& directory, Verdict verdict,
                        const std::string& reason)
    {
        const std::string who = id.empty() ? directoryName : id;
        if (verdict == Verdict::Skip) logger_.Info(kHostTag, "skipped " + who + ": " + reason);
        else logger_.Warn(kHostTag, "refused " + who + ": " + reason);
        refused_.push_back(RefusedMod{id, directoryName, directory, verdict, reason});
    }

    void ModHost::TryLoad(const ModCandidate& candidate)
    {
        const Manifest& manifest = candidate.manifest;

        const Decision admitted = AdmitManifest(manifest, config_.target, services_);
        if (!admitted)
        {
            Refuse(manifest.id, candidate.directory_name, candidate.directory, admitted.verdict,
                   admitted.reason);
            return;
        }

        LibraryLoader&              loader = config_.loader ? *config_.loader : PlatformLoader();
        const std::filesystem::path path   = candidate.directory / admitted.binary;
        ModBinary                   binary;
        std::string                 error;
        if (!ModBinary::Open(loader, path, binary, error))
        {
            Refuse(manifest.id, candidate.directory_name, candidate.directory, Verdict::Refuse,
                   error);
            return;
        }

        // The token this mod gets if it loads: its place at the end of loaded_.
        const ModToken token = static_cast<ModToken>(loaded_.size() + 1);
        char           description[96];

        // The first mod code to run, and by contract side-effect free.
        QueryCall query{binary.Entries().query, nullptr};
        if (!CallEntry(token, &InvokeQuery, &query, description, sizeof description))
        {
            events_.DropOwner(token);
            binary.Abandon();
            Refuse(manifest.id, candidate.directory_name, candidate.directory, Verdict::Refuse,
                   std::string("wxl_mod_query faulted: ") + description +
                       "; the binary stays mapped and nothing of it is called again");
            return;
        }

        const Decision accepted = AdmitInfo(manifest, query.result);
        if (!accepted)
        {
            Refuse(manifest.id, candidate.directory_name, candidate.directory, Verdict::Refuse,
                   accepted.reason);
            return;
        }

        LoadedMod loaded;
        loaded.id             = manifest.id;
        loaded.name           = query.result->name ? query.result->name : manifest.name;
        loaded.version        = query.result->version;
        loaded.directory_name = candidate.directory_name;
        loaded.directory      = candidate.directory;
        loaded.binary         = path;
        loaded.manifest       = manifest;
        loaded.has_stage      = binary.Entries().stage != nullptr;
        loaded.has_unload     = binary.Entries().unload != nullptr;

        LoadCall load{binary.Entries().load, &table_, WXL_E_FAILED};
        if (!CallEntry(token, &InvokeLoad, &load, description, sizeof description))
        {
            events_.DropOwner(token);
            binary.Abandon();
            Refuse(manifest.id, candidate.directory_name, candidate.directory, Verdict::Refuse,
                   std::string("wxl_mod_load faulted: ") + description +
                       "; the binary stays mapped and nothing of it is called again");
            return;
        }
        if (load.result != WXL_OK)
        {
            events_.DropOwner(token);
            Refuse(manifest.id, candidate.directory_name, candidate.directory, Verdict::Refuse,
                   std::string("wxl_mod_load returned ") + ResultName(load.result));
            return;
        }

        logger_.Info(kHostTag, "loaded " + loaded.id + " " + manifest.version.text + " (" +
                                   loaded.name + ") from " + admitted.binary);
        loaded_.push_back(std::move(loaded));
        binaries_.push_back(std::move(binary));
    }

    void ModHost::LoadAll()
    {
        if (loadedAll_) return;
        loadedAll_ = true;

        logger_.Info(kHostTag, "scanning " + config_.mods_directory.string());
        const Discovery found = Discover(config_.mods_directory);
        for (const DiscoveryProblem& problem : found.problems)
            Refuse(problem.id, problem.directory_name, problem.directory, Verdict::Refuse,
                   problem.message);

        const LoadOrder order = OrderByLoadAfter(found.mods);
        for (const DiscoveryProblem& problem : order.excluded)
            Refuse(problem.id, problem.directory_name, problem.directory, Verdict::Refuse,
                   problem.message);

        for (const ModCandidate& candidate : order.ordered) TryLoad(candidate);

        logger_.Info(kHostTag, std::to_string(loaded_.size()) + " mod(s) loaded, " +
                                   std::to_string(refused_.size()) + " not");
    }

    // --- Running ---------------------------------------------------------------------------

    void ModHost::Stage(wxl_stage stage)
    {
        logger_.Debug(kHostTag, std::string("stage ") + StageName(stage));
        const size_t count = binaries_.size();

        const auto one = [&](size_t i) {
            if (loaded_[i].disabled || !binaries_[i].Entries().stage) return;
            StageCall call{binaries_[i].Entries().stage, stage};
            char      description[96];
            if (!CallEntry(static_cast<ModToken>(i + 1), &InvokeStage, &call, description,
                           sizeof description))
                Disable(static_cast<ModToken>(i + 1), std::string("in wxl_mod_stage(") +
                                                         StageName(stage) + "): " + description);
        };

        if (stage == WXL_STAGE_SHUTDOWN)
        {
            for (size_t i = count; i-- > 0;) one(i);
        }
        else
        {
            for (size_t i = 0; i < count; ++i) one(i);
        }
    }

    void ModHost::UnloadAll()
    {
        for (size_t i = binaries_.size(); i-- > 0;)
        {
            // Before anything frees the code a handler points at. The client keeps raising
            // events while it takes itself apart -- window messages do not stop because the
            // mods are gone -- so a subscription left behind is a call into a freed binary.
            events_.DropOwner(static_cast<uint32_t>(i + 1));

            if (loaded_[i].disabled)
            {
                logger_.Info(kHostTag, "left " + loaded_[i].id + " mapped: it is disabled");
                binaries_[i].Abandon();
                continue;
            }
            if (binaries_[i].Entries().unload)
            {
                char description[96];
                if (!CallEntry(static_cast<ModToken>(i + 1), &InvokeUnload,
                               reinterpret_cast<void*>(binaries_[i].Entries().unload), description,
                               sizeof description))
                {
                    Disable(static_cast<ModToken>(i + 1),
                            std::string("in wxl_mod_unload: ") + description);
                    binaries_[i].Abandon();
                    continue;
                }
            }
            logger_.Info(kHostTag, "unloaded " + loaded_[i].id);
            binaries_[i].Close();
        }
        binaries_.clear();
        loaded_.clear();
    }

    void ModHost::Shutdown()
    {
        Stage(WXL_STAGE_SHUTDOWN);
        UnloadAll();
    }

    void ModHost::Raise(wxl_event event, const void* args) { events_.Emit(event, args); }
} // namespace wxl::host
