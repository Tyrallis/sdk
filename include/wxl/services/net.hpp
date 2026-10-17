// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.net: sending to the world server and claiming opcodes.

#ifndef WXL_SERVICE_NET_HPP
#define WXL_SERVICE_NET_HPP

#include <type_traits>

#include "wxl/sdk.hpp"
#include "wxl/services/net.h"

namespace wxl
{
    /**
     * @brief A claimed opcode, given back when this goes out of scope.
     *
     * Move-only. Release() keeps the claim for code that gives it back by hand.
     */
    class OpcodeClaim
    {
    public:
        OpcodeClaim() = default;
        OpcodeClaim(const wxl_net_api* api, uint32_t opcode) : api_(api), opcode_(opcode), live_(true) {}

        OpcodeClaim(OpcodeClaim&& other) noexcept
            : api_(other.api_), opcode_(other.opcode_), live_(other.Release())
        {
        }
        OpcodeClaim& operator=(OpcodeClaim&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                api_    = other.api_;
                opcode_ = other.opcode_;
                live_   = other.Release();
            }
            return *this;
        }

        OpcodeClaim(const OpcodeClaim&)            = delete;
        OpcodeClaim& operator=(const OpcodeClaim&) = delete;

        ~OpcodeClaim() { Reset(); }

        explicit operator bool() const { return live_; }
        uint32_t Opcode() const { return opcode_; }

        /// Gives up ownership without releasing the opcode. True when there was a claim.
        bool Release()
        {
            const bool was = live_;
            live_          = false;
            return was;
        }

        /// Gives the opcode back now. Safe to call more than once.
        void Reset()
        {
            if (!live_) return;
            if (api_) api_->release_opcode(opcode_);
            live_ = false;
        }

    private:
        const wxl_net_api* api_    = nullptr;
        uint32_t           opcode_ = 0;
        bool               live_   = false;
    };

    /**
     * @brief The typed facade over wxl.net.
     *
     * Every call answers WXL_E_UNSUPPORTED while the runtime does not publish the service.
     */
    class NetService : public detail::Facade<NetService, wxl_net_api>
    {
    public:
        static constexpr const char* kName    = WXL_NET_SERVICE;
        static constexpr uint32_t    kVersion = WXL_NET_SERVICE_VERSION;

        /// WXL_NET_OFFLINE without the service.
        wxl_net_state State() const
        {
            const wxl_net_api* api = Raw();
            return api ? static_cast<wxl_net_state>(api->state()) : WXL_NET_OFFLINE;
        }

        /// Sends a body under an opcode; the runtime frames it.
        wxl_result Send(uint32_t opcode, const void* data, uint32_t size) const
        {
            const wxl_net_api* api = Raw();
            if (!api) return WXL_E_UNSUPPORTED;
            return api->send(opcode, data, size);
        }

        /// Sends a trivially copyable struct as the body, byte for byte.
        template <class Body>
        wxl_result Send(uint32_t opcode, const Body& body) const
        {
            static_assert(std::is_trivially_copyable_v<Body>, "a wire body is plain data");
            return Send(opcode, &body, static_cast<uint32_t>(sizeof(Body)));
        }

        /// Claims an opcode the client does not handle. WXL_E_INVALID when another mod has it.
        Expected<OpcodeClaim> Claim(uint32_t opcode) const
        {
            const wxl_net_api* api = Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};
            const wxl_result result = api->claim_opcode(opcode);
            if (result != WXL_OK) return Unexpected{result};
            return OpcodeClaim(api, opcode);
        }

        /// The highest opcode the client itself uses. Anything above is free to claim.
        Expected<uint32_t> ReservedOpcodeLimit() const
        {
            const wxl_net_api* api = Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};
            return api->reserved_opcode_limit();
        }
    };
} // namespace wxl

#endif // WXL_SERVICE_NET_HPP
