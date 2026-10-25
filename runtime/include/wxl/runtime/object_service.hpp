// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The shared half of wxl.object: handles, type gating, enumeration, over a runtime's leaf reads.

#ifndef WXL_RUNTIME_OBJECT_SERVICE_HPP
#define WXL_RUNTIME_OBJECT_SERVICE_HPP

#include <cstdint>
#include <vector>

#include "wxl/abi.h"
#include "wxl/events.h"
#include "wxl/runtime/handles.hpp"
#include "wxl/services/object.h"

namespace wxl::runtime
{
    namespace detail
    {
        /// One bit per wxl_object_type, for the kinds a getter accepts.
        constexpr uint32_t KindBit(wxl_object_type type) { return 1u << static_cast<uint32_t>(type); }
    } // namespace detail

    /**
     * @brief The leaf reads a runtime supplies: the five below, and one per row of
     *        object_reads.def.
     *
     * Each read takes the native object a handle resolved to, never a handle and never a null,
     * and is only asked about an object of a kind its row accepts, so an implementation may cast
     * without checking. A read that does not apply answers false and leaves @p out alone.
     */
    class ObjectBinding
    {
    public:
        virtual ~ObjectBinding() = default;

        virtual wxl_guid        Guid(void* object) const = 0;
        virtual wxl_object_type Type(void* object) const = 0;

        /// The display name. The binding owns the storage and keeps it valid until its next
        /// call, which is what the service promises a mod.
        virtual bool Name(void* object, const char*& out, uint32_t& size) const = 0;

        /// The guid a unit targets; WXL_GUID_NONE for nothing. The service resolves it.
        virtual bool UnitTarget(void* object, wxl_guid& out) const = 0;

        /// The object the local player controls, or nullptr while out of the world.
        virtual void* LocalPlayer() const = 0;

        /// The object the client holds under a guid, asked of the client itself rather than of
        /// what the runtime tracks; nullptr when it holds none, or when the runtime cannot ask.
        virtual void* FindByGuid(wxl_guid guid) const
        {
            (void)guid;
            return nullptr;
        }

        // --- the descriptor fields --------------------------------------------------------------

        /// The object's descriptor block, its dwords as the client holds them (CGObject_C's
        /// m_obj); nullptr when the runtime cannot reach them, which makes the field calls
        /// answer WXL_E_UNSUPPORTED.
        virtual uint32_t* Descriptor(void* object) const
        {
            (void)object;
            return nullptr;
        }

        /**
         * @brief Asks the client to report the changes of one element: dwords [first, first +
         *        dwords) of the fields of `kind`, on every object carrying them, or on the
         *        active player only when `local`.
         *
         * The service asks once per element, when a first watch covers it. The runtime keeps
         * asking the client until UnwatchElement -- again whenever the client drops its
         * handlers or the active player comes back -- and hands each report to
         * ObjectService::FieldChange. False when it cannot.
         */
        virtual bool WatchElement(wxl_object_type kind, uint32_t first, uint32_t dwords, bool local) const
        {
            (void)kind, (void)first, (void)dwords, (void)local;
            return false;
        }

        /**
         * @brief Stops asking the client about an element WatchElement asked for, once no watch
         *        covers it any more.
         *
         * False when the runtime cannot take its handler back: the client goes on reporting the
         * element, the service drops the reports, and a later watch of it does not ask again.
         */
        virtual bool UnwatchElement(wxl_object_type kind, uint32_t first, uint32_t dwords, bool local) const
        {
            (void)kind, (void)first, (void)dwords, (void)local;
            return false;
        }

        /// Why a field call was refused, one line for the runtime's log.
        virtual void Report(const char* line) const { (void)line; }

        /// How one unit regards another (wxl_object_api::unit_reaction). A binding that cannot
        /// tell answers false.
        virtual bool UnitReaction(void* unit, void* other, uint32_t& out) const
        {
            (void)unit;
            (void)other;
            (void)out;
            return false;
        }

#define WXL_OBJECT_READ(kinds, member, Method, type) \
    virtual bool Method(void* object, type& out) const = 0;
#define WXL_OBJECT_READ_AT(kinds, member, Method, type) \
    virtual bool Method(void* object, uint32_t index, type& out) const = 0;
#include "wxl/runtime/object_reads.def"
#undef WXL_OBJECT_READ
#undef WXL_OBJECT_READ_AT
    };

