// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::Object: anything the client holds in the world, as a value over a handle.

#ifndef WXL_OBJECTS_OBJECT_HPP
#define WXL_OBJECTS_OBJECT_HPP

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "wxl/objects/facade.hpp"
#include "wxl/objects/fields.hpp"

namespace wxl
{
    /**
     * @brief Anything the client holds in the world.
     *
     * A value: copy it, compare it, keep it across frames, put it in a std::set or a
     * std::unordered_map. A handle whose subject is gone answers IsValid() with false, and every
     * read on it answers with WXL_E_NOT_FOUND, instead of reading whatever took its place.
     *
     * operator bool is IsValid(): `if (target)` asks whether the object is still there, which
     * costs one call. HasHandle() is the free question of whether there is a handle at all.
     *
     * The client's own kinds are the derived classes, one per wxl_object_type: Item, Container,
     * Unit, Player, GameObject, DynamicObject and Corpse. As<T>() views an Object as one of them
     * when its server type allows it; `wxl::Unit(object)` is the same view unchecked, for code that
     * already knows the kind.
     *
     * A handle names one visibility episode (wxl/services/object.h): an object that leaves view
     * and comes back is another Object with the same Guid(). Event handlers are handed their
     * objects as this class.
     */
    class Object
    {
    public:
        /// The server types this class stands for; WXL_OBJECT_UNKNOWN means every type.
        static constexpr wxl_object_type kKinds[] = {WXL_OBJECT_UNKNOWN};

        /// Whether an object of that server type -- a wxl_object_type, or the `type` an object
        /// event carries -- may be viewed as this class.
        static constexpr bool Accepts(uint32_t) { return true; }

        constexpr Object() = default;
        explicit constexpr Object(wxl_handle handle) : handle_(handle) {}

        constexpr wxl_handle Handle() const { return handle_; }
        constexpr bool       HasHandle() const { return handle_ != WXL_HANDLE_NONE; }

        /// Whether the handle still resolves. One call; false without the service.
        bool IsValid() const
        {
            const wxl_object_api* api = ObjectService::Raw();
            return api && HasHandle() && api->valid(handle_) == WXL_TRUE;
        }

        explicit operator bool() const { return IsValid(); }

        friend constexpr bool operator==(const Object& a, const Object& b) { return a.handle_ == b.handle_; }
        friend constexpr bool operator!=(const Object& a, const Object& b) { return a.handle_ != b.handle_; }
        friend constexpr bool operator<(const Object& a, const Object& b) { return a.handle_ < b.handle_; }
        friend constexpr bool operator>(const Object& a, const Object& b) { return a.handle_ > b.handle_; }
        friend constexpr bool operator<=(const Object& a, const Object& b) { return a.handle_ <= b.handle_; }
        friend constexpr bool operator>=(const Object& a, const Object& b) { return a.handle_ >= b.handle_; }

        /// The object with that server identity, or a null Object when it is not in view.
        static Object Resolve(wxl_guid guid)
        {
            const wxl_object_api* api = ObjectService::Raw();
            return Object(api && guid != WXL_GUID_NONE ? api->from_guid(guid) : WXL_HANDLE_NONE);
        }

        /// The server identity; WXL_GUID_NONE as a value means the server does not own it.
        Expected<wxl_guid> Guid() const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            const wxl_guid guid = api->guid(handle_);
            if (guid == WXL_GUID_NONE && api->valid(handle_) != WXL_TRUE)
                return Unexpected{WXL_E_NOT_FOUND};
            return guid;
        }

