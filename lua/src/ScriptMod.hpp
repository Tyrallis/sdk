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
#include "wxl/host/discovery.hpp"

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

        ScriptMod(Runtime& runtime, host::ModCandidate candidate);
        ~ScriptMod();

        ScriptMod(const ScriptMod&)            = delete;
        ScriptMod& operator=(const ScriptMod&) = delete;

        const std::string&           Id() const { return candidate_.manifest.id; }
        const host::Manifest&        Manifest() const { return candidate_.manifest; }
        const std::filesystem::path& Directory() const { return candidate_.directory; }

        /// The entry file, from the manifest's "lua".
        std::filesystem::path Entry() const;

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

        /// Stops its code from running until the next load. The state stays until Close.
        void Disable() { disabled_ = true; }

        /// Runs every callback registered under a name; onUpdate receives @p delta.
        void Call(Callback which, float delta = 0.0f);

        /**
         * @brief Logs an error of the mod's code and counts it toward the limit.
         * @param what     what the code was doing: "in its handler for OnUpdate".
         * @param message  the error with its traceback.
         */
        void ReportError(std::string_view what, std::string_view message);

        // --- the folder watch -------------------------------------------------------------

        /// What a scan of the folder sees; the same before and after means nothing changed.
        struct Signature
        {
            uint64_t files = 0;
            uint64_t bytes = 0;
            int64_t  newest = 0; ///< the latest write time, in the filesystem clock's ticks

            bool operator==(const Signature& other) const
            {
                return files == other.files && bytes == other.bytes && newest == other.newest;
            }
            bool operator!=(const Signature& other) const { return !(*this == other); }
        };

        Signature Scan() const;

        /// Whether the folder differs from the last @ref Mark.
        bool Changed() const { return Scan() != signature_; }

        /// Records the folder as it is now.
        void Mark() { signature_ = Scan(); }

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

        Runtime&            runtime_;
        host::ModCandidate  candidate_;
        lua_State*          state_     = nullptr;
        bool                disabled_  = false;
        uint32_t            errors_    = 0;
        int                 traceback_ = LUA_NOREF;
        std::vector<int>    callbacks_[static_cast<size_t>(Callback::Count)];
        Signature           signature_;
    };
} // namespace wxl::lua

#endif // WXL_LUA_SCRIPT_MOD_HPP
