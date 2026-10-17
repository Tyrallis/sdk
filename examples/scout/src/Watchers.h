// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The three watchers, one per family of events Scout cares about. A watcher is a plain class
// that derives from a script family and overrides the handful of hooks it wants; the ones it
// does not override cost nothing.

#ifndef SCOUT_WATCHERS_H
#define SCOUT_WATCHERS_H

#include <map>
#include <set>

#include "wxl/objects.hpp"
#include "wxl/sdk.hpp"

#include "Journal.h"

namespace scout
{
    /// The tag every line of Scout's log carries.
    inline constexpr const char* kTag = "scout";

    /**
     * @brief The world coming and going, and the clock.
     *
     * The journal is about one place: entering a world clears it, so what you read is what
     * happened here.
     */
    class WorldWatch final : public wxl::WorldScript
    {
    public:
        WorldWatch(Clock& clock, Journal& journal) : clock_(clock), journal_(journal) {}

        void OnUpdate(const wxl_update_args& args) override;
        void OnWorldEnter(const wxl_world_args& args) override;
        void OnWorldLeave(const wxl_world_args& args) override;

        /// The map we are on, or 0 while out of the world.
        uint32_t MapId() const { return mapId_; }

    private:
        Clock&   clock_;
        Journal& journal_;
        uint32_t mapId_ = 0;
    };

    /**
     * @brief What lives around you: units arriving, leaving, dying, and what you target.
     *
     * Every hook here is handed a handle, never a pointer. A handle carries a generation, so
     * one kept past its subject's death answers "gone" instead of reading whatever took its
     * place -- which is why the reads below can fail, and why they say which failure it was.
     */
    class ObjectWatch final : public wxl::ObjectScript
    {
    public:
        /// Only units within this many yards of you are worth a line.
        static constexpr float kRange = 60.0f;

        ObjectWatch(Clock& clock, Journal& journal) : clock_(clock), journal_(journal) {}

        void OnObjectCreate(const wxl_object_args& args) override;
        void OnObjectDestroy(const wxl_object_args& args) override;
        void OnUnitDeath(const wxl_unit_args& args) override;
        void OnTargetChanged(const wxl_target_args& args) override;

    private:
        /// Writes a line about a unit, unless it is too far away to care about.
        void Note(What what, wxl::Unit unit);

        Clock&   clock_;
        Journal& journal_;
    };

    /**
     * @brief The traffic between the client and the server.
     *
     * Counting is harmless. Muting is not: an opcode the client needs, dropped, leaves it out
     * of step with the server for as long as the mute lasts. Nothing is muted unless something
     * asks for it by number.
     */
    class NetWatch final : public wxl::NetScript
    {
    public:
        void OnMessageReceived(const wxl_message_args& args) override;
        void OnMessageSent(const wxl_message_args& args) override;

        /// How many messages of that opcode have crossed, in the direction asked for.
        uint32_t Count(uint32_t opcode, bool incoming) const;

        /// Starts or stops dropping an incoming opcode. Answers what it was before.
        bool Mute(uint32_t opcode, bool muted);
        bool IsMuted(uint32_t opcode) const { return muted_.count(opcode) != 0; }

    private:
        std::map<uint32_t, uint32_t> incoming_;
        std::map<uint32_t, uint32_t> outgoing_;
        std::set<uint32_t>           muted_;
    };
} // namespace scout

#endif // SCOUT_WATCHERS_H
