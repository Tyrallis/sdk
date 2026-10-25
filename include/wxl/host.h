// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// What the runtime offers a mod: the table handed to it at load.

#ifndef WXL_HOST_H
#define WXL_HOST_H

#include "wxl/abi.h"
#include "wxl/events.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Severity of a log line.
typedef enum wxl_log_level
{
    WXL_LOG_TRACE = 0,
    WXL_LOG_DEBUG = 1,
    WXL_LOG_INFO  = 2,
    WXL_LOG_WARN  = 3,
    WXL_LOG_ERROR = 4
} wxl_log_level;

/**
 * @brief An event handler.
 * @param user  the pointer given to subscribe.
 * @param args  the event's args struct, per the table in events.def.
 */
typedef void(WXL_CALL* wxl_event_fn)(void* user, const void* args);

/// A live subscription. Zero means the subscribe did not take.
typedef uint32_t wxl_subscription;

#define WXL_SUBSCRIPTION_NONE ((wxl_subscription)0)

/// Runs before handlers with a higher value. Equal values keep subscription order.
#define WXL_PRIORITY_DEFAULT 0

/**
 * @brief Which runtime a mod ended up in.
 *
 * A mod should branch on a service it needs rather than on this, but the identity is here for
 * logs, for diagnostics, and for the rare case where two runtimes differ in a way no service can
 * express.
 */
typedef struct wxl_runtime_info
{
    uint32_t struct_size;
    wxl_str  id;        ///< stable short id: "core", "client", "engine"
    wxl_str  name;      ///< display name, for logs
    uint32_t version;   ///< the runtime's own version, opaque to a mod
    uint32_t abi_major; ///< the contract it implements
    uint32_t abi_minor;

    // --- appended after 0.1.3 ----------------------------------------------------------------

    /**
     * @brief The directory the runtime scans for mods, UTF-8; empty when it scans none.
     *
     * For a mod that serves other mods -- a script runtime reading the manifests the host
     * skipped -- so that both read the same tree. Check @ref struct_size before reading it.
     */
    wxl_str mods_directory;

    // --- appended at 0.1.5 -------------------------------------------------------------------

    /// The feature of the contract it implements, after @ref abi_major and @ref abi_minor.
    uint32_t abi_feature;
} wxl_runtime_info;

/**
 * @brief The runtime's services, handed to a mod at load.
 *
 * The table lives for the process lifetime, so the pointer can be kept. Check @ref struct_size
 * before reading a field a smaller version would not carry.
 *
 * Threads: every function of the table may be called from any thread, from inside a handler or
 * outside one, including a handler of an event raised off the main thread (events.def marks
 * those). Lines logged from two threads at once come out whole, one after the other. A service
 * published on one thread is found by the next lookup on any other. A subscription made, or
 * dropped, while the event is being raised on another thread takes effect from the next raise.
 * What a service's own functions allow is that service's header's to say.
 */
typedef struct wxl_host
{
    uint32_t struct_size;
    uint32_t abi_major;
    uint32_t abi_minor;

    /**
     * @brief Writes one line to the runtime's log.
     *
     * No varargs: formatting is the caller's, so nothing with a variable signature crosses.
     * @p tag and @p message are UTF-8 text: a byte that is not part of a well-formed sequence, and
     * a control character other than a tab or a line break, is written as `\xNN`, so the log
     * stays a text file whatever a mod hands over.
     * @param tag  identifies the caller, since one log serves every mod.
     */
    void(WXL_CALL* log)(wxl_log_level level, const char* tag, wxl_str message);

    /// Describes the runtime. Valid for the process lifetime.
    const wxl_runtime_info*(WXL_CALL* runtime)(void);

    /**
     * @brief Whether a service exists, without resolving it.
     *
     * For deciding what to set up, and for reporting a clean refusal when something required is
     * missing. A missing service is an ordinary answer: no runtime offers all of them.
     */
    wxl_bool(WXL_CALL* has_service)(const char* name, uint32_t version);

    /**
     * @brief Resolves a service.
     *
     * Services arrive as other mods load, so a lookup that fails now may succeed later; resolve on
     * first use rather than at load. The returned pointer is a struct of function pointers defined
     * by the header that publishes the service.
     * @return the service, or NULL when nothing is published under that name and version.
     */
    const void*(WXL_CALL* get_service)(const char* name, uint32_t version);

    /**
     * @brief Offers a service to other mods.
     *
     * The runtime stores name, version and pointer without interpreting them, so two mods can
     * agree on a capability this contract does not model.
     * @param iface  valid for the process lifetime.
     */
    wxl_result(WXL_CALL* publish_service)(const char* name, uint32_t version, const void* iface);

    /**
     * @brief Subscribes to an event.
     * @param priority  see WXL_PRIORITY_DEFAULT.
     * @return a token for unsubscribe, or WXL_SUBSCRIPTION_NONE when the runtime does not raise
     *         that event, which is an answer rather than a fault.
     */
    wxl_subscription(WXL_CALL* subscribe)(wxl_event event, wxl_event_fn handler, void* user,
                                          int32_t priority);

    /// Drops a subscription. Safe inside the handler itself; it stops after that call returns.
    void(WXL_CALL* unsubscribe)(wxl_subscription subscription);

    /**
     * @brief Raises an event to every subscriber, the runtime's own included.
     * @param args  that event's args struct; the runtime forwards it without interpreting it.
     */
    void(WXL_CALL* emit)(wxl_event event, const void* args);

    // --- appended at 0.1.5 -------------------------------------------------------------------

    /**
     * @brief The feature of the contract the runtime implements, after @ref abi_major and
     *        @ref abi_minor.
     *
     * A mod whose manifest asks for less than its headers carry compares it with
     * WXL_ABI_VERSION_FEATURE before using what was appended in between.
     */
    uint32_t abi_feature;

    // --- appended at 0.1.8 -------------------------------------------------------------------

    /**
     * @brief Disables the calling mod, as a fault in its code would.
     *
     * For a failure the mod cannot recover from and the crash guard cannot see: a C++ exception
     * that reached one of its entry points, a broken invariant. The runtime logs @p reason under
     * the mod's id, drops its subscriptions, skips its remaining stages and its unload, and
     * withdraws what the mod registered with the runtime's services. The call returns; the mod
     * then returns from what it was doing and none of its code is called again. Called from code
     * that is no mod's, it does nothing.
     *
     * @param reason  what went wrong, for the log: "OnReady threw an exception: ...".
     */
    void(WXL_CALL* disable)(wxl_str reason);

    // --- appended at 0.1.13 ------------------------------------------------------------------

    /**
     * @brief Whether this runtime raises @p event at all: a subscribe to one it never raises is
     *        refused.
     *
     * For a mod that takes whatever the runtime offers (a probe, a menu of events) rather than
     * asking for one event it needs. Says nothing of whether anyone listens. An event outside
     * the contract the runtime was built with answers false.
     */
    wxl_bool(WXL_CALL* raises)(wxl_event event);
} wxl_host;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_HOST_H
