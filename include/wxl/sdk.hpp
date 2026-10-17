// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The C++ layer: a mod is a class, a hook is an override.
//
// Everything here compiles into the mod and resolves to the C calls in host.h. No C++ type ever
// reaches the runtime -- a script's vtable is read inside the mod, by a thunk the runtime calls
// through a plain function pointer. That is what lets a mod be built with a different toolchain
// than the runtime that loads it.

#ifndef WXL_SDK_HPP
#define WXL_SDK_HPP

#include <cstring>
#include <type_traits>
#include <vector>

#include "wxl/abi.h"
#include "wxl/events.h"
#include "wxl/host.h"
#include "wxl/mod.h"

namespace wxl
{
    namespace detail
    {
        /// The runtime table, set once by WXL_MOD before any of the mod's own code runs.
        inline const wxl_host*& HostRef()
        {
            static const wxl_host* host = nullptr;
            return host;
        }
    } // namespace detail

    /// The runtime that loaded this mod.
    inline const wxl_host* Host() { return detail::HostRef(); }

    /// Wraps a NUL-terminated string for the boundary.
    inline wxl_str Str(const char* text)
    {
        return wxl_str{text, text ? static_cast<uint32_t>(std::strlen(text)) : 0u};
    }

    /// Writes one line to the runtime's log.
    inline void Log(wxl_log_level level, const char* tag, const char* message)
    {
        if (const wxl_host* host = Host()) host->log(level, tag, Str(message));
    }

    // --- Services ------------------------------------------------------------------------

    /**
     * @brief A capability another mod or the runtime published, resolved on first use.
     *
     * Load order is not something a mod can rely on, so nothing is looked up until it is needed:
     *
     *     static wxl::Service<wxl_script_api> g_script(WXL_SCRIPT_SERVICE, 1);
     *     if (g_script) g_script->RegisterFunction("MyCall", &MyCall);
     *
     * A lookup that finds nothing is retried on the next use, so a service published later is
     * still picked up. Testing the handle is how a mod stays portable across runtimes that do not
     * all offer the same things.
     */
    template <class T>
    class Service
    {
    public:
        constexpr Service(const char* name, uint32_t version) : name_(name), version_(version) {}

        /// The service, or nullptr while nobody has published it under that name and version.
        const T* Get() const
        {
            if (!iface_)
            {
                if (const wxl_host* host = Host())
                    iface_ = static_cast<const T*>(host->get_service(name_, version_));
            }
            return iface_;
        }

        const T* operator->() const { return Get(); }
        explicit operator bool() const { return Get() != nullptr; }

    private:
        const char*      name_;
        uint32_t         version_;
        mutable const T* iface_ = nullptr;
    };

    // --- Scripts -------------------------------------------------------------------------
    //
    // One class per family, one virtual per event, generated from events.def. Override what
    // matters and ignore the rest; an empty override costs nothing.
    //
    // An override a runtime never calls is not an error. Declare the event's service in the
    // manifest when a hook is load-bearing, so the mod is refused with a reason instead of
    // running half-dead.
    //
    // Each block names only its own family: events.def defaults the rest to nothing.

    /// The game's own facts: ticks, the world, input, sound.
    class WorldScript
    {
    public:
        virtual ~WorldScript() = default;
#define WXL_EVENT_World(name, id, args) virtual void On##name(const args&) {}
#include "wxl/events.def"
#undef WXL_EVENT_World
    };

    /// The slots of a frame.
    class RenderScript
    {
    public:
        virtual ~RenderScript() = default;
#define WXL_EVENT_Render(name, id, args) virtual void On##name(const args&) {}
#include "wxl/events.def"
#undef WXL_EVENT_Render
    };

    /// What lives in the world.
    class ObjectScript
    {
    public:
        virtual ~ObjectScript() = default;
#define WXL_EVENT_Object(name, id, args) virtual void On##name(const args&) {}
#include "wxl/events.def"
#undef WXL_EVENT_Object
    };

    /// What comes off disk.
    class AssetScript
    {
    public:
        virtual ~AssetScript() = default;
#define WXL_EVENT_Asset(name, id, args) virtual void On##name(const args&) {}
#include "wxl/events.def"
#undef WXL_EVENT_Asset
    };