    /**
     * @brief Serves wxl.object on top of a binding.
     *
     * One per process. The C struct a mod receives holds plain function pointers with nowhere to
     * carry a context, so the thunks reach the single live instance; a client has one object
     * manager, so this costs nothing real.
     *
     * An object's kind is read once, when it is tracked, and kept with its handle: the client
     * never changes it, and every getter checks it.
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

        /**
         * @brief The handle of the object carrying a guid, or WXL_HANDLE_NONE.
         *
         * The tracked objects first. A guid the client holds but no seam announced -- an object
         * the client brought back by a path the runtime does not see -- is looked up through the
         * binding and tracked from here on, so a guid in view always resolves. Tracking it this
         * way raises nothing: the service has no host to raise through.
         */
        wxl_handle FromGuid(wxl_guid guid);

        const HandleTable& Handles() const { return handles_; }

        /**
         * @brief What the client reported of a watched element, as the event to raise.
         *
         * The runtime calls it from the client's mirror handler: the object's guid, the kind
         * whose fields hold the element, its first dword and size, and the copy of it the
         * client kept from before the update. Fills @p out and answers true when a watch asked
         * for that element on the object's type; false otherwise, and nothing is raised.
         */
        bool FieldChange(wxl_guid guid, wxl_object_type kind, uint32_t index, uint32_t dwords,
                         const void* old_value, wxl_object_field_args& out);

        /// Whether a watch asked for dword `index` of `kind` on objects of `type`.
        bool Watched(wxl_object_type kind, uint32_t index, uint32_t type) const;

        // --- who watches -----------------------------------------------------------------------
        //
        // A watch belongs to the mod that asked for it: the runtime says who is calling, and
        // delivers each ObjectFieldChanged to the owners Delivers() accepts.

        /// Answers the token of the mod whose code is running, 0 for nobody (the host's
        /// ModHost::CurrentMod).
        using CallerFn = uint32_t (*)(void* user);

        /// Who calls watch_field and unwatch_field. Without it every watch belongs to 0.
        void SetCaller(CallerFn caller, void* user)
        {
            caller_     = caller;
            callerUser_ = user;
        }

        /// Whether `owner` watches the element an event carries, on that object's type.
        bool Delivers(uint32_t owner, const wxl_object_field_args& args) const;

        /// Gives back every watch of an owner: the mod is disabled or unloaded.
        void DropWatches(uint32_t owner);

        /// How many watches an owner holds: one per element of each watch_field call.
        size_t WatchCount(uint32_t owner) const;

    private:
        static ObjectService* instance_;

        /// The kinds a getter accepts, one bit per wxl_object_type; the `kinds` column of
        /// object_reads.def names one of these.
        static constexpr uint32_t kAny         = 0xFFu;
        static constexpr uint32_t kUnits       = detail::KindBit(WXL_OBJECT_UNIT) | detail::KindBit(WXL_OBJECT_PLAYER);
        static constexpr uint32_t kPlayers     = detail::KindBit(WXL_OBJECT_PLAYER);
        static constexpr uint32_t kItems       = detail::KindBit(WXL_OBJECT_ITEM) | detail::KindBit(WXL_OBJECT_CONTAINER);
        static constexpr uint32_t kContainers  = detail::KindBit(WXL_OBJECT_CONTAINER);
        static constexpr uint32_t kGameObjects = detail::KindBit(WXL_OBJECT_GAMEOBJECT);
        static constexpr uint32_t kDynamics    = detail::KindBit(WXL_OBJECT_DYNAMIC);
        static constexpr uint32_t kCorpses     = detail::KindBit(WXL_OBJECT_CORPSE);

        /// Resolves a handle to its object, or nullptr; and, when asked, its kind.
        static void* Object(wxl_handle handle, uint32_t* kind = nullptr);

