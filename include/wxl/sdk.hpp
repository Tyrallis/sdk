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
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "wxl/abi.h"
#include "wxl/base.hpp"
#include "wxl/events.h"
#include "wxl/expected.hpp"
#include "wxl/host.h"
#include "wxl/mod.h"
#include "wxl/objects/object.hpp"

namespace wxl
{
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

    // --- Handler parameters ----------------------------------------------------------------
    //
    // A handler receives an event's args as parameters, one per member of the struct in the
    // order of fields.def:
    //
    //     void OnMessageReceived(uint32_t opcode, wxl::Bytes data, wxl::Answer suppressed)
    //
    // A member that only holds the length of a bytes member or the capacity of a buffer is not
    // a parameter of its own: the view it measures carries it. What each kind becomes:
    //
    //   u32 i32 u64 i64 f32   uint32_t int32_t uint64_t int64_t float
    //   bool                  bool
    //   handle                wxl::Object, the handle as a value: As<wxl::Unit>() and the other
    //                         classes of wxl/objects.hpp read the object behind it
    //   guid vec3             wxl_guid wxl_vec3
    //   str                   std::string_view, valid for the call
    //   bytes                 wxl::Bytes, valid for the call
    //   answer                wxl::Answer
    //   buffer                wxl::Buffer
    //   ptr                   the member's own C type
    //   enum                  the C enumeration its row names: wxl_object_type,
    //                         wxl_script_context, ... (enums.def)
    //
    // wxl::HandlerOf<Event> names the list as a function type. The parameters are read from the
    // struct the runtime raised, which stays the C ABI: a C handler still receives the struct.

    /// Bytes an event carries, as a read-only range. Valid for the handler's call only.
    class Bytes
    {
    public:
        constexpr Bytes() = default;
        Bytes(const void* data, uint32_t size)
            : data_(static_cast<const uint8_t*>(data)), size_(data ? size : 0u)
        {
        }

        const uint8_t* data() const { return data_; }
        uint32_t       size() const { return size_; }
        bool           empty() const { return size_ == 0; }
        const uint8_t* begin() const { return data_; }
        const uint8_t* end() const { return data_ + size_; }
        uint8_t        operator[](uint32_t index) const { return data_[index]; }

        /// The same bytes as characters, for a body that is text.
        std::string_view View() const
        {
            return data_ ? std::string_view(reinterpret_cast<const char*>(data_), size_)
                         : std::string_view();
        }

    private:
        const uint8_t* data_ = nullptr;
        uint32_t       size_ = 0;
    };

    /**
     * @brief The flag of an event a handler can act on: `handled`, `suppressed`, `refused`.
     *
     * Set() is a request the runtime honours, and it is never withdrawn: once one handler has
     * answered, the flag stays set for the handlers after it. Value() reads what the handlers
     * before this one decided.
     */
    class Answer
    {
    public:
        constexpr Answer() = default;
        explicit Answer(wxl_bool* flag) : flag_(flag) {}

        /// Whether a handler has answered.
        bool Value() const { return flag_ && *flag_ != WXL_FALSE; }

        /// Answers. False when the runtime passed no flag, so there is nothing to answer.
        bool Set() const
        {
            if (!flag_) return false;
            *flag_ = WXL_TRUE;
            return true;
        }

    private:
        wxl_bool* flag_ = nullptr;
    };

    /**
     * @brief A buffer a handler answers into with a string: FileOpen's `redirect`.
     *
     * It holds at most Capacity() bytes, the terminator included. The last handler to write
     * wins, so a handler that cares reads Value() first to see whether another already answered.
     */
    class Buffer
    {
    public:
        constexpr Buffer() = default;
        Buffer(char* data, uint32_t capacity) : data_(data), capacity_(data ? capacity : 0u) {}

        /// What the buffer holds: empty until a handler writes.
        std::string_view Value() const
        {
            if (!data_) return std::string_view();
            uint32_t length = 0;
            while (length < capacity_ && data_[length]) ++length;
            return std::string_view(data_, length);
        }

        /// Bytes the buffer takes, the terminator included; 0 when the runtime passed none.
        uint32_t Capacity() const { return capacity_; }