    /// What crosses to the server.
    class NetScript
    {
    public:
        virtual ~NetScript() = default;
#define WXL_EVENT_Net(name, id, args) virtual void On##name(const args&) {}
#include "wxl/events.def"
#undef WXL_EVENT_Net
    };

    /// The script VM coming and going.
    class LuaScript
    {
    public:
        virtual ~LuaScript() = default;
#define WXL_EVENT_Lua(name, id, args) virtual void On##name(const args&) {}
#include "wxl/events.def"
#undef WXL_EVENT_Lua
    };

    namespace detail
    {
        /// Every registered script of one family. Lives in the mod.
        template <class Script>
        std::vector<Script*>& Scripts()
        {
            static std::vector<Script*> scripts;
            return scripts;
        }

        /**
         * @brief What the runtime calls: a plain function that walks the family and dispatches.
         *
         * The virtual call happens here, inside the mod, so no vtable crosses the boundary.
         */
        template <class Script, class Args, void (Script::*Method)(const Args&)>
        struct Thunk
        {
            static void WXL_CALL Dispatch(void*, const void* args)
            {
                const Args& typed = *static_cast<const Args*>(args);
                for (Script* script : Scripts<Script>()) (script->*Method)(typed);
            }
        };

        template <class>
        struct NotAFamily : std::false_type
        {
        };

        /// Subscribes one thunk per event of a family. Specialised per family below.
        template <class Script>
        void SubscribeFamily()
        {
            static_assert(NotAFamily<Script>::value,
                          "ScriptMgr::Add takes a WorldScript, RenderScript, ObjectScript, "
                          "AssetScript, NetScript or LuaScript");
        }

        template <>
        inline void SubscribeFamily<WorldScript>()
        {
            const wxl_host* host = Host();
            if (!host) return;
#define WXL_EVENT_World(name, id, args)                                                   \
    host->subscribe(WXL_EV_##name,                                                        \
                    &Thunk<WorldScript, args, &WorldScript::On##name>::Dispatch, nullptr, \
                    WXL_PRIORITY_DEFAULT);
#include "wxl/events.def"
#undef WXL_EVENT_World
        }

        template <>
        inline void SubscribeFamily<RenderScript>()
        {
            const wxl_host* host = Host();
            if (!host) return;
#define WXL_EVENT_Render(name, id, args)                                                    \
    host->subscribe(WXL_EV_##name,                                                          \
                    &Thunk<RenderScript, args, &RenderScript::On##name>::Dispatch, nullptr, \
                    WXL_PRIORITY_DEFAULT);
#include "wxl/events.def"
#undef WXL_EVENT_Render
        }

        template <>
        inline void SubscribeFamily<ObjectScript>()
        {
            const wxl_host* host = Host();
            if (!host) return;
#define WXL_EVENT_Object(name, id, args)                                                    \
    host->subscribe(WXL_EV_##name,                                                          \
                    &Thunk<ObjectScript, args, &ObjectScript::On##name>::Dispatch, nullptr, \
                    WXL_PRIORITY_DEFAULT);
#include "wxl/events.def"
#undef WXL_EVENT_Object
        }

        template <>
        inline void SubscribeFamily<AssetScript>()
        {
            const wxl_host* host = Host();
            if (!host) return;
#define WXL_EVENT_Asset(name, id, args)                                                   \
    host->subscribe(WXL_EV_##name,                                                        \
                    &Thunk<AssetScript, args, &AssetScript::On##name>::Dispatch, nullptr, \
                    WXL_PRIORITY_DEFAULT);
#include "wxl/events.def"
#undef WXL_EVENT_Asset
        }

        template <>
        inline void SubscribeFamily<NetScript>()
        {
            const wxl_host* host = Host();
            if (!host) return;
#define WXL_EVENT_Net(name, id, args)                                                 \
    host->subscribe(WXL_EV_##name,                                                    \
                    &Thunk<NetScript, args, &NetScript::On##name>::Dispatch, nullptr, \
                    WXL_PRIORITY_DEFAULT);
#include "wxl/events.def"
#undef WXL_EVENT_Net
        }

