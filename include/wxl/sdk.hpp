// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The C++ layer: a mod is a class, a hook is an override, a resource is an object.
//
// Everything here compiles into the mod and resolves to the C calls in host.h. No C++ type ever
// reaches the runtime -- a script's vtable is read inside the mod, by a thunk the runtime calls
// through a plain function pointer. That is what lets a mod be built with a different toolchain
// than the runtime that loads it.

#ifndef WXL_SDK_HPP
#define WXL_SDK_HPP

#include <algorithm>
#include <atomic>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "wxl/abi.h"
#include "wxl/events.h"
#include "wxl/expected.hpp"
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

    /// Wraps a view for the boundary. The view's storage must outlive the call it is given to.
    inline wxl_str Str(std::string_view text)
    {
        return wxl_str{text.data(), static_cast<uint32_t>(text.size())};
    }

    /// The view over a boundary string. Empty when the runtime answered with nothing.
    inline std::string_view View(wxl_str text)
    {
        return text.data ? std::string_view(text.data, text.size) : std::string_view();
    }

    /// Writes one line to the runtime's log.
    inline void Log(wxl_log_level level, const char* tag, std::string_view message)
    {
        if (const wxl_host* host = Host()) host->log(level, tag, Str(message));
    }

    /// The same for a NUL-terminated string; a null message logs an empty line.
    inline void Log(wxl_log_level level, const char* tag, const char* message)
    {
        if (const wxl_host* host = Host()) host->log(level, tag, Str(message));
    }

    // --- Services ------------------------------------------------------------------------

    /**
     * @brief A raw service table, resolved on first use.
     *
     * Load order is not something a mod can rely on, so nothing is looked up until it is needed,
     * and a lookup that finds nothing is retried on the next use. The typed facades in
     * wxl/services/*.hpp wrap one of these with the name and version sealed in; this is for a
     * table those do not cover, or for reaching a raw table directly.
     *
     *     static wxl::Service<my_api> g_mine("my.service", 1);
     *     if (g_mine) g_mine->call();
     */
    template <class T>
    class Service
    {
    public:
        constexpr Service(const char* name, uint32_t version) : name_(name), version_(version) {}

        /// The table, or nullptr while nobody has published it under that name and version.
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

        constexpr const char* Name() const { return name_; }
        constexpr uint32_t    Version() const { return version_; }

    private:
        const char*      name_;
        uint32_t         version_;
        mutable const T* iface_ = nullptr;
    };

    namespace detail
    {
        /**
         * @brief Shared shape of every typed facade: a stateless object over one cached table.
         *
         * Derived names the table type and carries `kName` and `kVersion`. The table is resolved
         * on first use and shared by every instance, so a facade can be a temporary.
         */
        template <class Derived, class Api>
        class Facade
        {
        public:
            using api_type = Api;

            /// The raw table, or nullptr while the runtime does not publish the service.
            static const Api* Raw()
            {
                static Service<Api> service(Derived::kName, Derived::kVersion);
                return service.Get();
            }

            /// Whether the runtime publishes the service right now.
            static bool Available() { return Raw() != nullptr; }

            explicit operator bool() const { return Available(); }
        };
    } // namespace detail

    namespace detail
    {
        /**
         * @brief Whether a service table really answers for one of its members.
         *
         * Two ways it may not. The runtime's table can be older than this header, and stop
         * short of the member: struct_size says how far it reaches. Or the runtime can reach
         * the member and have nothing true to say through it, and leave it null -- which the
         * contract allows, and which is how a runtime tells a mod that one row of a service is
         * beyond it without refusing the whole service.
         *
         * Either way the answer is the same to a mod: unsupported.
         */
        template <class Api, class Member>
        bool Carries(const Api* api, Member Api::*member)
        {
            if (!api) return false;
            const char* base = reinterpret_cast<const char*>(api);
            const char* end  = reinterpret_cast<const char*>(&(api->*member)) + sizeof(Member);
            if (static_cast<size_t>(end - base) > api->struct_size) return false;
            return (api->*member) != nullptr;
        }
    } // namespace detail

    // --- Subscriptions -------------------------------------------------------------------

    /**
     * @brief A live event subscription, dropped when this goes out of scope.
     *
     * Move-only. Release() hands the token back for code that manages it by hand; Reset() drops
     * it early. Destroying one after the host is gone does nothing.
     */
    class Subscription
    {
    public:
        Subscription() = default;
        explicit Subscription(wxl_subscription token) : token_(token) {}

        Subscription(Subscription&& other) noexcept : token_(other.Release()) {}
        Subscription& operator=(Subscription&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                token_ = other.Release();
            }
            return *this;
        }

        Subscription(const Subscription&)            = delete;
        Subscription& operator=(const Subscription&) = delete;

        ~Subscription() { Reset(); }

        explicit operator bool() const { return token_ != WXL_SUBSCRIPTION_NONE; }
        wxl_subscription Get() const { return token_; }

        /// Gives up ownership without unsubscribing.
        wxl_subscription Release()
        {
            const wxl_subscription token = token_;
            token_                       = WXL_SUBSCRIPTION_NONE;
            return token;
        }

        /// Unsubscribes now. Safe to call more than once and safe inside the handler itself.
        void Reset()
        {
            if (token_ == WXL_SUBSCRIPTION_NONE) return;
            if (const wxl_host* host = Host()) host->unsubscribe(token_);
            token_ = WXL_SUBSCRIPTION_NONE;
        }

    private:
        wxl_subscription token_ = WXL_SUBSCRIPTION_NONE;
    };

    /// The args struct an event carries, per events.def: `wxl::EventArgs<WXL_EV_Update>::type`.
    template <wxl_event Event>
    struct EventArgs;

