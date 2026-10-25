// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// wxl::ObjectMap: a mod's own data about objects, dropped when they leave.

#ifndef WXL_OBJECTS_MAP_HPP
#define WXL_OBJECTS_MAP_HPP

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>

#include "wxl/objects/object.hpp"
#include "wxl/sdk.hpp"

namespace wxl
{
    /**
     * @brief Data a mod keeps per object, erased when the object leaves view.
     *
     *     wxl::ObjectMap<uint32_t> hits;
     *     void OnUnitDeath(wxl::Object unit) override { ++hits[unit]; }
     *
     * Keyed by handle, so an entry lasts one visibility episode: when the runtime raises
     * ObjectDestroy for an object, its entry goes, after every handler of default priority has
     * run -- an OnObjectDestroy of the mod still finds the entry there. An object that comes back
     * is a new key; key a std::unordered_map by Guid() for what must outlast that.
     *
     * The map subscribes to ObjectDestroy on its first insertion, so one built before the mod is
     * loaded (a member, a global) works. It holds that subscription for its whole life, which is
     * why it is neither copied nor moved: the runtime holds its address. Without the event (a
     * runtime that does not raise it), nothing is erased by itself; Prune() drops what no longer
     * resolves.
     *
     * Main thread only, like the objects themselves.
     */
    template <class T>
    class ObjectMap
    {
    public:
        using map_type       = std::unordered_map<Object, T>;
        using iterator       = typename map_type::iterator;
        using const_iterator = typename map_type::const_iterator;

        /// Below every handler a mod declares in the ordinary way, so they see the entry first.
        static constexpr int32_t kPriority = INT32_MAX;

        ObjectMap() = default;

        ObjectMap(const ObjectMap&)            = delete;
        ObjectMap& operator=(const ObjectMap&) = delete;
        ObjectMap(ObjectMap&&)                 = delete;
        ObjectMap& operator=(ObjectMap&&)      = delete;

        /// The entry of an object, default-built when it has none. A null Object is a key like any
        /// other, which no ObjectDestroy ever erases.
        T& operator[](const Object& object)
        {
            Watch();
            return entries_[Object(object.Handle())];
        }

        /// Sets an object's entry, replacing what it held.
        template <class V>
        T& Set(const Object& object, V&& value)
        {
            Watch();
            T& entry = entries_[Object(object.Handle())];
            entry    = std::forward<V>(value);
            return entry;
        }

        /// The entry of an object, or nullptr.
        T* Find(const Object& object)
        {
            const iterator found = entries_.find(Object(object.Handle()));
            return found == entries_.end() ? nullptr : &found->second;
        }

        const T* Find(const Object& object) const
        {
            const const_iterator found = entries_.find(Object(object.Handle()));
            return found == entries_.end() ? nullptr : &found->second;
        }

        bool Contains(const Object& object) const { return Find(object) != nullptr; }

        /// Drops an object's entry; false when it had none.
        bool Erase(const Object& object) { return entries_.erase(Object(object.Handle())) != 0; }

        /// Drops the entries whose object no longer resolves; how many went.
        size_t Prune()
        {
            size_t dropped = 0;
            for (iterator it = entries_.begin(); it != entries_.end();)
            {
                if (it->first.IsValid())
                {
                    ++it;
                    continue;
                }
                it = entries_.erase(it);
                ++dropped;
            }
            return dropped;
        }

        void   Clear() { entries_.clear(); }
        size_t Size() const { return entries_.size(); }
        bool   Empty() const { return entries_.empty(); }

        /// Visits (Object, T) pairs; an object visited has not left yet.
        iterator       begin() { return entries_.begin(); }
        iterator       end() { return entries_.end(); }
        const_iterator begin() const { return entries_.begin(); }
        const_iterator end() const { return entries_.end(); }

        /// Whether the map is told when objects leave: false before the first insertion and on a
        /// runtime that does not raise ObjectDestroy.
        bool Watching() const { return static_cast<bool>(destroyed_); }

    private:
        static void WXL_CALL OnDestroy(void* user, const void* args)
        {
            const wxl_object_args* object = static_cast<const wxl_object_args*>(args);
            static_cast<ObjectMap*>(user)->entries_.erase(Object(object->object));
        }

        /// Subscribes once; retried while the runtime has not answered (before the mod loaded).
        void Watch()
        {
            if (destroyed_) return;
            destroyed_ = Subscribe(WXL_EV_ObjectDestroy, &ObjectMap::OnDestroy, this, kPriority);
        }

        map_type     entries_;
        Subscription destroyed_;
    };
} // namespace wxl

#endif // WXL_OBJECTS_MAP_HPP
