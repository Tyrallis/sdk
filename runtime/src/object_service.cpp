// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The shared half of wxl.object: the thunks behind the table, over a runtime's leaf reads.

#include "wxl/runtime/object_service.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "wxl/objects/fields.hpp"

namespace wxl::runtime
{
    namespace
    {
        /// "a Unit", "an Item": a kind as a log line names it.
        const char* Article(const char* name)
        {
            return std::strchr("AEIO", name[0]) ? "an" : "a"; // a Unit, an Item, an Object
        }

        const char* KindName(uint32_t type)
        {
            const field::Kind* kind = field::KindOf(type);
            return kind ? kind->name : "Object";
        }

        /// "dword 0x95" or "dwords 0x90..0x97".
        void Range(char* text, size_t size, uint32_t first, uint32_t count)
        {
            if (count == 1) std::snprintf(text, size, "dword 0x%X", first);
            else std::snprintf(text, size, "dwords 0x%X..0x%X", first, first + count - 1);
        }
    } // namespace

    ObjectService* ObjectService::instance_ = nullptr;

    wxl_result ObjectService::Gate(wxl_handle handle, uint32_t kinds, void*& native)
    {
        uint32_t kind = 0;
        native        = Object(handle, &kind);
        if (!native) return WXL_E_NOT_FOUND;
        return kind < 32u && (kinds & (1u << kind)) != 0 ? WXL_OK : WXL_E_INVALID;
    }

    template <class V, auto Read, uint32_t Kinds>
    wxl_result WXL_CALL ObjectService::Get(wxl_handle handle, V* out)
    {
        if (!out) return WXL_E_INVALID;

        void*            native = nullptr;
        const wxl_result gated  = Gate(handle, Kinds, native);
        if (gated != WXL_OK) return gated;
        return (instance_->binding_.*Read)(native, *out) ? WXL_OK : WXL_E_INVALID;
    }

    template <class V, auto Read, uint32_t Kinds>
    wxl_result WXL_CALL ObjectService::GetAt(wxl_handle handle, uint32_t index, V* out)
    {
        if (!out) return WXL_E_INVALID;

        void*            native = nullptr;
        const wxl_result gated  = Gate(handle, Kinds, native);
        if (gated != WXL_OK) return gated;
        return (instance_->binding_.*Read)(native, index, *out) ? WXL_OK : WXL_E_INVALID;
    }

    ObjectService::ObjectService(ObjectBinding& binding) : binding_(binding)
    {
        instance_ = this;

        api_.struct_size = sizeof(api_);

        api_.valid        = &ObjectService::ValidThunk;
        api_.guid         = &ObjectService::GuidThunk;
        api_.type         = &ObjectService::TypeThunk;
        api_.from_guid    = &ObjectService::FromGuidThunk;
        api_.local_player = &ObjectService::LocalPlayerThunk;

        api_.name        = &ObjectService::NameThunk;
        api_.unit_target = &ObjectService::UnitTargetThunk;
        api_.enumerate   = &ObjectService::EnumerateThunk;
        api_.positions   = &ObjectService::PositionsThunk;

        api_.field_count     = &ObjectService::FieldCountThunk;
        api_.read_fields     = &ObjectService::ReadFieldsThunk;
        api_.read_field_many = &ObjectService::ReadFieldManyThunk;
        api_.write_fields    = &ObjectService::WriteFieldsThunk;
        api_.watch_field     = &ObjectService::WatchFieldThunk;

#define WXL_OBJECT_READ(kinds, member, Method, type) \
    api_.member = &Get<type, &ObjectBinding::Method, k##kinds>;
#define WXL_OBJECT_READ_AT(kinds, member, Method, type) \
    api_.member = &GetAt<type, &ObjectBinding::Method, k##kinds>;
#include "wxl/runtime/object_reads.def"
#undef WXL_OBJECT_READ
#undef WXL_OBJECT_READ_AT
    }

    ObjectService::~ObjectService()
    {
        if (instance_ == this) instance_ = nullptr;
    }

