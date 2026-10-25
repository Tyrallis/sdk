// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// Generational handles over whatever a runtime holds.
//
// Shared by every runtime: how a handle is minted, retired and checked has nothing to do with
// whether the thing behind it is reached at an offset or by calling a method.

#ifndef WXL_RUNTIME_HANDLES_HPP
#define WXL_RUNTIME_HANDLES_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include <ankerl/unordered_dense.h>

#include "wxl/hash.hpp"

#include "wxl/abi.h"

namespace wxl::runtime
{
    /**
     * @brief Maps handles to the objects a runtime owns, and tells a stale one from a live one.
     *
     * A handle packs a slot index and the number of times that slot has been reused. Retiring an
     * object bumps its slot's generation, so a handle kept past its subject's death resolves to
     * nothing instead of to whatever moved in. That is the whole reason a mod may hold a handle
     * across frames.
     *
     * Slots are reused, so the table does not grow with churn. Slot 0 is never handed out, which
     * makes WXL_HANDLE_NONE naturally invalid.
     *
     * Not thread-safe: a client's object set changes on its own thread, and locking every read
     * would cost more than it buys. A runtime that needs another thread guards it.
     */
    class HandleTable
    {
    public:
        HandleTable();

        /**
         * @brief Mints a handle for an object, or returns the one it already has.
         *
         * Tracking the same pointer twice is the ordinary case -- an object seen again is the
         * same object -- so it answers with the existing handle rather than a second one.
         *
         * @param key  a name the owner can find the object by later, or 0 for none. The table
         *             does not read it or care what it means: an object service passes the
         *             guid, so that looking an object up by guid is a lookup and not a walk of
         *             everything tracked. A key given twice belongs to whichever object was
         *             tracked last, which is what reusing a name means.
         * @param tag  a value kept with the object and handed back on resolve: an object
         *             service keeps its kind there. Ignored when the object is tracked already.
         */
        wxl_handle Track(void* object, uint64_t key = 0, uint32_t tag = 0);

        /// Retires an object. Its handle stops resolving; later handles never collide with it.
        void Untrack(void* object);

        /// The same, for a caller that already has the handle and need not look it up again.
        void Untrack(wxl_handle handle);

        /// The handle tracked under a key, or WXL_HANDLE_NONE for 0 and for a key nobody holds.
        wxl_handle FindByKey(uint64_t key) const;

        /// Retires everything. Every outstanding handle stops resolving.
        void Clear();

        /// The object behind a handle, or nullptr when it no longer resolves; and, when @p tag is
        /// given and the handle resolves, the tag it was tracked with.
        void* Resolve(wxl_handle handle, uint32_t* tag = nullptr) const;

        /// Whether a handle still resolves.
        bool Valid(wxl_handle handle) const { return Resolve(handle) != nullptr; }

        /// The handle an object was given, or WXL_HANDLE_NONE when it is not tracked.
        wxl_handle HandleOf(void* object) const;

        /// How many objects are tracked.
        std::size_t Size() const { return live_; }

        /**
         * @brief Visits every live object.
         * @param visit  called as visit(wxl_handle, void* object, uint32_t tag); must not track
         *               or untrack.
         */
        template <class Visitor>
        void ForEach(Visitor&& visit) const
        {
            for (std::size_t index = 1; index < slots_.size(); ++index)
            {
                const Slot& slot = slots_[index];
                if (!slot.object) continue;
                visit(Pack(index, slot.generation), slot.object, slot.tag);
            }
        }

    private:
        struct Slot
        {
            void*    object     = nullptr;
            uint64_t key        = 0; ///< what Track was given, so retiring need not be told again
            uint32_t tag        = 0;
            uint32_t generation = 1; ///< bumped on retire, so it differs from every handle out; never 0
        };

        /// Empties a slot and bumps it past the handle handed out for it.
        void Retire(std::size_t index);

        static wxl_handle Pack(std::size_t index, uint32_t generation)
        {
            return (static_cast<wxl_handle>(generation) << 32) | static_cast<uint32_t>(index);
        }

        static std::size_t IndexOf(wxl_handle handle)
        {
            return static_cast<std::size_t>(handle & 0xFFFFFFFFull);
        }

        static uint32_t GenerationOf(wxl_handle handle)
        {
            return static_cast<uint32_t>(handle >> 32);
        }

        std::vector<Slot>                         slots_;
        std::vector<std::size_t>                  free_;
        // Open-addressed, values kept dense: an object event looks its handle up on every raise.
        struct PointerHash
        {
            using is_avalanching = void;
            uint64_t operator()(void* p) const noexcept { return Hash64()(reinterpret_cast<uintptr_t>(p)); }
        };
        ankerl::unordered_dense::map<void*, wxl_handle, PointerHash> byObject_;
        ankerl::unordered_dense::map<uint64_t, wxl_handle, Hash64> byKey_;
        std::size_t                               live_ = 0;
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_HANDLES_HPP