        /// Writes @p text and a terminator. False, and nothing written, when it does not fit.
        bool Write(std::string_view text) const
        {
            if (!data_ || text.size() >= capacity_) return false;
            if (!text.empty()) std::memcpy(data_, text.data(), text.size());
            data_[text.size()] = '\0';
            return true;
        }

    private:
        char*    data_     = nullptr;
        uint32_t capacity_ = 0;
    };

    namespace detail
    {
        template <class... Members>
        struct List
        {
        };

        /// The struct and the type behind a pointer to a data member.
        template <class P>
        struct MemberOf;
        template <class C, class T>
        struct MemberOf<T C::*>
        {
            using owner = C;
            using type  = T;
        };

        // How one member of an args struct reaches a handler: `member` is what it reads,
        // `extent` the member holding its length or capacity, for the kinds that have one.

        template <auto M>
        struct AsValue
        {
            static constexpr auto member = M;
            using type                   = typename MemberOf<decltype(M)>::type;
            static type Read(const typename MemberOf<decltype(M)>::owner& args) { return args.*M; }
        };

        template <auto M>
        struct AsObject
        {
            static constexpr auto member = M;
            using type                   = Object;
            static Object Read(const typename MemberOf<decltype(M)>::owner& args)
            {
                return Object(args.*M);
            }
        };

        template <auto M, class Enum>
        struct AsEnum
        {
            static constexpr auto member = M;
            using type                   = Enum;
            static Enum Read(const typename MemberOf<decltype(M)>::owner& args)
            {
                return static_cast<Enum>(args.*M);
            }
        };

        template <auto M>
        struct AsBool
        {
            static constexpr auto member = M;
            using type                   = bool;
            static bool Read(const typename MemberOf<decltype(M)>::owner& args)
            {
                return args.*M != WXL_FALSE;
            }
        };

        template <auto M>
        struct AsText
        {
            static constexpr auto member = M;
            using type                   = std::string_view;
            static std::string_view Read(const typename MemberOf<decltype(M)>::owner& args)
            {
                return View(args.*M);
            }
        };

        template <auto M, auto Size>
        struct AsBytes
        {
            static constexpr auto member = M;
            static constexpr auto extent = Size;
            using type                   = Bytes;
            static Bytes Read(const typename MemberOf<decltype(M)>::owner& args)
            {
                return Bytes(args.*M, args.*Size);
            }
        };

        template <auto M>
        struct AsAnswer
        {
            static constexpr auto member = M;
            using type                   = Answer;
            static Answer Read(const typename MemberOf<decltype(M)>::owner& args)
            {
                return Answer(args.*M);
            }
        };

        template <auto M, auto Capacity>
        struct AsBuffer
        {
            static constexpr auto member = M;
            static constexpr auto extent = Capacity;
            using type                   = Buffer;
            static Buffer Read(const typename MemberOf<decltype(M)>::owner& args)
            {
                return Buffer(args.*M, args.*Capacity);
            }
        };

        /// Closes a struct's list of members; never a parameter.
        struct EndOfArgs
        {
        };