    wxl_handle ObjectService::Track(void* object)
    {
        if (!object) return WXL_HANDLE_NONE;
        if (const wxl_handle known = handles_.HandleOf(object)) return known;

        // The guid and the kind are read once, here, and kept with the handle: the guid so that
        // FromGuid is a lookup and not a walk of a set thousands deep in a city, the kind so that
        // no getter has to ask the client for it again.
        return handles_.Track(object, binding_.Guid(object),
                              static_cast<uint32_t>(binding_.Type(object)));
    }

    void ObjectService::Untrack(void* object) { handles_.Untrack(object); }

    void ObjectService::Untrack(wxl_handle handle) { handles_.Untrack(handle); }

    void ObjectService::Clear() { handles_.Clear(); }

    wxl_handle ObjectService::FromGuid(wxl_guid guid)
    {
        if (guid == WXL_GUID_NONE) return WXL_HANDLE_NONE;
        if (const wxl_handle known = handles_.FindByKey(guid)) return known;

        void* object = binding_.FindByGuid(guid);
        return object ? Track(object) : WXL_HANDLE_NONE;
    }

    void* ObjectService::Object(wxl_handle handle, uint32_t* kind)
    {
        return instance_ ? instance_->handles_.Resolve(handle, kind) : nullptr;
    }

    wxl_bool WXL_CALL ObjectService::ValidThunk(wxl_handle object)
    {
        return Object(object) ? WXL_TRUE : WXL_FALSE;
    }

    wxl_guid WXL_CALL ObjectService::GuidThunk(wxl_handle object)
    {
        void* native = Object(object);
        return native ? instance_->binding_.Guid(native) : WXL_GUID_NONE;
    }

    uint32_t WXL_CALL ObjectService::TypeThunk(wxl_handle object)
    {
        uint32_t kind = WXL_OBJECT_UNKNOWN;
        return Object(object, &kind) ? kind : static_cast<uint32_t>(WXL_OBJECT_UNKNOWN);
    }

    wxl_handle WXL_CALL ObjectService::FromGuidThunk(wxl_guid guid)
    {
        return instance_ ? instance_->FromGuid(guid) : WXL_HANDLE_NONE;
    }

    wxl_handle WXL_CALL ObjectService::LocalPlayerThunk(void)
    {
        if (!instance_) return WXL_HANDLE_NONE;

        // Tracked on demand like FromGuid: the player is in view whenever the client has one.
        void* player = instance_->binding_.LocalPlayer();
        return player ? instance_->Track(player) : WXL_HANDLE_NONE;
    }

    wxl_result WXL_CALL ObjectService::NameThunk(wxl_handle object, wxl_str* out)
    {
        if (!out) return WXL_E_INVALID;

        void* native = Object(object);
        if (!native) return WXL_E_NOT_FOUND;

        const char* text = nullptr;
        uint32_t    size = 0;
        if (!instance_->binding_.Name(native, text, size)) return WXL_E_INVALID;

        out->data = text;
        out->size = size;
        return WXL_OK;
    }

    wxl_result WXL_CALL ObjectService::UnitTargetThunk(wxl_handle unit, wxl_handle* out)
    {
        if (!out) return WXL_E_INVALID;

        void*            native = nullptr;
        const wxl_result gated  = Gate(unit, kUnits, native);
        if (gated != WXL_OK) return gated;

        wxl_guid target = WXL_GUID_NONE;
        if (!instance_->binding_.UnitTarget(native, target)) return WXL_E_INVALID;
        *out = instance_->FromGuid(target);
        return WXL_OK;
    }

    uint32_t WXL_CALL ObjectService::EnumerateThunk(uint32_t type, wxl_handle* out, uint32_t cap)
    {
        if (!instance_) return 0;

        uint32_t matched = 0;
        instance_->handles_.ForEach(
            [&](wxl_handle handle, void*, uint32_t kind)
            {
                if (type != WXL_OBJECT_UNKNOWN && kind != type) return;

                if (out && matched < cap) out[matched] = handle;
                ++matched;
            });
        return matched;
    }

