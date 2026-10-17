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
} wxl_runtime_info;

/**
 * @brief The runtime's services, handed to a mod at load.
 *
 * The table lives for the process lifetime, so the pointer can be kept. Check @ref struct_size
 * before reading a field a smaller version would not carry.
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
} wxl_host;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_HOST_H