#define WXL_EVENT(family, name, id, args) \
    template <>                           \
    struct EventArgs<WXL_EV_##name>       \
    {                                     \
        using type = args;                \
    };
#include "wxl/events.def"
#undef WXL_EVENT

    template <wxl_event Event>
    using EventArgsOf = typename EventArgs<Event>::type;

    /// Subscribes a raw handler. An empty Subscription means the runtime does not raise the event.
    inline Subscription Subscribe(wxl_event event, wxl_event_fn handler, void* user,
                                  int32_t priority = WXL_PRIORITY_DEFAULT)
    {
        const wxl_host* host = Host();
        if (!host || !handler) return Subscription();
        return Subscription(host->subscribe(event, handler, user, priority));
    }

    namespace detail
    {
        template <wxl_event Event, class Handler>
        struct HandlerThunk
        {
            static void WXL_CALL Dispatch(void* user, const void* args)
            {
                (*static_cast<Handler*>(user))(*static_cast<const EventArgsOf<Event>*>(args));
            }
        };
    } // namespace detail

    /**
     * @brief Subscribes a callable object to one event, with the args typed.
     *
     * The handler is called as `(*handler)(const EventArgsOf<Event>&)` and must outlive the
     * subscription; a lambda stored in a member works, a temporary does not.
     *
     *     wxl::Subscription tick = wxl::Subscribe<WXL_EV_Update>(&m_onTick);
     */
    template <wxl_event Event, class Handler>
    Subscription Subscribe(Handler* handler, int32_t priority = WXL_PRIORITY_DEFAULT)
    {
        if (!handler) return Subscription();
        return Subscribe(Event, &detail::HandlerThunk<Event, Handler>::Dispatch, handler, priority);
    }

    /// Raises an event to every subscriber, the runtime's own included.
    template <wxl_event Event>
    void Emit(const EventArgsOf<Event>& args)
    {
        if (const wxl_host* host = Host()) host->emit(Event, &args);
    }

    // --- Scripts -------------------------------------------------------------------------
    //
    // One class per family, one virtual per event, generated from events.def. Override what
    // matters and ignore the rest: an event a script does not override is not subscribed, so
    // the client never crosses into the mod for it.
    //
    // An override for an event the runtime never raises is not an error, but it never runs, and
    // the runtime logs a warning naming the mod and the event.
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
        /// One family's registered scripts and the subscriptions that feed them. Lives in the mod.
        template <class Script>
        struct Family
        {
            std::vector<Script*>      scripts; ///< null where a script was removed mid-dispatch
            std::vector<Subscription> subscriptions;
            bool                      fed[WXL_EV_COUNT] = {}; ///< per event: already subscribed
            std::atomic<int>          walking{0}; ///< dispatches walking `scripts` right now

            /// Drops the null slots, when nothing is walking the list.
            void Compact()
            {
                if (walking.load() != 0) return;
                scripts.erase(std::remove(scripts.begin(), scripts.end(), nullptr), scripts.end());
            }
        };

        template <class Script>
        Family<Script>& FamilyOf()
        {
            static Family<Script> family;
            return family;
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
                const Args&     typed  = *static_cast<const Args*>(args);
                Family<Script>& family = FamilyOf<Script>();

                struct Walk
                {
                    std::atomic<int>& walking;
                    explicit Walk(std::atomic<int>& w) : walking(w) { ++walking; }
                    ~Walk() { --walking; }
                } walk(family.walking);

                // By index, over the scripts there were when the event arrived: a script added
                // by a handler waits for the next event, one removed is skipped from here on.
                const size_t count = family.scripts.size();
                for (size_t i = 0; i < count; ++i)
                    if (Script* script = family.scripts[i]) (script->*Method)(typed);
            }
        };

        // --- What a script handles --------------------------------------------------------
        //
        // A family declares an empty handler for each of its events, and a script overrides the
        // ones it wants. Subscribing the whole family the moment one of its scripts is added
        // would charge a mod that handles one texture event a guarded crossing for every file
        // the client opens, and one that wants the connection a crossing for every packet. So a
        // script subscribes only the events it handles.
        //
        // That is decided at compile time from the type of &D::OnName. It names D's own member
        // when D, or a class between D and the family, declares one, and the family's member
        // when D only inherits it. The comparison is between types, which is well defined for
        // virtual functions; comparing the pointers themselves would not be.
        //
        // The rule errs one way only, because leaving out an event a script handles is a mod
        // that compiles and silently does nothing. An event is left out only when its handler is
        // provably the family's empty default. A handler the trait cannot see -- an override
        // declared private or protected, which C++ allows -- is subscribed, and so is every
        // event of a script added through its family's pointer, which hides what it overrides.

        /// Whether D's handler for one event is, provably, the family's empty default.