    wxl_result WXL_CALL ObjectService::PositionsThunk(const wxl_handle* objects, uint32_t count,
                                                      wxl_vec3* out)
    {
        if (!objects || !out) return WXL_E_INVALID;
        if (!instance_) return WXL_E_UNSUPPORTED;

        for (uint32_t i = 0; i < count; ++i)
        {
            out[i] = wxl_vec3{0.0f, 0.0f, 0.0f};

            void* native = Object(objects[i]);
            if (native) instance_->binding_.Position(native, out[i]);
        }
        return WXL_OK;
    }

    // --- the descriptor fields ---------------------------------------------------------------

    void ObjectService::Refuse(const char* format, ...) const
    {
        char    line[256];
        va_list args;
        va_start(args, format);
        std::vsnprintf(line, sizeof line, format, args);
        va_end(args);
        binding_.Report(line);
    }

    wxl_result ObjectService::Block(void* native, uint32_t kind, uint32_t*& dwords, uint32_t& end) const
    {
        dwords = binding_.Descriptor(native);
        if (!dwords) return WXL_E_UNSUPPORTED;
        // A player other than the one the client controls has no fields past PLAYER_FIELD_PAD_0.
        end = field::BlockEnd(kind, kind != WXL_OBJECT_PLAYER || binding_.LocalPlayer() == native);
        return end ? WXL_OK : WXL_E_UNSUPPORTED;
    }

    wxl_result ObjectService::InBlock(const char* call, void* native, uint32_t kind, uint32_t first,
                                      uint32_t count, uint32_t*& dwords, bool report) const
    {
        uint32_t         end    = 0;
        const wxl_result result = Block(native, kind, dwords, end);
        if (result != WXL_OK) return result;
        if (count == 0)
        {
            if (report) Refuse("%s: a count of 0 dwords reads nothing; ask for at least one", call);
            return WXL_E_INVALID;
        }
        if (first < end && count <= end - first) return WXL_OK;
        if (report)
        {
            char range[48];
            Range(range, sizeof range, first, count);
            const char* name = KindName(kind);
            if (kind == WXL_OBJECT_PLAYER && end != field::BlockEnd(kind, true))
                Refuse("%s: %s %s past %s %s's 0x%X: a player other than the active one has no "
                       "fields past PLAYER_FIELD_PAD_0", call, range, count == 1 ? "is" : "run",
                       Article(name), name, end);
            else
                Refuse("%s: %s %s past %s %s's 0x%X", call, range, count == 1 ? "is" : "run",
                       Article(name), name, end);
        }
        return WXL_E_INVALID;
    }

    uint32_t WXL_CALL ObjectService::FieldCountThunk(wxl_handle object)
    {
        uint32_t kind   = WXL_OBJECT_UNKNOWN;
        void*    native = Object(object, &kind);
        if (!native) return 0;
        uint32_t* dwords = nullptr;
        uint32_t  end    = 0;
        return instance_->Block(native, kind, dwords, end) == WXL_OK ? end : 0;
    }

    wxl_result WXL_CALL ObjectService::ReadFieldsThunk(wxl_handle object, uint32_t first,
                                                       uint32_t count, uint32_t* out)
    {
        if (!instance_) return WXL_E_UNSUPPORTED;
        if (!out)
        {
            instance_->Refuse("read_fields: out is null");
            return WXL_E_INVALID;
        }
        uint32_t kind   = WXL_OBJECT_UNKNOWN;
        void*    native = Object(object, &kind);
        if (!native) return WXL_E_NOT_FOUND;
        uint32_t*        dwords = nullptr;
        const wxl_result result = instance_->InBlock("read_fields", native, kind, first, count, dwords, true);
        if (result != WXL_OK) return result;
        std::memcpy(out, dwords + first, size_t(count) * sizeof(uint32_t));
        return WXL_OK;
    }

