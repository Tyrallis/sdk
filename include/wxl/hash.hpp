// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The hash of a 64-bit key -- a handle, a guid -- for the hash tables of the runtimes and of mods.

#ifndef WXL_HASH_HPP
#define WXL_HASH_HPP

#include <cstdint>

#include <ankerl/unordered_dense.h>

namespace wxl
{
    /**
     * @brief Hashes a 64-bit key for ankerl::unordered_dense, which finds a bucket in the top bits
     *        of the hash and keeps a fingerprint of the bottom ones.
     *
     * On x64, unordered_dense's own: one 128-bit multiply and a fold. On x86 that multiply is four,
     * so the key is folded to 32 bits and mixed there (two multiplies), and the 32 bits stand for
     * both halves of the hash: the top for the bucket, the bottom for the fingerprint. A handle's
     * low half is its slot and its high half the slot's generation; a guid's high half is its
     * kind: both halves count.
     */
    struct Hash64
    {
        using is_avalanching = void;

        uint64_t operator()(uint64_t key) const noexcept
        {
#if UINTPTR_MAX > 0xFFFFFFFFu
            return ankerl::unordered_dense::detail::wyhash::hash(key);
#else
            uint32_t h = static_cast<uint32_t>(key) * 0x9E3779B1u ^ static_cast<uint32_t>(key >> 32);
            h ^= h >> 16;
            h *= 0x85EBCA6Bu;
            h ^= h >> 13;
            return (static_cast<uint64_t>(h) << 32) | h;
#endif
        }
    };
} // namespace wxl

#endif // WXL_HASH_HPP