#define WXL_SDK_DEFAULT_TRAIT(name, args)                                                      \
        template <class D, class F, class = void>                                             \
        struct InheritsDefault_##name : std::false_type                                       \
        {                                                                                     \
        };                                                                                    \
        template <class D, class F>                                                           \
        struct InheritsDefault_##name<D, F, std::void_t<decltype(&D::On##name)>>              \
            : std::is_same<decltype(&D::On##name), void (F::*)(const args&)>                  \
        {                                                                                     \
        };
#define WXL_EVENT(family, name, id, args) WXL_SDK_DEFAULT_TRAIT(name, args)
#include "wxl/events.def"
#undef WXL_EVENT
#undef WXL_SDK_DEFAULT_TRAIT

        /// One family's events, subscribed for a script of type D. Specialised per family below.
        template <class Script>
        struct Rows;

#define WXL_SDK_SUBSCRIBE_ROW(Script, name, args)                                              \
    if constexpr (std::is_same_v<D, Script> || !InheritsDefault_##name<D, Script>::value)      \
    {                                                                                          \
        if (!family.fed[WXL_EV_##name])                                                        \
        {                                                                                      \
            family.fed[WXL_EV_##name] = true;                                                  \
            family.subscriptions.push_back(Subscribe(                                          \
                WXL_EV_##name, &Thunk<Script, args, &Script::On##name>::Dispatch, nullptr,     \
                WXL_PRIORITY_DEFAULT));                                                        \
        }                                                                                      \
    }

        template <>
        struct Rows<WorldScript>
        {
            template <class D>
            static void For(Family<WorldScript>& family)
            {
                (void)family; // a script may handle none of its family's events
#define WXL_EVENT_World(name, id, args) WXL_SDK_SUBSCRIBE_ROW(WorldScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_World
            }
        };

        template <>
        struct Rows<RenderScript>
        {
            template <class D>
            static void For(Family<RenderScript>& family)
            {
                (void)family;
#define WXL_EVENT_Render(name, id, args) WXL_SDK_SUBSCRIBE_ROW(RenderScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_Render
            }
        };

        template <>
        struct Rows<ObjectScript>
        {
            template <class D>
            static void For(Family<ObjectScript>& family)
            {
                (void)family;
#define WXL_EVENT_Object(name, id, args) WXL_SDK_SUBSCRIBE_ROW(ObjectScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_Object
            }
        };

        template <>
        struct Rows<AssetScript>
        {
            template <class D>
            static void For(Family<AssetScript>& family)
            {
                (void)family;
#define WXL_EVENT_Asset(name, id, args) WXL_SDK_SUBSCRIBE_ROW(AssetScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_Asset
            }
        };

        template <>
        struct Rows<NetScript>
        {
            template <class D>
            static void For(Family<NetScript>& family)
            {
                (void)family;
#define WXL_EVENT_Net(name, id, args) WXL_SDK_SUBSCRIBE_ROW(NetScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_Net
            }
        };

        template <>
        struct Rows<LuaScript>
        {
            template <class D>
            static void For(Family<LuaScript>& family)
            {
                (void)family;
#define WXL_EVENT_Lua(name, id, args) WXL_SDK_SUBSCRIBE_ROW(LuaScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_Lua
            }
        };

#undef WXL_SDK_SUBSCRIBE_ROW

        /// How many of the six families a script derives from.
        template <class D>
        constexpr int kFamilies =
            int(std::is_base_of_v<WorldScript, D>) + int(std::is_base_of_v<RenderScript, D>) +
            int(std::is_base_of_v<ObjectScript, D>) + int(std::is_base_of_v<AssetScript, D>) +
            int(std::is_base_of_v<NetScript, D>) + int(std::is_base_of_v<LuaScript, D>);

        /// The family a script belongs to, for one that belongs to exactly one.
        template <class D>
        using FamilyFor = std::conditional_t<
            std::is_base_of_v<WorldScript, D>, WorldScript,
            std::conditional_t<
                std::is_base_of_v<RenderScript, D>, RenderScript,
                std::conditional_t<
                    std::is_base_of_v<ObjectScript, D>, ObjectScript,
                    std::conditional_t<std::is_base_of_v<AssetScript, D>, AssetScript,
                                       std::conditional_t<std::is_base_of_v<NetScript, D>,
                                                          NetScript, LuaScript>>>>>;

        /// Keeps one script, and subscribes what it handles that its family does not have yet.
        template <class Script, class D>
        void Register(D* script)
        {
            if (!script) return;
            Family<Script>& family = FamilyOf<Script>();
            family.Compact();
            Rows<Script>::template For<D>(family);
            family.scripts.push_back(script);
        }

        template <class Script>
        void Unregister(Script* script)
        {
            Family<Script>& family = FamilyOf<Script>();
            for (Script*& slot : family.scripts)
            {
                if (slot != script) continue;
                slot = nullptr;
                break;
            }
            family.Compact();
        }

        /// Drops a family's scripts and its subscriptions; the next Add subscribes again.
        template <class Script>
        void ClearFamily()
        {
            Family<Script>& family = FamilyOf<Script>();
            family.scripts.clear();
            family.subscriptions.clear();
            for (bool& fed : family.fed) fed = false;
        }
    } // namespace detail

    /**
     * @brief Registers scripts.
     *
     * Adding a script subscribes the events it handles, and only those: a script that overrides
     * OnWorldEnter costs the client nothing on any other World event. A second script of the same
     * family subscribes what it handles that the first did not, and nothing twice. The
     * subscriptions are owned here and dropped by Clear(), which WXL_MOD runs at unload so no
     * handler outlives the code it points into.
     *
     * Pass the script itself, as `Add(&script)`: that is what lets Add see what it overrides. A
     * script passed through its family's pointer still works, but Add can no longer tell which
     * events it handles, so it subscribes every event of the family, as if all were overridden.
     *
     * A script belongs to one family. One that derives from two is added once per family, through
     * each family's pointer, which says plainly what it is doing.
     *
     * Add and Remove may be called from a handler: a script added there gets the next event, one
     * removed gets nothing more. Both are for the main thread, where OnLoad, the stages and most
     * events run. The few events raised on other threads (see events.def) read the family's list
     * without a lock, so a family that handles them is set up in OnLoad and left alone after.
     */
    class ScriptMgr
    {
    public:
        template <class D>
        static void Add(D* script)
        {
            static_assert(detail::kFamilies<D> != 0,
                          "ScriptMgr::Add takes a script: a class deriving from WorldScript, "
                          "RenderScript, ObjectScript, AssetScript, NetScript or LuaScript");
            static_assert(detail::kFamilies<D> < 2,
                          "this script derives from more than one family; add it once per family, "
                          "through that family: ScriptMgr::Add(static_cast<wxl::WorldScript*>("
                          "&script)), and so on");
            if constexpr (detail::kFamilies<D> == 1) detail::Register<detail::FamilyFor<D>>(script);
        }

        template <class D>
        static void Remove(D* script)
        {
            static_assert(detail::kFamilies<D> != 0,
                          "ScriptMgr::Remove takes a script: a class deriving from WorldScript, "
                          "RenderScript, ObjectScript, AssetScript, NetScript or LuaScript");
            static_assert(detail::kFamilies<D> < 2,
                          "this script derives from more than one family; remove it once per "
                          "family, through that family");
            if constexpr (detail::kFamilies<D> == 1)
                detail::Unregister<detail::FamilyFor<D>>(script);
        }

        /// Drops every script and every family subscription.
        static void Clear()
        {
            detail::ClearFamily<WorldScript>();
            detail::ClearFamily<RenderScript>();
            detail::ClearFamily<ObjectScript>();
            detail::ClearFamily<AssetScript>();
            detail::ClearFamily<NetScript>();
            detail::ClearFamily<LuaScript>();
        }
    };

    // --- The mod itself ------------------------------------------------------------------

    /**
     * @brief A mod, as the runtime sees it.
     *
     * Derive, override what is needed, and declare it with WXL_MOD. Scripts are usually added in
     * OnLoad; anything that needs a subsystem waits for the stage that brings it up, which is the
     * named virtual for that stage.
     */
    class Mod
    {
    public:
        virtual ~Mod() = default;

        /// Set-up. Nothing the boot brings up exists yet. Return anything but WXL_OK to abort.
        virtual wxl_result OnLoad() { return WXL_OK; }

        /// WXL_STAGE_EARLY: nothing is up yet.
        virtual void OnEarly() {}

        /// WXL_STAGE_STORAGE: archives are mounted and files can be read.
        virtual void OnStorageReady() {}

        /// WXL_STAGE_GRAPHICS: a graphics device exists.
        virtual void OnGraphicsReady() {}

        /// WXL_STAGE_SCRIPT: the script VM exists and an API can be registered on it.
        virtual void OnScriptReady() {}

        /// WXL_STAGE_READY: fully booted, before the first frame.
        virtual void OnReady() {}

        /// WXL_STAGE_SHUTDOWN: coming down. Release what the stages above acquired.
        virtual void OnShutdown() {}

        /**
         * @brief Every stage, after its named virtual.
         *
         * The general hook: for logging the boot, and for a stage appended to the contract after
         * this SDK was built, which has no named virtual yet.
         */
        virtual void OnStage(wxl_stage) {}

        /// Tear-down, after WXL_STAGE_SHUTDOWN. ScriptMgr is cleared right after it returns.
        virtual void OnUnload() {}

        /// Routes a stage to its named virtual, then to OnStage. Called by WXL_MOD.
        void DispatchStage(wxl_stage stage)
        {
            switch (stage)
            {
            case WXL_STAGE_EARLY: OnEarly(); break;
            case WXL_STAGE_STORAGE: OnStorageReady(); break;
            case WXL_STAGE_GRAPHICS: OnGraphicsReady(); break;
            case WXL_STAGE_SCRIPT: OnScriptReady(); break;
            case WXL_STAGE_READY: OnReady(); break;
            case WXL_STAGE_SHUTDOWN: OnShutdown(); break;
            default: break;
            }
            OnStage(stage);
        }
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
        wxl_mod_instance_        = new ModClass();                                            \
        const wxl_result result  = wxl_mod_instance_->OnLoad();                               \
        if (result != WXL_OK)                                                                 \
        {                                                                                     \
            ::wxl::ScriptMgr::Clear();                                                        \
            delete wxl_mod_instance_;                                                         \
            wxl_mod_instance_ = nullptr;                                                      \
        }                                                                                     \
        return result;                                                                        \
    }                                                                                         \
    extern "C" WXL_EXPORT void WXL_CALL wxl_mod_stage(wxl_stage stage)                        \
    {                                                                                         \
        if (wxl_mod_instance_) wxl_mod_instance_->DispatchStage(stage);                       \
    }                                                                                         \
    extern "C" WXL_EXPORT void WXL_CALL wxl_mod_unload(void)                                  \
    {                                                                                         \
        if (wxl_mod_instance_) wxl_mod_instance_->OnUnload();                                 \
        ::wxl::ScriptMgr::Clear();                                                            \
        delete wxl_mod_instance_;                                                             \
        wxl_mod_instance_ = nullptr;                                                          \
    }

#endif // WXL_SDK_HPP