        /// Resolves a handle whose kind is in a mask. WXL_E_NOT_FOUND for a handle that does not
        /// resolve, WXL_E_INVALID for one of another kind.
        static wxl_result Gate(wxl_handle handle, uint32_t kinds, void*& native);

        /**
         * @brief One getter of the table: resolves, gates on kind, reads through the binding.
         *
         * @tparam V      the out type the getter writes.
         * @tparam Read   the ObjectBinding member that supplies it, as bool (void*, V&) const.
         * @tparam Kinds  the kinds the getter accepts.
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
        static wxl_result WXL_CALL UnitReactionThunk(wxl_handle unit, wxl_handle other, uint32_t* out);
        static uint32_t   WXL_CALL EnumerateThunk(uint32_t type, wxl_handle* out, uint32_t cap);
        static wxl_result WXL_CALL PositionsThunk(const wxl_handle* objects, uint32_t count, wxl_vec3* out);

        static uint32_t   WXL_CALL FieldCountThunk(wxl_handle object);
        static wxl_result WXL_CALL ReadFieldsThunk(wxl_handle object, uint32_t first, uint32_t count, uint32_t* out);
        static wxl_result WXL_CALL ReadFieldManyThunk(const wxl_handle* objects, uint32_t count, uint32_t field,
                                                      uint32_t dwords, uint32_t* out, wxl_result* results);
        static wxl_result WXL_CALL WriteFieldsThunk(wxl_handle object, uint32_t first, uint32_t count,
                                                    const uint32_t* in, uint32_t flags);
        static wxl_result WXL_CALL WatchFieldThunk(uint32_t type, uint32_t first, uint32_t count);
        static wxl_result WXL_CALL UnwatchFieldThunk(uint32_t type, uint32_t first, uint32_t count);

        /// The block of a resolved object and its end, or WXL_E_UNSUPPORTED.
        wxl_result Block(void* native, uint32_t kind, uint32_t*& dwords, uint32_t& end) const;

        /// Checks [first, first + count) against an object's block, naming what is wrong.
        wxl_result InBlock(const char* call, void* native, uint32_t kind, uint32_t first,
                           uint32_t count, uint32_t*& dwords, bool report) const;

        /// One element a range covers: the kind whose fields hold it, its first dword, its size.
        struct Element
        {
            wxl_object_type kind;
            uint32_t        index;
            uint32_t        dwords;
            bool            local;
        };

        /// The elements of [first, first + count) on objects of `type`, checked whole; logs and
        /// answers why not. `watchable` also refuses an element the client does not see change.
        wxl_result Elements(const char* call, uint32_t type, uint32_t first, uint32_t count,
                            bool watchable, std::vector<Element>& out) const;

        wxl_result Watch(uint32_t type, uint32_t first, uint32_t count);
        wxl_result Unwatch(uint32_t type, uint32_t first, uint32_t count);

        uint32_t Caller() const { return caller_ ? caller_(callerUser_) : 0; }

        /// One element less watched; the binding lets it go with the last watch.
        void Release(const Element& element);

        /// Recomputes the types an element is watched for, from the watches.
        void Refresh(wxl_object_type kind, uint32_t index);

        /// One line for the log, through the binding.
        void Refuse(const char* format, ...) const;

        /// One watch: an owner, the type it asked for, and the element.
        struct WatchEntry
        {
            uint32_t owner;
            uint32_t type;
            Element  element;
        };
        std::vector<WatchEntry> watchList_;

        /// Per kind (by wxl_object_type), per dword of the whole block: the types some watch
        /// asks for, one bit each, whether the binding watches the element, and how
        /// many watches cover it.
        struct Slot
        {
            uint16_t types     = 0;
            bool     installed = false;
            uint32_t watchers  = 0;
        };
        std::vector<Slot> watches_[WXL_OBJECT_CORPSE + 1];

        CallerFn caller_     = nullptr;
        void*    callerUser_ = nullptr;

        ObjectBinding& binding_;
        HandleTable    handles_;
        wxl_object_api api_{};
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECT_SERVICE_HPP