        /// Every member of an args struct, in the order of fields.def, closed by EndOfArgs.
        template <class Args>
        struct MembersOf;

#define WXL_SDK_AS_u32    AsValue
#define WXL_SDK_AS_i32    AsValue
#define WXL_SDK_AS_u64    AsValue
#define WXL_SDK_AS_i64    AsValue
#define WXL_SDK_AS_f32    AsValue
#define WXL_SDK_AS_bool   AsBool
#define WXL_SDK_AS_handle AsObject
#define WXL_SDK_AS_guid   AsValue
#define WXL_SDK_AS_vec3   AsValue
#define WXL_SDK_AS_str    AsText
#define WXL_ARGS(args)                                                                         \
    template <>                                                                                \
    struct MembersOf<args>                                                                     \
    {                                                                                          \
        using type = List<
#define WXL_ARGS_FIELD(args, member, kind)      WXL_SDK_AS_##kind<&args::member>,
#define WXL_ARGS_BYTES(args, member, size)      AsBytes<&args::member, &args::size>,
#define WXL_ARGS_ANSWER(args, member)           AsAnswer<&args::member>,
#define WXL_ARGS_BUFFER(args, member, capacity) AsBuffer<&args::member, &args::capacity>,
#define WXL_ARGS_PTR(args, member, type)        AsValue<&args::member>,
#define WXL_ARGS_ENUM(args, member, type)       AsEnum<&args::member, type>,
#define WXL_ARGS_END(args) EndOfArgs>; };
#include "wxl/fields.def"
#undef WXL_ARGS_END
#undef WXL_ARGS_ENUM
#undef WXL_ARGS_PTR
#undef WXL_ARGS_BUFFER
#undef WXL_ARGS_ANSWER
#undef WXL_ARGS_BYTES
#undef WXL_ARGS_FIELD
#undef WXL_ARGS
#undef WXL_SDK_AS_str
#undef WXL_SDK_AS_vec3
#undef WXL_SDK_AS_guid
#undef WXL_SDK_AS_handle
#undef WXL_SDK_AS_bool
#undef WXL_SDK_AS_f32
#undef WXL_SDK_AS_i64
#undef WXL_SDK_AS_u64
#undef WXL_SDK_AS_i32
#undef WXL_SDK_AS_u32

        template <class M, class = void>
        struct HasMember : std::false_type
        {
        };
        template <class M>
        struct HasMember<M, std::void_t<decltype(M::member)>> : std::true_type
        {
        };

        template <class M, class = void>
        struct HasExtent : std::false_type
        {
        };
        template <class M>
        struct HasExtent<M, std::void_t<decltype(M::extent)>> : std::true_type
        {
        };

        /// Whether @p Other reads @p M as its length or capacity.
        template <class M, class Other>
        constexpr bool MeasuresIt()
        {
            if constexpr (HasMember<M>::value && HasExtent<Other>::value)
            {
                if constexpr (std::is_same_v<std::remove_cv_t<decltype(M::member)>,
                                             std::remove_cv_t<decltype(Other::extent)>>)
                    return M::member == Other::extent;
            }
            return false;
        }

        /// Whether a member is a parameter: a member that no other member of @p All measures.
        template <class M, class... All>
        constexpr bool IsParameter()
        {
            return HasMember<M>::value && !(MeasuresIt<M, All>() || ...);
        }

        template <class... Lists>
        struct Concat
        {
            using type = List<>;
        };
        template <class... A>
        struct Concat<List<A...>>
        {
            using type = List<A...>;
        };
        template <class... A, class... B, class... Rest>
        struct Concat<List<A...>, List<B...>, Rest...> : Concat<List<A..., B...>, Rest...>
        {
        };

        template <class Members>
        struct ParametersOf;
        template <class... All>
        struct ParametersOf<List<All...>>
            : Concat<std::conditional_t<IsParameter<All, All...>(), List<All>, List<>>...>
        {
        };

        /// A handler's parameter list, and the call that reads an args struct into it.
        template <class Parameters>
        struct Unpack;
        template <class... P>
        struct Unpack<List<P...>>
        {
            using Signature = void(typename P::type...);

            /// Calls @p f with @p lead first, then one value per parameter.
            template <class Args, class F, class... Lead>
            static void Call(const Args& args, F&& f, Lead&&... lead)
            {
                (void)args; // a struct may have no parameter
                std::invoke(std::forward<F>(f), std::forward<Lead>(lead)..., P::Read(args)...);
            }
        };

        template <class Args>
        using UnpackOf = Unpack<typename ParametersOf<typename MembersOf<Args>::type>::type>;

        /// Whether @p F can be called with the parameters of a handler's signature.
        template <class F, class Signature>
        struct CallableAs;
        template <class F, class... P>
        struct CallableAs<F, void(P...)> : std::is_invocable<F, P...>
        {
        };
    } // namespace detail

    /**
     * @brief A handler of an event, as a function type.
     *
     * `wxl::HandlerOf<WXL_EV_MessageReceived>` is `void(uint32_t, wxl::Bytes, wxl::Answer)`: the
     * parameters an On<Event> override and a wxl::Subscribe handler take.
     */
    template <wxl_event Event>
    using HandlerOf = typename detail::UnpackOf<EventArgsOf<Event>>::Signature;

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
        /// "UnitDeath" for WXL_EV_UnitDeath; "?" for an id this SDK does not know.
        constexpr const char* EventName(wxl_event event)
        {
            switch (event)
            {
#define WXL_EVENT(family, name, id, args) \
    case WXL_EV_##name: return #name;
#include "wxl/events.def"
#undef WXL_EVENT
            default: return "?";
            }
        }

