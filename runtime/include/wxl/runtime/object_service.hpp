// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The object service, written once for every runtime.
//
// This is the pattern the other services follow. The skeleton owns everything that does not
// depend on how the client is reached: the handle table, staleness, the guid index, enumeration,
// type checking, argument validation, and the assembly of the C struct a mod receives. What is
// left for a runtime is ObjectBinding -- a handful of leaf reads against one native object.
//
//   core:   a leaf read is a load at a known offset in the loaded image
//   client: a leaf read is a member access or a method call on the real object
//
// That difference is irreducible. Everything above it is not, so it lives here.

#ifndef WXL_RUNTIME_OBJECT_SERVICE_HPP
#define WXL_RUNTIME_OBJECT_SERVICE_HPP

#include "wxl/runtime/handles.hpp"
#include "wxl/services/object.h"

namespace wxl::runtime
{
    /**
     * @brief The leaf reads a runtime has to supply.
     *
     * Each takes the native object the handle resolved to -- never a handle, never a null: the
     * skeleton has already checked both. A read that does not apply to this object answers false
     * and leaves @p out alone.
     */
    class ObjectBinding
    {
    public:
        virtual ~ObjectBinding() = default;

        virtual wxl_guid        Guid(void* object) const                    = 0;
        virtual wxl_object_type Type(void* object) const                    = 0;
        virtual bool            Position(void* object, wxl_vec3& out) const = 0;
        virtual bool            Facing(void* object, float& out) const      = 0;

        /**
         * @brief The display name.
         *
         * The binding owns the storage and keeps it valid until its next call, which is what the
         * service's own contract promises a mod.
         */
        virtual bool Name(void* object, const char*& out, uint32_t& size) const = 0;

        virtual bool  UnitHealth(void* object, uint32_t& out) const    = 0;
        virtual bool  UnitMaxHealth(void* object, uint32_t& out) const = 0;
        virtual bool  UnitLevel(void* object, uint32_t& out) const     = 0;
        virtual void* UnitTarget(void* object) const                   = 0;

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

        /// Drops everything, as when leaving the world.
        void Clear();

        /// The handle an object already has, for a runtime filling in an event's args.
        wxl_handle HandleOf(void* object) const { return handles_.HandleOf(object); }

        const HandleTable& Handles() const { return handles_; }

    private:
        static ObjectService* instance_;

        /// Resolves a handle, or nullptr. Every thunk starts here.
        static void* Object(wxl_handle handle);

        /// Resolves a handle that must be a unit or a player, or nullptr.
        static void* Unit(wxl_handle handle);

        static wxl_bool   WXL_CALL ValidThunk(wxl_handle object);
        static wxl_guid   WXL_CALL GuidThunk(wxl_handle object);
        static uint32_t   WXL_CALL TypeThunk(wxl_handle object);
        static wxl_handle WXL_CALL FromGuidThunk(wxl_guid guid);
        static wxl_handle WXL_CALL LocalPlayerThunk(void);

        static wxl_result WXL_CALL PositionThunk(wxl_handle object, wxl_vec3* out);
        static wxl_result WXL_CALL FacingThunk(wxl_handle object, float* out);
        static wxl_result WXL_CALL NameThunk(wxl_handle object, wxl_str* out);

        static wxl_result WXL_CALL UnitHealthThunk(wxl_handle unit, uint32_t* out);
        static wxl_result WXL_CALL UnitMaxHealthThunk(wxl_handle unit, uint32_t* out);
        static wxl_result WXL_CALL UnitLevelThunk(wxl_handle unit, uint32_t* out);
        static wxl_result WXL_CALL UnitTargetThunk(wxl_handle unit, wxl_handle* out);

        static uint32_t   WXL_CALL EnumerateThunk(uint32_t type, wxl_handle* out, uint32_t cap);
        static wxl_result WXL_CALL PositionsThunk(const wxl_handle* objects, uint32_t count,
                                                  wxl_vec3* out);

        ObjectBinding& binding_;
        HandleTable    handles_;
        wxl_object_api api_{};
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECT_SERVICE_HPP