    wxl_result WXL_CALL ObjectService::ReadFieldManyThunk(const wxl_handle* objects, uint32_t count,
                                                          uint32_t field, uint32_t dwords,
                                                          uint32_t* out, wxl_result* results)
    {
        if (!instance_) return WXL_E_UNSUPPORTED;
        if ((!objects || !out) && count)
        {
            instance_->Refuse("read_field_many: %s is null", objects ? "out" : "objects");
            return WXL_E_INVALID;
        }
        if (dwords == 0)
        {
            instance_->Refuse("read_field_many: a count of 0 dwords reads nothing; ask for at least one");
            return WXL_E_INVALID;
        }
        bool reported = false;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t* const slot   = out + size_t(i) * dwords;
            uint32_t        kind   = WXL_OBJECT_UNKNOWN;
            void*           native = Object(objects[i], &kind);
            wxl_result      result = WXL_E_NOT_FOUND;
            uint32_t*       block  = nullptr;
            // One line for the whole call: a pass over a city would otherwise log per object.
            if (native) result = instance_->InBlock("read_field_many", native, kind, field, dwords, block, !reported);
            if (result == WXL_E_INVALID) reported = true;
            if (result == WXL_OK) std::memcpy(slot, block + field, size_t(dwords) * sizeof(uint32_t));
            else std::memset(slot, 0, size_t(dwords) * sizeof(uint32_t));
            if (results) results[i] = result;
        }
        return WXL_OK;
    }

    wxl_result WXL_CALL ObjectService::WriteFieldsThunk(wxl_handle object, uint32_t first,
                                                        uint32_t count, const uint32_t* in,
                                                        uint32_t flags)
    {
        if (!instance_) return WXL_E_UNSUPPORTED;
        if (!in)
        {
            instance_->Refuse("write_fields: in is null");
            return WXL_E_INVALID;
        }
        if (flags != WXL_FIELD_WRITE_PLAIN)
        {
            instance_->Refuse("write_fields: flags 0x%X are not known to this runtime; 0 is a plain write", flags);
            return WXL_E_INVALID;
        }
        uint32_t kind   = WXL_OBJECT_UNKNOWN;
        void*    native = Object(object, &kind);
        if (!native) return WXL_E_NOT_FOUND;
        uint32_t*        dwords = nullptr;
        const wxl_result result = instance_->InBlock("write_fields", native, kind, first, count, dwords, true);
        if (result != WXL_OK) return result;
        // OBJECT_FIELD_GUID and OBJECT_FIELD_TYPE: what the client hashes and casts the object by.
        if (first < 3)
        {
            instance_->Refuse("write_fields: dwords 0x0..0x2 (OBJECT_FIELD_GUID, OBJECT_FIELD_TYPE) "
                              "are the keys the client finds the object by; they cannot be written");
            return WXL_E_INVALID;
        }
        std::memcpy(dwords + first, in, size_t(count) * sizeof(uint32_t));
        return WXL_OK;
    }

    wxl_result WXL_CALL ObjectService::WatchFieldThunk(uint32_t type, uint32_t first, uint32_t count)
    {
        return instance_ ? instance_->Watch(type, first, count) : WXL_E_UNSUPPORTED;
    }

    wxl_result ObjectService::Watch(uint32_t type, uint32_t first, uint32_t count)
    {
        const field::Kind* kind = field::KindOf(type);
        if (!kind)
        {
            Refuse("watch_field: %u is not an object type (wxl_object_type)", type);
            return WXL_E_INVALID;
        }
        if (count == 0)
        {
            Refuse("watch_field: a count of 0 dwords watches nothing; ask for at least one");
            return WXL_E_INVALID;
        }
        const uint32_t end = kind->end;
        if (first >= end || count > end - first)
        {
            char range[48];
            Range(range, sizeof range, first, count);
            Refuse("watch_field: %s %s past %s %s's 0x%X", range, count == 1 ? "is" : "run",
                   Article(kind->name), kind->name, end);
            return WXL_E_INVALID;
        }

        // The elements the range covers, checked whole before anything is watched.
        struct Element
        {
            const field::Kind* owner;
            uint32_t           index;
            uint32_t           dwords;
            bool               local;
        };
        std::vector<Element> elements;
        for (uint32_t index = first; index < first + count;)
        {
            const field::Row* row = field::RowAt(type, index);
            if (!row) return WXL_E_INVALID; // the table covers every dword of a block
            const uint32_t size  = row->Dwords();
            const uint32_t start = row->index + (index - row->index) / size * size;
            if (start != index)
            {
                Refuse("watch_field: dword 0x%X is the second half of an element of %s (from 0x%X); "
                       "start the range at 0x%X", index, row->name, start, start);
                return WXL_E_INVALID;
            }
            if (index + size > first + count)
            {
                Refuse("watch_field: the range ends inside an element of %s (0x%X..0x%X); end it "
                       "at 0x%X", row->name, index, index + size - 1, index + size);
                return WXL_E_INVALID;
            }
            const field::Kind* owner = field::OwnerOf(type, index);
            field::Watch       scope = field::WatchOf(owner->type, index);
            if (size == 2 && field::WatchOf(owner->type, index + 1) != scope) scope = field::Watch::none;
            if (scope == field::Watch::none)
            {
                if (row->Elements() > 1)
                    Refuse("watch_field: element 0x%X of %s is not one the client sees change: it keeps "
                           "no copy of it from before an update", index, row->name);
                else
                    Refuse("watch_field: %s (dword 0x%X) is not one the client sees change: it keeps "
                           "no copy of it from before an update", row->name, index);
                return WXL_E_UNSUPPORTED;
            }
            elements.push_back(Element{owner, index, size, scope == field::Watch::local});
            index += size;
        }

        for (const Element& element : elements)
        {
            std::vector<uint16_t>& slots = watches_[element.owner->type];
            if (slots.size() < element.owner->end) slots.resize(element.owner->end, 0);
            uint16_t& slot = slots[element.index];
            if (!(slot & kRegistered))
            {
                if (!binding_.WatchElement(element.owner->type, element.index, element.dwords, element.local))
                {
                    Refuse("watch_field: this runtime cannot watch fields");
                    return WXL_E_UNSUPPORTED;
                }
                slot |= kRegistered;
            }
            slot |= uint16_t(1u << type);
        }
        return WXL_OK;
    }

    bool ObjectService::Watched(wxl_object_type kind, uint32_t index, uint32_t type) const
    {
        const uint32_t k = static_cast<uint32_t>(kind);
        if (k > WXL_OBJECT_CORPSE || index >= watches_[k].size() || type > WXL_OBJECT_CORPSE) return false;
        // A watch for a type covers the types under it: one asked for every object reaches all.
        const uint16_t slot = watches_[k][index];
        for (const field::Kind* at = field::KindOf(type); at;
             at = at->type == at->parent ? nullptr : field::KindOf(at->parent))
            if (slot & (1u << static_cast<uint32_t>(at->type))) return true;
        return false;
    }

    bool ObjectService::FieldChange(wxl_guid guid, wxl_object_type kind, uint32_t index,
                                    uint32_t dwords, const void* old_value, wxl_object_field_args& out)
    {
        if (dwords == 0 || dwords > 2 || !old_value) return false;
        const wxl_handle handle = FromGuid(guid);
        uint32_t         type   = WXL_OBJECT_UNKNOWN;
        void*            native = handles_.Resolve(handle, &type);
        if (!native || !Watched(kind, index, type)) return false;
        uint32_t*        block  = nullptr;
        const wxl_result result = InBlock("ObjectFieldChanged", native, type, index, dwords, block, false);
        if (result != WXL_OK) return false;

        out        = wxl_object_field_args{};
        out.object = handle;
        out.type   = type;
        out.field  = index;
        out.count  = dwords;
        std::memcpy(&out.old_value, old_value, size_t(dwords) * sizeof(uint32_t));
        std::memcpy(&out.new_value, block + index, size_t(dwords) * sizeof(uint32_t));
        return true;
    }
} // namespace wxl::runtime