        Expected<wxl_object_type> Type() const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            const wxl_object_type type = static_cast<wxl_object_type>(api->type(handle_));
            if (type == WXL_OBJECT_UNKNOWN && api->valid(handle_) != WXL_TRUE)
                return Unexpected{WXL_E_NOT_FOUND};
            return type;
        }

        Expected<wxl_vec3> Position() const { return Read(&wxl_object_api::position); }

        /// Facing in radians.
        Expected<float> Facing() const { return Read(&wxl_object_api::facing); }

        /// The display name, copied out of the runtime's storage.
        Expected<std::string> Name() const
        {
            Expected<wxl_str> name = Read(&wxl_object_api::name);
            if (!name) return Unexpected{name.Error()};
            return std::string(View(*name));
        }

        /// The server's template id: a creature, item or game-object entry; 0 for a player.
        Expected<uint32_t> Entry() const { return Read(&wxl_object_api::entry); }

        Expected<float> Scale() const { return Read(&wxl_object_api::scale); }

        /**
         * @brief This object as a narrower class, when its server type allows it.
         *
         *     if (std::optional<wxl::Unit> unit = object.As<wxl::Unit>()) ...
         */
        template <class T>
        std::optional<T> As() const
        {
            const Expected<wxl_object_type> type = Type();
            if (!type || !T::Accepts(*type)) return std::nullopt;
            return T(handle_);
        }

        // --- the descriptor fields ------------------------------------------------------------
        //
        // Every field of update_fields.def, by the constants of wxl/objects/fields.hpp:
        //
        //     wxl::Expected<int32_t> health = unit.Field(wxl::field::Unit::Health);
        //     wxl::Expected<float>   arcane = unit.Field(wxl::field::Unit::Resistances, 6);
        //
        // A field of a kind the object is not -- a Unit field asked of a game object -- answers
        // WXL_E_INVALID, as does an element past the field's last. A runtime older than abi
        // 0.1.9 answers WXL_E_UNSUPPORTED.

        /// The size of the object's descriptor block in dwords (wxl_object_api::field_count).
        Expected<uint32_t> FieldCount() const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!detail::Carries(api, &wxl_object_api::field_count)) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            const uint32_t count = api->field_count(handle_);
            if (!count) return Unexpected{WXL_E_NOT_FOUND};
            return count;
        }

        /// One element of a field, as its kind of value.
        template <class T>
        Expected<T> Field(const field::Field<T>& which, uint32_t element = 0) const
        {
            if (element >= which.Elements()) return Unexpected{WXL_E_INVALID};
            uint32_t         dwords[2] = {0, 0};
            const wxl_result result    = ReadOf(which.kind, which.IndexOf(element), which.Dwords(), dwords);
            if (result != WXL_OK) return Unexpected{result};
            return field::Decode<T>(dwords);
        }

        /// Every element of a field, in order.
        template <class T>
        Expected<std::vector<T>> Fields(const field::Field<T>& which) const
        {
            std::vector<uint32_t> dwords(which.size);
            const wxl_result      result = ReadOf(which.kind, which.index, which.size, dwords.data());
            if (result != WXL_OK) return Unexpected{result};
            std::vector<T> out;
            out.reserve(which.Elements());
            for (uint32_t i = 0; i < which.Elements(); ++i)
                out.push_back(field::Decode<T>(dwords.data() + i * which.Dwords()));
            return out;
        }

        /**
         * @brief Writes one element of a field in the client (wxl_object_api::write_fields).
         *
         * The client holds the value until the next update of the server that carries the
         * field replaces it; nothing else is told of the write.
         */
        template <class T>
        wxl_result SetField(const field::Field<T>& which, const T& value, uint32_t element = 0) const
        {
            if (element >= which.Elements()) return WXL_E_INVALID;
            uint32_t dwords[2] = {0, 0};
            field::Encode(value, dwords);
            return WriteOf(which.kind, which.IndexOf(element), which.Dwords(), dwords);
        }

        /// Dwords [first, first + count) of the block as the client holds them, by index.
        Expected<std::vector<uint32_t>> RawFields(uint32_t first, uint32_t count) const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!detail::Carries(api, &wxl_object_api::read_fields)) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            std::vector<uint32_t> out(count);
            const wxl_result      result = api->read_fields(handle_, first, count, out.data());
            if (result != WXL_OK) return Unexpected{result};
            return out;
        }

        /// Writes dwords [first, first + count) of the block in the client, by index.
        wxl_result SetRawFields(uint32_t first, const uint32_t* values, uint32_t count) const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!detail::Carries(api, &wxl_object_api::write_fields)) return WXL_E_UNSUPPORTED;
            if (!HasHandle()) return WXL_E_NOT_FOUND;
            return api->write_fields(handle_, first, count, values, WXL_FIELD_WRITE_PLAIN);
        }

    protected:
        wxl_handle handle_ = WXL_HANDLE_NONE;

        /// A getter of the object service, as a member of the table.
        template <class V>
        using Getter = wxl_result(WXL_CALL*)(wxl_handle, V*);

        /// An indexed getter: a slot, a power, a bag position.
        template <class V>
        using GetterAt = wxl_result(WXL_CALL*)(wxl_handle, uint32_t, V*);

        /// One getter through the service, with the three absences told apart.
        template <class V>
        Expected<V> Read(Getter<V> wxl_object_api::*getter) const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!detail::Carries(api, getter)) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            V                out{};
            const wxl_result result = (api->*getter)(handle_, &out);
            if (result != WXL_OK) return Unexpected{result};
            return out;
        }

        /// Whether the object carries the fields of a kind, as WXL_OK, or why not.
        wxl_result CarriesKind(const wxl_object_api* api, wxl_object_type kind) const
        {
            const uint32_t type = api->type(handle_);
            if (type == WXL_OBJECT_UNKNOWN && api->valid(handle_) != WXL_TRUE) return WXL_E_NOT_FOUND;
            return field::Carries(type, kind) ? WXL_OK : WXL_E_INVALID;
        }

        /// Dwords of a field of a kind, checked to be the object's.
        wxl_result ReadOf(wxl_object_type kind, uint32_t first, uint32_t count, uint32_t* out) const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!detail::Carries(api, &wxl_object_api::read_fields)) return WXL_E_UNSUPPORTED;
            if (!HasHandle()) return WXL_E_NOT_FOUND;
            const wxl_result carried = CarriesKind(api, kind);
            if (carried != WXL_OK) return carried;
            return api->read_fields(handle_, first, count, out);
        }

        wxl_result WriteOf(wxl_object_type kind, uint32_t first, uint32_t count, const uint32_t* in) const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!detail::Carries(api, &wxl_object_api::write_fields)) return WXL_E_UNSUPPORTED;
            if (!HasHandle()) return WXL_E_NOT_FOUND;
            const wxl_result carried = CarriesKind(api, kind);
            if (carried != WXL_OK) return carried;
            return api->write_fields(handle_, first, count, in, WXL_FIELD_WRITE_PLAIN);
        }

        template <class V>
        Expected<V> ReadAt(GetterAt<V> wxl_object_api::*getter, uint32_t index) const
        {
            const wxl_object_api* api = ObjectService::Raw();
            if (!detail::Carries(api, getter)) return Unexpected{WXL_E_UNSUPPORTED};
            if (!HasHandle()) return Unexpected{WXL_E_NOT_FOUND};
            V                out{};
            const wxl_result result = (api->*getter)(handle_, index, &out);
            if (result != WXL_OK) return Unexpected{result};
            return out;
        }
    };

    // What a handler is handed must cost what the handle costs: one register, copied freely.
    static_assert(sizeof(Object) == sizeof(wxl_handle) && std::is_trivially_copyable_v<Object>,
                  "wxl::Object is a handle and nothing more");
} // namespace wxl

namespace std
{
    template <>
    struct hash<wxl::Object>
    {
        size_t operator()(const wxl::Object& object) const noexcept
        {
            return hash<wxl_handle>()(object.Handle());
        }
    };
} // namespace std

#endif // WXL_OBJECTS_OBJECT_HPP
