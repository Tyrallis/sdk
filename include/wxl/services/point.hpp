// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.point: Before, Replace and After handlers on the engine's functions.

#ifndef WXL_SERVICE_POINT_HPP
#define WXL_SERVICE_POINT_HPP

#include <atomic>
#include <cstdint>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "wxl/sdk.hpp"
#include "wxl/services/point.h"
#include "wxl/services/rows.hpp"

namespace wxl::api::facade
{
    template <>
    struct Methods<wxl_point_api>
    {
#define WXL_API_TABLE "wxl/services/point.def"
#include "wxl/services/methods.inc"
    };
} // namespace wxl::api::facade

namespace wxl
{
    /**
     * @brief A handler attached to a point, detached when this goes out of scope.
     *
     * Move-only. Release() leaves the handler attached for good (it still goes when the mod
     * unloads). A handler may reset its own attachment from inside itself: what it captured is
     * kept until it returns.
     */
    class PointAttachment
    {
    public:
        /// What a stateful handler's captures live in; the runtime is handed its address.
        struct Holder
        {
            void (*destroy)(Holder*) = nullptr;
            std::atomic<int>  running{0};
            std::atomic<bool> released{false};

            /// Frees the holder once nothing runs in it; called by the side that sees last.
            static void Settle(Holder* holder)
            {
                if (holder && holder->released.load(std::memory_order_acquire) &&
                    holder->running.load(std::memory_order_acquire) == 0)
                    holder->destroy(holder);
            }
        };

        PointAttachment() = default;
        PointAttachment(wxl_point_attachment id, Holder* holder) : id_(id), holder_(holder) {}

        PointAttachment(PointAttachment&& other) noexcept : id_(other.id_), holder_(other.holder_)
        {
            other.id_     = WXL_POINT_ATTACHMENT_NONE;
            other.holder_ = nullptr;
        }
        PointAttachment& operator=(PointAttachment&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                id_           = other.id_;
                holder_       = other.holder_;
                other.id_     = WXL_POINT_ATTACHMENT_NONE;
                other.holder_ = nullptr;
            }
            return *this;
        }

        PointAttachment(const PointAttachment&)            = delete;
        PointAttachment& operator=(const PointAttachment&) = delete;

        ~PointAttachment() { Reset(); }

        explicit operator bool() const { return id_ != WXL_POINT_ATTACHMENT_NONE; }
        wxl_point_attachment Get() const { return id_; }

        /// Leaves the handler attached; its captures are kept for the mod's life.
        wxl_point_attachment Release()
        {
            const wxl_point_attachment id = id_;
            id_                           = WXL_POINT_ATTACHMENT_NONE;
            holder_                       = nullptr;
            return id;
        }

        /// Detaches now. Safe to call more than once, and from inside the handler.
        void Reset()
        {
            if (id_ != WXL_POINT_ATTACHMENT_NONE)
            {
                static Service<wxl_point_api> service(WXL_POINT_SERVICE, WXL_POINT_SERVICE_VERSION);
                if (const wxl_point_api* api = service.Get()) api->detach(id_);
                id_ = WXL_POINT_ATTACHMENT_NONE;
            }
            if (holder_)
            {
                holder_->released.store(true, std::memory_order_release);
                Holder::Settle(holder_);
                holder_ = nullptr;
            }
        }

