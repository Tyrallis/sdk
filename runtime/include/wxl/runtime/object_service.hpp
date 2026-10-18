// The shared half of wxl.object: handles, type gating, enumeration, over a runtime's leaf reads.
// Copyright (C) 2026 WarcraftXL
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef WXL_RUNTIME_OBJECT_SERVICE_HPP
#define WXL_RUNTIME_OBJECT_SERVICE_HPP

#include "wxl/runtime/handles.hpp"
#include "wxl/runtime/objects/container.hpp"
#include "wxl/runtime/objects/corpse.hpp"
#include "wxl/runtime/objects/dynamic_object.hpp"
#include "wxl/runtime/objects/game_object.hpp"
#include "wxl/runtime/objects/item.hpp"
#include "wxl/runtime/objects/object.hpp"
#include "wxl/runtime/objects/player.hpp"
#include "wxl/runtime/objects/unit.hpp"
#include "wxl/services/object.h"

namespace wxl::runtime
{
    namespace detail
    {
        /// One bit per wxl_object_type, for the kinds a getter accepts.
        constexpr uint32_t KindBit(wxl_object_type type) { return 1u << static_cast<uint32_t>(type); }
    } // namespace detail

    /**
     * @brief The leaf reads a runtime has to supply: one interface per kind of object, together.
     *
     * The per-kind interfaces live in wxl/runtime/objects/, one file each. The service asks a
     * kind's reads only for an object of that kind (a UnitReads method only ever sees a unit or a
     * player), so an implementation may cast without checking.
     */
    class ObjectBinding : public ObjectReads,
                          public UnitReads,
                          public PlayerReads,
                          public ItemReads,
                          public ContainerReads,
                          public GameObjectReads,
                          public DynamicObjectReads,
                          public CorpseReads
    {
    public:
        /// The object the local player controls, or nullptr while out of the world.
        virtual void* LocalPlayer() const = 0;
    };

    /**
     * @brief Serves wxl.object on top of a binding.
     *
     * One per process. The C struct a mod receives holds plain function pointers with nowhere to
     * carry a context, so the thunks reach the single live instance; a client has one object
     * manager, so this costs nothing real. Constructing a second one is a programming error and
     * is reported as such.
     *
     * Not thread-safe, like the handle table under it: the runtime drives it from the thread
     * that owns the client's objects, and a mod reads it from there.
     */
    class ObjectService
    {
    public:
        explicit ObjectService(ObjectBinding& binding);
        ~ObjectService();

        ObjectService(const ObjectService&)            = delete;
        ObjectService& operator=(const ObjectService&) = delete;

        /// The table to publish under WXL_OBJECT_SERVICE.
        const wxl_object_api* Api() const { return &api_; }

        // --- what the runtime drives ----------------------------------------------------------
        //
        // The runtime calls these from wherever its client creates and destroys objects. The
        // service does not go looking: it is told.

        /// Starts tracking an object and returns its handle.
        wxl_handle Track(void* object);

        /// Stops tracking it. Outstanding handles to it go stale from here on.
        void Untrack(void* object);

        /// The same, for a caller that has the handle already and need not be charged a lookup.
        void Untrack(wxl_handle handle);

        /// Drops everything, as when leaving the world.
        void Clear();

        /// The handle an object already has, for a runtime filling in an event's args.
        wxl_handle HandleOf(void* object) const { return handles_.HandleOf(object); }

        /// The handle of the tracked object carrying a guid, or WXL_HANDLE_NONE.
        wxl_handle FromGuid(wxl_guid guid) const;

        const HandleTable& Handles() const { return handles_; }

    private:
        static ObjectService* instance_;

        /// The kinds a getter accepts, one bit per wxl_object_type.
        static constexpr uint32_t kAny         = 0xFFu;
        static constexpr uint32_t kUnits       = detail::KindBit(WXL_OBJECT_UNIT) | detail::KindBit(WXL_OBJECT_PLAYER);
        static constexpr uint32_t kPlayers     = detail::KindBit(WXL_OBJECT_PLAYER);
        static constexpr uint32_t kItems       = detail::KindBit(WXL_OBJECT_ITEM) | detail::KindBit(WXL_OBJECT_CONTAINER);
        static constexpr uint32_t kContainers  = detail::KindBit(WXL_OBJECT_CONTAINER);
        static constexpr uint32_t kGameObjects = detail::KindBit(WXL_OBJECT_GAMEOBJECT);
        static constexpr uint32_t kDynamics    = detail::KindBit(WXL_OBJECT_DYNAMIC);
        static constexpr uint32_t kCorpses     = detail::KindBit(WXL_OBJECT_CORPSE);

        /// Resolves a handle, or nullptr. Every thunk starts here.
        static void* Object(wxl_handle handle);

        /// Whether a resolved object is of one of the kinds in a mask.
        static bool OfKind(void* native, uint32_t kinds);

        /**
         * @brief One getter of the table: resolves, gates on kind, reads through the binding.
         *
         * @tparam V      the out type the getter writes.
         * @tparam Read   the ObjectBinding member that supplies it, as bool (void*, V&) const.
         * @tparam Kinds  the kinds the getter accepts; any other live handle is WXL_E_INVALID.
         */
        template <class V, auto Read, uint32_t Kinds>
        static wxl_result WXL_CALL Get(wxl_handle handle, V* out);

        /// The same for an indexed read, as bool (void*, uint32_t, V&) const.
        template <class V, auto Read, uint32_t Kinds>
        static wxl_result WXL_CALL GetAt(wxl_handle handle, uint32_t index, V* out);

        static wxl_bool   WXL_CALL ValidThunk(wxl_handle object);
        static wxl_guid   WXL_CALL GuidThunk(wxl_handle object);
        static uint32_t   WXL_CALL TypeThunk(wxl_handle object);
        static wxl_handle WXL_CALL FromGuidThunk(wxl_guid guid);
        static wxl_handle WXL_CALL LocalPlayerThunk(void);
        static wxl_result WXL_CALL NameThunk(wxl_handle object, wxl_str* out);
        static wxl_result WXL_CALL UnitTargetThunk(wxl_handle unit, wxl_handle* out);
        static uint32_t   WXL_CALL EnumerateThunk(uint32_t type, wxl_handle* out, uint32_t cap);
        static wxl_result WXL_CALL PositionsThunk(const wxl_handle* objects, uint32_t count, wxl_vec3* out);

        ObjectBinding& binding_;
        HandleTable    handles_;
        wxl_object_api api_{};
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECT_SERVICE_HPP