        template <>
        inline void SubscribeFamily<LuaScript>()
        {
            const wxl_host* host = Host();
            if (!host) return;
#define WXL_EVENT_Lua(name, id, args)                                                 \
    host->subscribe(WXL_EV_##name,                                                    \
                    &Thunk<LuaScript, args, &LuaScript::On##name>::Dispatch, nullptr, \
                    WXL_PRIORITY_DEFAULT);
#include "wxl/events.def"
#undef WXL_EVENT_Lua
        }

        /// Keeps one script, and subscribes its family the first time that family is used.
        template <class Family>
        void Register(Family* script)
        {
            if (!script) return;
            static const bool subscribed = (SubscribeFamily<Family>(), true);
            (void)subscribed;
            Scripts<Family>().push_back(script);
        }
    } // namespace detail

    /**
     * @brief Registers scripts.
     *
     * A family subscribes once, the first time one of its scripts is added, so a mod pays only for
     * the families it uses.
     *
     * The overloads take the family, not the derived type: a script that belongs to two families
     * is added twice, once per family, which says plainly what it is doing.
     */
    class ScriptMgr
    {
    public:
        static void Add(WorldScript* script) { detail::Register(script); }
        static void Add(RenderScript* script) { detail::Register(script); }
        static void Add(ObjectScript* script) { detail::Register(script); }
        static void Add(AssetScript* script) { detail::Register(script); }
        static void Add(NetScript* script) { detail::Register(script); }
        static void Add(LuaScript* script) { detail::Register(script); }
    };

    // --- The mod itself ------------------------------------------------------------------

    /**
     * @brief A mod, as the runtime sees it.
     *
     * Derive, override what is needed, and declare it with WXL_MOD. Scripts are usually added in
     * OnLoad; anything that needs a subsystem waits for its stage.
     */
    class Mod
    {
    public:
        virtual ~Mod() = default;

        /// Set-up. Nothing the boot brings up exists yet. Return anything but WXL_OK to abort.
        virtual wxl_result OnLoad() { return WXL_OK; }

        /// The client reached a boot stage.
        virtual void OnStage(wxl_stage) {}

        /// Tear-down, after WXL_STAGE_SHUTDOWN.
        virtual void OnUnload() {}
    };
} // namespace wxl

/**
 * @brief Declares the mod: writes the four entry points a runtime looks for.
 *
 * Place it once, at file scope, in exactly one translation unit:
 *
 *     WXL_MOD(MyMod, "wxl.example", "Example", 1)
 */
#define WXL_MOD(ModClass, ModId, ModName, ModVersion)                                         \
    namespace                                                                                 \
    {                                                                                         \
        ModClass* wxl_mod_instance_ = nullptr;                                                \
    }                                                                                         \
    extern "C" WXL_EXPORT const wxl_mod_info* WXL_CALL wxl_mod_query(void)                    \
    {                                                                                         \
        static const wxl_mod_info info = {sizeof(wxl_mod_info), WXL_ABI_VERSION_MAJOR,        \
                                          WXL_ABI_VERSION_MINOR, ModId, ModName, ModVersion}; \
        return &info;                                                                         \
    }                                                                                         \
    extern "C" WXL_EXPORT wxl_result WXL_CALL wxl_mod_load(const wxl_host* host)              \
    {                                                                                         \
        if (!host) return WXL_E_INVALID;                                                      \
        ::wxl::detail::HostRef() = host;                                                      \
        wxl_mod_instance_ = new ModClass();                                                   \
        return wxl_mod_instance_->OnLoad();                                                   \
    }                                                                                         \
    extern "C" WXL_EXPORT void WXL_CALL wxl_mod_stage(wxl_stage stage)                        \
    {                                                                                         \
        if (wxl_mod_instance_) wxl_mod_instance_->OnStage(stage);                             \
    }                                                                                         \
    extern "C" WXL_EXPORT void WXL_CALL wxl_mod_unload(void)                                  \
    {                                                                                         \
        if (wxl_mod_instance_) wxl_mod_instance_->OnUnload();                                 \
        delete wxl_mod_instance_;                                                             \
        wxl_mod_instance_ = nullptr;                                                          \
    }

#endif // WXL_SDK_HPP