    private:
        wxl_point_attachment id_     = WXL_POINT_ATTACHMENT_NONE;
        Holder*              holder_ = nullptr;
    };

    /// What a runtime says of one point, copied out.
    struct PointInfo
    {
        wxl_point   id = WXL_POINT_NONE; ///< WXL_POINT_NONE when this runtime does not have it
        std::string name;
        std::string signature;           ///< lowered: "i32(ptr,ptr)"
        uint32_t    thread      = WXL_POINT_THREAD_MAIN;
        uint32_t    flags       = 0;
        uint32_t    layout_hash = 0;
        uint32_t    arg_count   = 0;
        std::string absent;              ///< why this runtime does not have it, empty when it does
    };

    /**
     * @brief The facade over wxl.point by name and id, for code that does not know its point at
     *        compile time. A point known at compile time is better reached through its typed
     *        class, wxl::points::<Point> (below).
     *
     * Find(name), the id of a point this runtime has, is a row of point.def: Expected<uint32_t>,
     * unsupported for a point of another runtime, not_found for a name the table does not hold.
     */
    class PointService : public detail::Facade<PointService, wxl_point_api>,
                         public api::facade::Methods<wxl_point_api>
    {
    public:
        static constexpr const char* kName    = WXL_POINT_SERVICE;
        static constexpr uint32_t    kVersion = WXL_POINT_SERVICE_VERSION;

        /// Every point of the table, this runtime's or not.
        std::vector<PointInfo> Points() const
        {
            std::vector<PointInfo> points;
            const wxl_point_api*   api = Raw();
            if (!api) return points;
            const uint32_t count = api->list(nullptr, 0);
            std::vector<wxl_point_info> raw(count);
            for (wxl_point_info& info : raw) info.struct_size = sizeof(wxl_point_info);
            const uint32_t written = api->list(raw.data(), count);
            for (uint32_t i = 0; i < written && i < count; ++i)
            {
                const wxl_point_info& r = raw[i];
                points.push_back(PointInfo{r.point, std::string(View(r.name)), std::string(View(r.signature)),
                                           r.thread, r.flags, r.layout_hash, r.arg_count,
                                           std::string(View(r.absent))});
            }
            return points;
        }

        /// Attaches a handler of the generic shape: the call, with every argument by address.
        PointAttachment Attach(wxl_point point, uint32_t side, wxl_point_fn handler, void* user = nullptr,
                               int32_t priority = WXL_POINT_PRIORITY_DEFAULT, uint32_t layout_hash = 0) const
        {
            const wxl_point_api* api = Raw();
            if (!api || !handler) return PointAttachment();
            return PointAttachment(api->attach(point, side, handler, user, priority, layout_hash), nullptr);
        }

        /// From a Replace handler of the generic shape: the rest of the chain, then the function.
        void CallOriginal(wxl_point_call* call) const
        {
            if (const wxl_point_api* api = Raw()) api->call_original(call);
        }
    };

    namespace detail::point
    {
        /// One type in a parameter list by position.
        template <size_t I, class... A>
        using At = std::tuple_element_t<I, std::tuple<A...>>;

        /// The callable a handler is: stateless ones are rebuilt at each call, others live in a holder.
        template <class F>
        constexpr bool kStateless = std::is_empty_v<F> && std::is_default_constructible_v<F>;

        template <class F>
        struct Held : PointAttachment::Holder
        {
            F fn;
            explicit Held(F&& f) : fn(std::move(f)) { destroy = [](Holder* h) { delete static_cast<Held*>(h); }; }
        };

        /// Runs the handler @p F, stateless or held in @p user, under the boundary's catch.
        template <class F, class Body>
        bool Run(void* user, const char* where, Body&& body)
        {
            Thrown thrown;
            bool   returned;
            if constexpr (kStateless<F>)
            {
                (void)user;
                F fn{};
                returned = Contain([&] { body(fn); }, thrown);
            }
            else
            {
                Held<F>* held = static_cast<Held<F>*>(user);
                held->running.fetch_add(1, std::memory_order_acq_rel);
                returned = Contain([&] { body(held->fn); }, thrown);
                held->running.fetch_sub(1, std::memory_order_acq_rel);
                PointAttachment::Holder::Settle(held);
            }
            if (!returned) LogThrown(where, thrown, "; the call goes on");
            return returned;
        }
    } // namespace detail::point

    /**
     * @brief The original of a point, as a typed Replace handler is handed it: calling it runs
     *        the Replace handlers below this one, then the function, with these arguments.
     */
    template <class R, class... A>
    class PointOriginal
    {
    public:
        PointOriginal(wxl_point_call* call, bool* called) : call_(call), called_(called) {}

        R operator()(A... args) const
        {
            Store(std::index_sequence_for<A...>{}, args...);
            if (called_) *called_ = true;
            static Service<wxl_point_api> service(WXL_POINT_SERVICE, WXL_POINT_SERVICE_VERSION);
            if (const wxl_point_api* api = service.Get()) api->call_original(call_);
            if constexpr (!std::is_void_v<R>) return *static_cast<R*>(call_->result);
        }

        /// The arguments as the call holds them now, which is what the original would be given.
        template <size_t I>
        detail::point::At<I, A...>& Arg() const
        {
            return *static_cast<detail::point::At<I, A...>*>(call_->args[I]);
        }

        wxl_point_call* Call() const { return call_; }

    private:
        template <size_t... I>
        void Store(std::index_sequence<I...>, A... args) const
        {
            ((*static_cast<A*>(call_->args[I]) = args), ...);
        }

        wxl_point_call* call_;
        bool*           called_; ///< the facade's, read when the handler threw
    };

    namespace detail::point
    {
        /**
         * @brief What every generated point class is: its typed Before, Replace and After.
         *
         * @tparam P    the generated class (wxl::points::<Point>), which carries the row.
         * @tparam Sig  its signature as the contract has it: every pointer void*, `self` first.
         */
        template <class P, class Sig>
        struct Typed;

        template <class P, class R, class... A>
        struct Typed<P, R(A...)>
        {
            using Original = PointOriginal<R, A...>;

            /// The id of the point on this runtime, WXL_POINT_NONE when it does not have it.
            static wxl_point Id()
            {
                static std::atomic<wxl_point> id{WXL_POINT_NONE};
                wxl_point known = id.load(std::memory_order_acquire);
                if (known != WXL_POINT_NONE) return known;
                const wxl_point_api* api = PointService::Raw();
                uint32_t             found = WXL_POINT_NONE;
                if (api && api->find(P::kName, &found) == WXL_OK) id.store(found, std::memory_order_release);
                return found;
            }

            /// Whether this runtime has the point.
            static bool Here() { return Id() != WXL_POINT_NONE; }

            /// Why this runtime does not have the point ("its x86 body is ..."), empty when it does.
            static std::string Absent()
            {
                for (const PointInfo& info : PointService().Points())
                    if (info.name == P::kName) return info.absent;
                return PointService::Available() ? std::string("this runtime's table does not list it")
                                                 : std::string("this runtime does not publish wxl.point");
            }

            /**
             * @brief Runs @p fn when the function is entered: fn(A&... args), each argument by
             *        reference, so writing one changes what the function is given.
             */
            template <class F>
            static PointAttachment Before(F fn, int32_t priority = WXL_POINT_PRIORITY_DEFAULT)
            {
                return Attach<F>(WXL_POINT_BEFORE, std::move(fn), priority, &RunBefore<F>);
            }

            /**
             * @brief Runs @p fn instead of the function: fn(original, A... args) answers in its
             *        place; original(args...) runs the rest of the chain and the function.
             */
            template <class F>
            static PointAttachment Replace(F fn, int32_t priority = WXL_POINT_PRIORITY_DEFAULT)
            {
                static_assert(P::kReplaceable, "this point is observe only: attach Before or After");
                return Attach<F>(WXL_POINT_REPLACE, std::move(fn), priority, &RunReplace<F>);
            }

            /**
             * @brief Runs @p fn once the function returned: fn(R& result, A... args), or fn(A...
             *        args) for a function that answers nothing; writing result changes the answer.
             */
            template <class F>
            static PointAttachment After(F fn, int32_t priority = WXL_POINT_PRIORITY_DEFAULT)
            {
                return Attach<F>(WXL_POINT_AFTER, std::move(fn), priority, &RunAfter<F>);
            }

        private:
            template <class F>
            static PointAttachment Attach(uint32_t side, F&& fn, int32_t priority, wxl_point_fn trampoline)
            {
                static_assert(P::kThread == WXL_POINT_THREAD_MAIN || kStateless<F>,
                              "this point is called on any thread, several at once: its handler holds no state "
                              "(a lambda without captures, or a function)");
                const wxl_point_api* api = PointService::Raw();
                const wxl_point      id  = Id();
                if (!api || id == WXL_POINT_NONE)
                {
                    const std::string why = std::string("wxl::points: ") + P::kName + " is not attached: " + Absent();
                    Log(WXL_LOG_WARN, ModId(), why);
                    return PointAttachment();
                }
                PointAttachment::Holder* holder = nullptr;
                if constexpr (!kStateless<F>) holder = new Held<F>(std::move(fn));
                else (void)fn;
                const wxl_point_attachment attached =
                    api->attach(id, side, trampoline, holder, priority, P::kLayoutHash);
                if (attached == WXL_POINT_ATTACHMENT_NONE && holder)
                {
                    holder->destroy(holder);
                    holder = nullptr;
                }
                return PointAttachment(attached, holder);
            }

            template <class F, size_t... I>
            static void CallBefore(F& fn, wxl_point_call* call, std::index_sequence<I...>)
            {
                fn(*static_cast<A*>(call->args[I])...);
            }

            template <class F>
            static void WXL_CALL RunBefore(wxl_point_call* call, void* user)
            {
                Run<F>(user, "a Before handler", [&](F& fn) { CallBefore(fn, call, std::index_sequence_for<A...>{}); });
            }

            template <class F, size_t... I>
            static void CallReplace(F& fn, const Original& original, wxl_point_call* call, std::index_sequence<I...>)
            {
                if constexpr (std::is_void_v<R>)
                    fn(original, *static_cast<A*>(call->args[I])...);
                else
                    *static_cast<R*>(call->result) = fn(original, *static_cast<A*>(call->args[I])...);
            }

            template <class F>
            static void WXL_CALL RunReplace(wxl_point_call* call, void* user)
            {
                bool           called = false;
                const Original original(call, &called);
                if (!Run<F>(user, "a Replace handler",
                            [&](F& fn) { CallReplace(fn, original, call, std::index_sequence_for<A...>{}); }) &&
                    !called)
                {
                    // It threw before calling the original: the call goes on as if it had not been there.
                    PointService().CallOriginal(call);
                }
            }

            template <class F, size_t... I>
            static void CallAfter(F& fn, wxl_point_call* call, std::index_sequence<I...>)
            {
                if constexpr (std::is_void_v<R>)
                    fn(*static_cast<A*>(call->args[I])...);
                else
                    fn(*static_cast<R*>(call->result), *static_cast<A*>(call->args[I])...);
            }

            template <class F>
            static void WXL_CALL RunAfter(wxl_point_call* call, void* user)
            {
                Run<F>(user, "an After handler", [&](F& fn) { CallAfter(fn, call, std::index_sequence_for<A...>{}); });
            }
        };
    } // namespace detail::point

    /**
     * @brief One class per point of wxl/points.def, with its row as constants and its typed
     *        Before, Replace and After:
     *
     *     wxl::PointAttachment smooth = wxl::points::CGCamera_UpdateCallback::Replace(
     *         [](auto original, void* data, void* camera) { return original(data, camera); });
     *
     * Attaching passes the point's layout hash: a mod built against another signature of the
     * point than the runtime's is refused, and the log says to rebuild it.
     */
    namespace points
    {
#define WXL_POINT_POLICY_replace    true
#define WXL_POINT_POLICY_observe    false
#define WXL_POINT_THREAD_main       WXL_POINT_THREAD_MAIN
#define WXL_POINT_THREAD_any        WXL_POINT_THREAD_ANY
#define WXL_POINT_STABILITY_stable      true
#define WXL_POINT_STABILITY_provisional false
#define WXL_POINT_RUNTIMES_both     (WXL_POINT_ON_X86 | WXL_POINT_ON_X64)
#define WXL_POINT_RUNTIMES_x86      WXL_POINT_ON_X86
#define WXL_POINT_RUNTIMES_x64      WXL_POINT_ON_X64
#define WXL_POINT(Ident, name, ret, params, lowered, layout_hash, policy, thread, stability, runtimes) \
    struct Ident : ::wxl::detail::point::Typed<Ident, ret params>                                         \
    {                                                                                                    \
        using Signature                              = ret params;                                       \
        static constexpr const char* kName           = name;                                             \
        static constexpr const char* kLowered        = lowered;                                          \
        static constexpr uint32_t    kLayoutHash     = layout_hash;                                      \
        static constexpr bool        kReplaceable    = WXL_POINT_POLICY_##policy;                        \
        static constexpr uint32_t    kThread         = WXL_POINT_THREAD_##thread;                        \
        static constexpr bool        kStable         = WXL_POINT_STABILITY_##stability;                  \
        static constexpr uint32_t    kRuntimes       = WXL_POINT_RUNTIMES_##runtimes;                    \
    };
#include "wxl/points.def"
#undef WXL_POINT
#undef WXL_POINT_RUNTIMES_x64
#undef WXL_POINT_RUNTIMES_x86
#undef WXL_POINT_RUNTIMES_both
#undef WXL_POINT_STABILITY_provisional
#undef WXL_POINT_STABILITY_stable
#undef WXL_POINT_THREAD_any
#undef WXL_POINT_THREAD_main
#undef WXL_POINT_POLICY_observe
#undef WXL_POINT_POLICY_replace
    } // namespace points
} // namespace wxl

#endif // WXL_SERVICE_POINT_HPP
