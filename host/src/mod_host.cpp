// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/host/mod_host.hpp"

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
    } // namespace

    ModHost::ModHost(HostConfig config)
        : config_(std::move(config)), logger_(config_.log_sink, config_.log_minimum),
          events_(config_.events, &logger_)
    {
        current_ = this;

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
        return current_->events_.Subscribe(event, handler, user, priority);
    }

    void WXL_CALL ModHost::ThunkUnsubscribe(wxl_subscription subscription)
    {
        if (current_) current_->events_.Unsubscribe(subscription);
    }

    void WXL_CALL ModHost::ThunkEmit(wxl_event event, const void* args)
    {
        if (current_) current_->events_.Emit(event, args);
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

        // The first mod code to run, and by contract side-effect free.
        const wxl_mod_info* info     = binary.Entries().query();
        const Decision      accepted = AdmitInfo(manifest, info);
        if (!accepted)
        {
            Refuse(manifest.id, candidate.directory_name, candidate.directory, Verdict::Refuse,
                   accepted.reason);
            return;
        }

        LoadedMod loaded;
        loaded.id             = manifest.id;
        loaded.name           = info->name ? info->name : manifest.name;
        loaded.version        = info->version;
        loaded.directory_name = candidate.directory_name;
        loaded.directory      = candidate.directory;
        loaded.binary         = path;
        loaded.manifest       = manifest;
        loaded.has_stage      = binary.Entries().stage != nullptr;
        loaded.has_unload     = binary.Entries().unload != nullptr;

        const wxl_result result = binary.Entries().load(&table_);
        if (result != WXL_OK)
        {
            Refuse(manifest.id, candidate.directory_name, candidate.directory, Verdict::Refuse,
                   std::string("wxl_mod_load returned ") + ResultName(result));
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
        if (stage == WXL_STAGE_SHUTDOWN)
        {
            for (size_t i = count; i-- > 0;)
                if (binaries_[i].Entries().stage) binaries_[i].Entries().stage(stage);
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
                if (binaries_[i].Entries().stage) binaries_[i].Entries().stage(stage);
        }
    }

    void ModHost::UnloadAll()
    {
        for (size_t i = binaries_.size(); i-- > 0;)
        {
            if (binaries_[i].Entries().unload) binaries_[i].Entries().unload();
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
