// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The net service: talking to the world server.
//
// Receiving is an event (Net/MessageReceived), sending is a call. A mod that suppresses an
// incoming opcode the client does not know, and answers it itself, has a private protocol with a
// server module without either side patching the other.

#ifndef WXL_SERVICE_NET_H
#define WXL_SERVICE_NET_H

#include "wxl/abi.h"

#define WXL_NET_SERVICE         "wxl.net"
#define WXL_NET_SERVICE_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/// Where the client is in its conversation with the server.
typedef enum wxl_net_state
{
    WXL_NET_OFFLINE      = 0, ///< nothing open
    WXL_NET_CONNECTING   = 1,
    WXL_NET_CONNECTED    = 2, ///< the socket is up
    WXL_NET_AUTHENTICATED = 3 ///< logged in, messages flow
} wxl_net_state;

typedef struct wxl_net_api
{
    uint32_t struct_size;

    /// A wxl_net_state value.
    uint32_t(WXL_CALL* state)(void);

    /**
     * @brief Sends a message to the server.
     *
     * @p data is the body, without the opcode; the runtime frames it. Sending while not
     * authenticated is WXL_E_FAILED rather than a crash, because a mod cannot see the state
     * change between its check and its send.
     */
    wxl_result(WXL_CALL* send)(uint32_t opcode, const void* data, uint32_t size);

    /**
     * @brief Claims an opcode the client itself does not handle.
     *
     * A claimed opcode reaches Net/MessageReceived and then stops, instead of being reported as
     * unknown and dropping the connection on a strict client. Two mods claiming the same opcode
     * is WXL_E_INVALID for the second: silent sharing would make the bug unfindable.
     */
    wxl_result(WXL_CALL* claim_opcode)(uint32_t opcode);

    /// Gives back a claimed opcode.
    wxl_result(WXL_CALL* release_opcode)(uint32_t opcode);

    /// The highest opcode the client itself uses. Anything above is free to claim.
    uint32_t(WXL_CALL* reserved_opcode_limit)(void);
} wxl_net_api;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WXL_SERVICE_NET_H
