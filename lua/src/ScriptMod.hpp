// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// One Lua mod: its folder, its state, the API installed in it, its callbacks and its errors.

#ifndef WXL_LUA_SCRIPT_MOD_HPP
#define WXL_LUA_SCRIPT_MOD_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Lua.hpp"
#include "Objects.hpp"
#include "wxl/host/discovery.hpp"
#include "wxl/reflect.hpp"

namespace wxl::lua
{
    class Runtime;

    class ScriptMod
    {
    public:
        /// The callbacks registerForEvent takes, in the order of @ref kCallbackNames.
        enum class Callback : uint32_t
        {
            Init,
            Update,
            Draw,
            Shutdown,
            Count
        };

        static constexpr const char* kCallbackNames[] = {"onInit", "onUpdate", "onDraw", "onShutdown"};

        /// Handler errors a mod may make before it is disabled, counted since its last load.
        static constexpr uint32_t kErrorLimit = 25;

        /// @p rank is its place in the load order: 0 for the first mod loaded.
        ScriptMod(Runtime& runtime, host::ModCandidate candidate, uint32_t rank);
        ~ScriptMod();

        ScriptMod(const ScriptMod&)            = delete;
        ScriptMod& operator=(const ScriptMod&) = delete;

        const std::string&           Id() const { return candidate_.manifest.id; }
        const host::Manifest&        Manifest() const { return candidate_.manifest; }
        const std::filesystem::path& Directory() const { return candidate_.directory; }

        /// The entry file, from the manifest's "lua".
        std::filesystem::path Entry() const;

        /// Its place in the load order, kept across reloads: its handlers run after those of
        /// every mod of a lower rank and before those of every mod of a higher one.
        uint32_t Rank() const { return rank_; }

        /// Whether it came with a mod.json; false for a folder found by its init.lua alone.
        bool HasManifest() const { return manifested_; }

        /**
         * @brief Reads the mod.json again, for a reload.
         *
         * False with @p problem set when it no longer parses, gives another id, or names no
         * "lua" entry. True, the manifest kept as it was, for a mod without one.
         */
        bool Reread(std::string& problem);

        bool       Loaded() const { return state_ != nullptr; }
        bool       Disabled() const { return disabled_; }
        /// Whether its code may run: loaded and not disabled.
        bool       Runs() const { return state_ && !disabled_; }
        lua_State* State() const { return state_; }
        int        TracebackRef() const { return traceback_; }

        /**
         * @brief Opens the state, installs the API and the sandbox, runs the entry file.
         *
         * False when the entry does not compile or fails; the error is logged and the state
         * closed. The runtime's handler list for this mod must be empty on entry.
         */
        bool Load();

        /// Closes the state and forgets the callbacks. The runtime drops the handlers first.
        void Close();

        /// Stops its code from running until the next load, and gives back its field watches
        /// and its search folders. The state stays until Close.
        void Disable();

        /// The fields it watches through wxl.object.Watch: what decides which ObjectFieldChanged
        /// its handlers receive. Given back at Disable and Close.
        const FieldWatches& Watches() const { return fieldWatches_; }

        /// Runs every callback registered under a name; onUpdate receives @p delta.
        void Call(Callback which, float delta = 0.0f);

        /**
         * @brief Logs an error of the mod's code and counts it toward the limit.
         * @param what     what the code was doing: "in its handler for OnUpdate".
         * @param message  the error with its traceback.
         */
        void ReportError(std::string_view what, std::string_view message);

        // --- the files its code comes from ------------------------------------------------

        /**
         * @brief Watches a file the mod's code comes from, as it is now.
         *
         * Its mod.json, its entry file, and every file the sandbox loads code from for it
         * (loadfile, dofile, require), recorded as each is loaded; a module require looked for
         * and did not find is watched for its appearance. What the mod reads or writes with
         * io.open is data, never watched: a mod writing into its own folder does not reload.
         * Load starts the list over.
         */
        void Watch(const std::filesystem::path& file);

        /// The first watched file that differs from when it was watched (written, created,
        /// removed), relative to the mod's folder; empty when none does.
        std::filesystem::path Changed() const;

        /// Starts the list over with its mod.json and its entry file, as they are now.
        void Rewatch();

    private:
        static ScriptMod& Self(lua_State* L);

        static int L_print(lua_State* L);
        static int L_log(lua_State* L);
        static int L_registerForEvent(lua_State* L);
        static int L_Observe(lua_State* L);
        static int L_Override(lua_State* L);
        static int L_Unobserve(lua_State* L);
        static int L_traceback(lua_State* L);

        /// Observe and Override: resolves the event, keeps the function, tells the runtime.
        int Subscribe(lua_State* L, bool override);

        /// Installs the globals, the family tables and the `wxl` table.
        void Install();

        /**
         * @brief Subscribes the handlers the entry file defined on the family tables.
         *
         * Every member of every family table must be a function named after an event of that
         * family; it is subscribed as Observe, or as Override when the event can be answered.
         * @return false with @p error set for a member that is neither.
         */
        bool BindFamilies(std::string& error);

        void PushClosure(lua_CFunction fn);

        /// Empty when the mod may subscribe to @p event, otherwise why not: the event came with a
        /// later contract than the "abi" its manifest asks for.
        std::string TooNew(const reflect::Event& event) const;

        /// A watched file as it was: whether it existed, its size and its last write.
        struct Watched
        {
            std::filesystem::path file;
            bool                  exists  = false;
            uint64_t              size    = 0;
            int64_t               written = 0; ///< in the filesystem clock's ticks

            bool Same(const Watched& other) const
            {
                return exists == other.exists && size == other.size && written == other.written;
            }
        };

        static Watched Look(const std::filesystem::path& file);

        Runtime&             runtime_;
        host::ModCandidate   candidate_;
        uint32_t             rank_       = 0;
        bool                 manifested_ = false;
        lua_State*           state_      = nullptr;
        bool                 disabled_   = false;
        uint32_t             errors_     = 0;
        int                  traceback_  = LUA_NOREF;
        std::vector<int>     callbacks_[static_cast<size_t>(Callback::Count)];
        std::vector<Watched> watched_;
        FieldWatches         fieldWatches_;
    };
} // namespace wxl::lua

#endif // WXL_LUA_SCRIPT_MOD_HPP