        /// "ObjectScript::OnUnitDeath": a script's handler, as a log line names it.
        constexpr const char* HandlerName(wxl_event event)
        {
            switch (event)
            {
#define WXL_EVENT(family, name, id, args) \
    case WXL_EV_##name: return #family "Script::On" #name;
#include "wxl/events.def"
#undef WXL_EVENT
            default: return "?";
            }
        }

        /// What a handler that threw is told it cost: that one call, nothing more.
        constexpr const char* kHandlerGoesOn =
            "; that call ended there, and the mod goes on receiving events";

        template <wxl_event Event, class Handler>
        struct HandlerThunk
        {
            static void WXL_CALL Dispatch(void* user, const void* args)
            {
                Thrown thrown;
                const bool returned = Contain(
                    [&] {
                        UnpackOf<EventArgsOf<Event>>::Call(
                            *static_cast<const EventArgsOf<Event>*>(args), *static_cast<Handler*>(user));
                    },
                    thrown);
                if (returned) return;
                char where[96];
                std::snprintf(where, sizeof where, "a wxl::Subscribe handler for %s", EventName(Event));
                LogThrown(where, thrown, kHandlerGoesOn);
            }
        };
    } // namespace detail

    /**
     * @brief Subscribes a callable object to one event, with the args as parameters.
     *
     * The handler is called with the parameters of wxl::HandlerOf<Event>, as an On<Event>
     * override is, and must outlive the subscription; a lambda stored in a member works, a
     * temporary does not.
     *
     *     std::function<void(float)> m_onTick = [](float delta) { ... };
     *     wxl::Subscription tick = wxl::Subscribe<WXL_EV_Update>(&m_onTick);
     */
    template <wxl_event Event, class Handler>
    Subscription Subscribe(Handler* handler, int32_t priority = WXL_PRIORITY_DEFAULT)
    {
        static_assert(detail::CallableAs<Handler&, HandlerOf<Event>>::value,
                      "this handler cannot be called with the event's parameters; they are "
                      "wxl::HandlerOf<Event>, one per member of its args (sdk.hpp, Handler "
                      "parameters)");
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
    // Each virtual takes the event's parameters (see Handler parameters above):
    //
    //     void OnUnitDeath(wxl::Object unit) override;
    //     void OnSoundPlay(uint32_t sound_id, std::string_view path, wxl::Answer suppressed) override;
    //
    // An override for an event the runtime never raises is not an error, but it never runs, and
    // the runtime logs a warning naming the mod and the event.
    //
    // Each block names only its own family: events.def defaults the rest to nothing.

    namespace detail
    {
        /// The first base of every family, before its events'.
        class ScriptRoot
        {
        };

        // One base per event, carrying its virtual. The parameter list is a type, wxl::HandlerOf,
        // so the virtual is declared by a template specialised on it.
#define WXL_EVENT(family, name, id, args)                                                      \
    template <class Signature>                                                                 \
    class Hook##name;                                                                          \
    template <class... P>                                                                      \
    class Hook##name<void(P...)>                                                               \
    {                                                                                          \
    public:                                                                                    \
        virtual void On##name(P...) {}                                                         \
                                                                                               \
    protected:                                                                                 \
        ~Hook##name() = default;                                                               \
    };
#include "wxl/events.def"
#undef WXL_EVENT
    } // namespace detail

    /// The game's own facts: ticks, the world, input, sound.
    class WorldScript : public detail::ScriptRoot
#define WXL_EVENT_World(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_World
    {
    public:
        virtual ~WorldScript() = default;
    };

    /// The slots of a frame.
    class RenderScript : public detail::ScriptRoot
#define WXL_EVENT_Render(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_Render
    {
    public:
        virtual ~RenderScript() = default;
    };

    /// What lives in the world.
    class ObjectScript : public detail::ScriptRoot
#define WXL_EVENT_Object(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_Object
    {
    public:
        virtual ~ObjectScript() = default;
    };

    /// What comes off disk.
    class AssetScript : public detail::ScriptRoot
#define WXL_EVENT_Asset(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_Asset
    {
    public:
        virtual ~AssetScript() = default;
    };

    /// What crosses to the server.
    class NetScript : public detail::ScriptRoot
#define WXL_EVENT_Net(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_Net
    {
    public:
        virtual ~NetScript() = default;
    };

    /// The script VM coming and going.
    class LuaScript : public detail::ScriptRoot
#define WXL_EVENT_Lua(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_Lua
    {
    public:
        virtual ~LuaScript() = default;
    };

    /// What the server reports of fights: attacks, damage, heals, misses.
    class CombatScript : public detail::ScriptRoot
#define WXL_EVENT_Combat(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_Combat
    {
    public:
        virtual ~CombatScript() = default;
    };

    /// What the local player does and is told: movement, levels, experience, money.
    class PlayerScript : public detail::ScriptRoot
#define WXL_EVENT_Player(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_Player
    {
    public:
        virtual ~PlayerScript() = default;
    };

    /// The login screens: the screen shown, the character and realm lists, the login's result.
    class GlueScript : public detail::ScriptRoot
#define WXL_EVENT_Glue(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_Glue
    {
    public:
        virtual ~GlueScript() = default;
    };

    /// What is said: messages received, and the local player's own.
    class ChatScript : public detail::ScriptRoot
#define WXL_EVENT_Chat(name, id, args) , public detail::Hook##name<HandlerOf<WXL_EV_##name>>
#include "wxl/events.def"
#undef WXL_EVENT_Chat
    {
    public:
        virtual ~ChatScript() = default;
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
        template <class Script, class Args, auto Method, wxl_event Event>
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
                // A script that throws loses that call only; the next script still gets it.
                const size_t count = family.scripts.size();
                for (size_t i = 0; i < count; ++i)
                {
                    Script* script = family.scripts[i];
                    if (!script) continue;
                    Thrown thrown;
                    if (!Contain([&] { UnpackOf<Args>::Call(typed, Method, script); }, thrown))
                        LogThrown(HandlerName(Event), thrown, kHandlerGoesOn);
                }
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
        //
        // A member named like a handler but taking other parameters hides the family's virtual
        // instead of overriding it, and would never be called: that is a compile error.

        /// The function type of a pointer to member function, without its class.
        template <class P>
        struct FunctionOf
        {
            using type = void;
        };
        template <class C, class R, class... A>
        struct FunctionOf<R (C::*)(A...)>
        {
            using type = R(A...);
        };

        /// What naming D's handler gives when it cannot be named from here.
        struct Unnamed
        {
        };

        // Probe<name><D>(0) has the type of &D::On<name>, or Unnamed. A function overload rather
        // than a partial specialisation: clang refuses a protected override named in a partial
        // specialisation's base, where it should only fail to match.

        /// Whether D's handler for one event is, provably, the family's empty default; and
        /// whether D declares one with parameters other than the event's.
#define WXL_SDK_DEFAULT_TRAIT(name)                                                            \
        template <class D>                                                                    \
        auto Probe##name(int) -> decltype(&D::On##name);                                      \
        template <class D>                                                                    \
        Unnamed Probe##name(...);                                                             \
        template <class D, class F>                                                           \
        using InheritsDefault_##name =                                                        \
            std::is_same<decltype(Probe##name<D>(0)), decltype(&F::On##name)>;                \
        template <class D, class F>                                                           \
        using Misdeclared_##name = std::bool_constant<                                        \
            !std::is_same_v<decltype(Probe##name<D>(0)), Unnamed> &&                          \
            !std::is_same_v<typename FunctionOf<decltype(Probe##name<D>(0))>::type,           \
                            typename FunctionOf<decltype(&F::On##name)>::type>>;
#define WXL_EVENT(family, name, id, args) WXL_SDK_DEFAULT_TRAIT(name)
#include "wxl/events.def"
#undef WXL_EVENT
#undef WXL_SDK_DEFAULT_TRAIT

        /// One family's events, subscribed for a script of type D. Specialised per family below.
        template <class Script>
        struct Rows;

#define WXL_SDK_SUBSCRIBE_ROW(Script, name, args)                                              \
    static_assert(!Misdeclared_##name<D, Script>::value,                                       \
                  "On" #name " does not take the parameters of the event, so it would never "  \
                  "be called; they are wxl::HandlerOf<WXL_EV_" #name ">, one per member of "   \
                  "its args (sdk.hpp, Handler parameters)");                                   \
    if constexpr (std::is_same_v<D, Script> || !InheritsDefault_##name<D, Script>::value)      \
    {                                                                                          \
        if (!family.fed[WXL_EV_##name])                                                        \
        {                                                                                      \
            family.fed[WXL_EV_##name] = true;                                                  \
            family.subscriptions.push_back(Subscribe(                                          \
                WXL_EV_##name, &Thunk<Script, args, &Script::On##name, WXL_EV_##name>::Dispatch, \
                nullptr, WXL_PRIORITY_DEFAULT));                                               \
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

        template <>
        struct Rows<CombatScript>
        {
            template <class D>
            static void For(Family<CombatScript>& family)
            {
                (void)family;
#define WXL_EVENT_Combat(name, id, args) WXL_SDK_SUBSCRIBE_ROW(CombatScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_Combat
            }
        };

        template <>
        struct Rows<PlayerScript>
        {
            template <class D>
            static void For(Family<PlayerScript>& family)
            {
                (void)family;
#define WXL_EVENT_Player(name, id, args) WXL_SDK_SUBSCRIBE_ROW(PlayerScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_Player
            }
        };

        template <>
        struct Rows<GlueScript>
        {
            template <class D>
            static void For(Family<GlueScript>& family)
            {
                (void)family;
#define WXL_EVENT_Glue(name, id, args) WXL_SDK_SUBSCRIBE_ROW(GlueScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_Glue
            }
        };

        template <>
        struct Rows<ChatScript>
        {
            template <class D>
            static void For(Family<ChatScript>& family)
            {
                (void)family;
#define WXL_EVENT_Chat(name, id, args) WXL_SDK_SUBSCRIBE_ROW(ChatScript, name, args)
#include "wxl/events.def"
#undef WXL_EVENT_Chat
            }
        };

#undef WXL_SDK_SUBSCRIBE_ROW

        /// How many of the ten families a script derives from.
        template <class D>
        constexpr int kFamilies =
            int(std::is_base_of_v<WorldScript, D>) + int(std::is_base_of_v<RenderScript, D>) +
            int(std::is_base_of_v<ObjectScript, D>) + int(std::is_base_of_v<AssetScript, D>) +
            int(std::is_base_of_v<NetScript, D>) + int(std::is_base_of_v<LuaScript, D>) +
            int(std::is_base_of_v<CombatScript, D>) + int(std::is_base_of_v<PlayerScript, D>) +
            int(std::is_base_of_v<GlueScript, D>) + int(std::is_base_of_v<ChatScript, D>);

        /// The family a script belongs to, for one that belongs to exactly one.
        template <class D>
        using FamilyFor = std::conditional_t<
            std::is_base_of_v<WorldScript, D>, WorldScript,
            std::conditional_t<
                std::is_base_of_v<RenderScript, D>, RenderScript,
                std::conditional_t<
                    std::is_base_of_v<ObjectScript, D>, ObjectScript,
                    std::conditional_t<
                        std::is_base_of_v<AssetScript, D>, AssetScript,
                        std::conditional_t<
                            std::is_base_of_v<NetScript, D>, NetScript,
                            std::conditional_t<
                                std::is_base_of_v<LuaScript, D>, LuaScript,
                                std::conditional_t<
                                    std::is_base_of_v<CombatScript, D>, CombatScript,
                                    std::conditional_t<
                                        std::is_base_of_v<PlayerScript, D>, PlayerScript,
                                        std::conditional_t<std::is_base_of_v<GlueScript, D>,
                                                           GlueScript, ChatScript>>>>>>>>>;

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
                          "RenderScript, ObjectScript, AssetScript, NetScript, LuaScript, "
                          "CombatScript, PlayerScript, GlueScript or ChatScript");
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
                          "RenderScript, ObjectScript, AssetScript, NetScript, LuaScript, "
                          "CombatScript, PlayerScript, GlueScript or ChatScript");
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
            detail::ClearFamily<CombatScript>();
            detail::ClearFamily<PlayerScript>();
            detail::ClearFamily<GlueScript>();
            detail::ClearFamily<ChatScript>();
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

        /// Set-up. Nothing the boot brings up exists yet. Return anything but WXL_OK to abort;
        /// an exception that leaves it aborts too, and is logged.
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

        /// Routes a stage to its named virtual, then to OnStage.
        void DispatchStage(wxl_stage stage)
        {
            DispatchNamedStage(stage);
            OnStage(stage);
        }

        /// Calls the named virtual of a stage; nothing for a stage without one.
        void DispatchNamedStage(wxl_stage stage)
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
        }
    };

    // --- What a throw costs ----------------------------------------------------------------
    //
    // No C++ exception crosses into the runtime. One that leaves the mod's own code is caught
    // where the SDK called that code, logged as an error under the mod's id with its what(), and
    // costs what it interrupted:
    //
    //   the mod's constructor, OnLoad       the load: the mod is not loaded
    //   a stage's virtual, OnStage,         the mod: the runtime disables it as it disables a mod
    //   OnUnload                            that faults (wxl_host.disable), and none of its code
    //                                       runs again
    //   a script's handler, a handler       that call: the next script, the next handler and the
    //   given to wxl::Subscribe             next event still run, and the mod stays enabled
    //   a function registered with          that call: wxl.script fails it with what() as the
    //   wxl.script or wxl.ui                script's error; wxl.ui ends that frame's drawing of it
    //
    // A destructor that throws still ends the process: C++ makes destructors noexcept, so
    // nothing can catch it.

    namespace detail
    {
        /// Set once the mod failed beyond recovery: WXL_MOD then calls none of its code.
        inline std::atomic<bool>& FailedRef()
        {
            static std::atomic<bool> failed{false};
            return failed;
        }

        /// "early", "storage", ...: a stage as a log line names it.
        constexpr const char* StageName(wxl_stage stage)
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

        /// The named virtual a stage calls, or null for a stage without one.
        constexpr const char* StageMethod(wxl_stage stage)
        {
            switch (stage)
            {
            case WXL_STAGE_EARLY: return "OnEarly";
            case WXL_STAGE_STORAGE: return "OnStorageReady";
            case WXL_STAGE_GRAPHICS: return "OnGraphicsReady";
            case WXL_STAGE_SCRIPT: return "OnScriptReady";
            case WXL_STAGE_READY: return "OnReady";
            case WXL_STAGE_SHUTDOWN: return "OnShutdown";
            }
            return nullptr;
        }

        /**
         * @brief Stops the mod after a failure it cannot recover from.
         *
         * The runtime disables it through wxl_host.disable. A runtime older than that member
         * cannot, so the mod stops itself as far as it can: its scripts are dropped, and WXL_MOD
         * calls nothing more of it.
         */
        inline void FailMod(const char* reason)
        {
            FailedRef().store(true);
            ScriptMgr::Clear();
            const wxl_host* host = Host();
            if (!host) return;
            if (Carries(host, &wxl_host::disable))
            {
                host->disable(Str(reason));
                return;
            }
            char line[768];
            std::snprintf(line, sizeof line,
                          "%s; this runtime cannot disable a mod (it predates abi 0.1.8), so the "
                          "mod stops itself: its scripts are dropped, and its remaining stages "
                          "and its unload do nothing",
                          reason);
            Log(WXL_LOG_ERROR, ModId(), line);
        }

        /// Fails the mod with "<where> <what it threw>".
        inline void FailThrown(const char* where, const Thrown& thrown)
        {
            char described[320];
            char reason[448];
            std::snprintf(reason, sizeof reason, "%s %s", where,
                          thrown.Describe(described, sizeof described));
            FailMod(reason);
        }

        /// wxl_mod_load, as WXL_MOD writes it.
        template <class M>
        wxl_result LoadMod(const wxl_host* host, const char* id, Mod*& instance)
        {
            static_assert(std::is_base_of_v<Mod, M>,
                          "WXL_MOD takes the mod's class, which derives from wxl::Mod");
            if (!host) return WXL_E_INVALID;
            HostRef()  = host;
            ModIdRef() = id;
            FailedRef().store(false);

            Thrown thrown;
            M*     built = nullptr;
            if (!Contain([&] { built = new M(); }, thrown))
            {
                LogThrown("the constructor of the mod's class", thrown, "; the mod is not loaded");
                return WXL_E_FAILED;
            }

            wxl_result result = WXL_E_FAILED;
            if (!Contain([&] { result = built->OnLoad(); }, thrown))
            {
                LogThrown("OnLoad", thrown, "; the mod is not loaded");
                result = WXL_E_FAILED;
            }
            if (result != WXL_OK)
            {
                ScriptMgr::Clear();
                delete built;
                return result;
            }
            instance = built;
            return WXL_OK;
        }

        /// wxl_mod_stage, as WXL_MOD writes it.
        inline void StageMod(Mod* instance, wxl_stage stage)
        {
            if (!instance || FailedRef().load()) return;

            Thrown thrown;
            if (const char* method = StageMethod(stage))
            {
                if (!Contain([&] { instance->DispatchNamedStage(stage); }, thrown))
                {
                    FailThrown(method, thrown);
                    return;
                }
            }
            if (!Contain([&] { instance->OnStage(stage); }, thrown))
            {
                char where[48];
                std::snprintf(where, sizeof where, "OnStage(%s)", StageName(stage));
                FailThrown(where, thrown);
            }
        }

        /// wxl_mod_unload, as WXL_MOD writes it. A mod that failed is left as it is: none of its
        /// code, its destructor included, runs again.
        inline void UnloadMod(Mod*& instance)
        {
            if (instance && !FailedRef().load())
            {
                Thrown thrown;
                if (!Contain([&] { instance->OnUnload(); }, thrown)) FailThrown("OnUnload", thrown);
            }
            ScriptMgr::Clear();
            if (!FailedRef().load()) delete instance;
            instance = nullptr;
        }
    } // namespace detail
} // namespace wxl

/**
 * @brief Declares the mod: writes the four entry points a runtime looks for.
 *
 * Place it once, at file scope, in exactly one translation unit, with the mod's class, its id
 * -- the manifest's, letter for letter -- and its display name:
 *
 *     WXL_MOD(MyMod, "wxl.example", "Example")
 *
 * The mod's version is the manifest's "version" alone. The entry points catch what the mod's code
 * throws: see "What a throw costs" above.
 */
#define WXL_MOD(ModClass, ModId, ModName)                                                     \
    namespace                                                                                 \
    {                                                                                         \
        ::wxl::Mod* wxl_mod_instance_ = nullptr;                                              \
    }                                                                                         \
    extern "C" WXL_EXPORT const wxl_mod_info* WXL_CALL wxl_mod_query(void)                    \
    {                                                                                         \
        static const wxl_mod_info info = {sizeof(wxl_mod_info), WXL_ABI_VERSION_MAJOR,        \
                                          WXL_ABI_VERSION_MINOR, ModId, ModName, 0u,          \
                                          WXL_ABI_VERSION_FEATURE};                           \
        return &info;                                                                         \
    }                                                                                         \
    extern "C" WXL_EXPORT wxl_result WXL_CALL wxl_mod_load(const wxl_host* host)              \
    {                                                                                         \
        return ::wxl::detail::LoadMod<ModClass>(host, ModId, wxl_mod_instance_);              \
    }                                                                                         \
    extern "C" WXL_EXPORT void WXL_CALL wxl_mod_stage(wxl_stage stage)                        \
    {                                                                                         \
        ::wxl::detail::StageMod(wxl_mod_instance_, stage);                                    \
    }                                                                                         \
    extern "C" WXL_EXPORT void WXL_CALL wxl_mod_unload(void)                                  \
    {                                                                                         \
        ::wxl::detail::UnloadMod(wxl_mod_instance_);                                          \
    }

#endif // WXL_SDK_HPP
