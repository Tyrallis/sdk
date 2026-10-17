// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.script: functions and events on the client's scripting surface.

#ifndef WXL_SERVICE_SCRIPT_HPP
#define WXL_SERVICE_SCRIPT_HPP

#include <initializer_list>
#include <optional>
#include <string_view>
#include <type_traits>

#include "wxl/sdk.hpp"
#include "wxl/services/script.h"

namespace wxl
{
    /// Builds the values an event carries to script. A string's storage must outlive the call.
    struct Value
    {
        static wxl_value Nil()
        {
            wxl_value value{};
            value.type = WXL_VALUE_NIL;
            return value;
        }

        static wxl_value Number(double number)
        {
            wxl_value value{};
            value.type      = WXL_VALUE_NUMBER;
            value.as.number = number;
            return value;
        }

        static wxl_value String(std::string_view text)
        {
            wxl_value value{};
            value.type      = WXL_VALUE_STRING;
            value.as.string = Str(text);
            return value;
        }

        static wxl_value Bool(bool flag)
        {
            wxl_value value{};
            value.type       = WXL_VALUE_BOOL;
            value.as.boolean = flag ? WXL_TRUE : WXL_FALSE;
            return value;
        }
    };

    /**
     * @brief One invocation of a registered function: its arguments and its return values.
     *
     * Valid for that call only. An argument of the wrong type reads as nullopt, not as a
     * conversion.
     */
    class ScriptCall
    {
    public:
        ScriptCall(const wxl_script_api* api, wxl_script_call* call) : api_(api), call_(call) {}

        uint32_t ArgCount() const { return api_->arg_count(call_); }

        wxl_value_type ArgType(uint32_t index) const
        {
            return static_cast<wxl_value_type>(api_->arg_type(call_, index));
        }

        std::optional<double> Number(uint32_t index) const
        {
            double out = 0.0;
            if (api_->arg_number(call_, index, &out) != WXL_TRUE) return std::nullopt;
            return out;
        }

        /// Points into the VM; copy it to keep it past the call.
        std::optional<std::string_view> String(uint32_t index) const
        {
            wxl_str out{nullptr, 0};
            if (api_->arg_string(call_, index, &out) != WXL_TRUE) return std::nullopt;
            return View(out);
        }

        std::optional<bool> Bool(uint32_t index) const
        {
            wxl_bool out = WXL_FALSE;
            if (api_->arg_bool(call_, index, &out) != WXL_TRUE) return std::nullopt;
            return out != WXL_FALSE;
        }

        void PushNumber(double value) { api_->push_number(call_, value); }
        void PushString(std::string_view value) { api_->push_string(call_, Str(value)); }
        void PushBool(bool value) { api_->push_bool(call_, value ? WXL_TRUE : WXL_FALSE); }
        void PushNil() { api_->push_nil(call_); }

        /// Fails the call on the script side once it unwinds; this function returns normally.
        void Fail(std::string_view message) { api_->fail(call_, Str(message)); }

        wxl_script_call* Raw() const { return call_; }

    private:
        const wxl_script_api* api_;
        wxl_script_call*      call_;
    };

    /**
     * @brief The typed facade over wxl.script.
     *
     * Registrations do not survive a context: register again on every ScriptVmReady. Every call
     * answers WXL_E_UNSUPPORTED while the runtime does not publish the service.
     */
    class ScriptService : public detail::Facade<ScriptService, wxl_script_api>
    {
    public:
        static constexpr const char* kName    = WXL_SCRIPT_SERVICE;
        static constexpr uint32_t    kVersion = WXL_SCRIPT_SERVICE_VERSION;

        /// Which context is live.
        Expected<wxl_script_context> Context() const
        {
            const wxl_script_api* api = Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};
            return static_cast<wxl_script_context>(api->context());
        }

        /// Registers a raw function, with its user pointer.
        wxl_result RegisterFunction(const char* name, wxl_script_fn fn, void* user) const
        {
            const wxl_script_api* api = Raw();
            if (!api) return WXL_E_UNSUPPORTED;
            if (!name || !fn) return WXL_E_INVALID;
            return api->register_function(name, fn, user);
        }

        /**
         * @brief Registers a plain function taking a ScriptCall.
         *
         *     void Greet(wxl::ScriptCall& call);
         *     script.RegisterFunction<&Greet>("Greet");
         */
        template <void (*Fn)(ScriptCall&)>
        wxl_result RegisterFunction(const char* name) const
        {
            return RegisterFunction(name, &FunctionThunk<Fn>, nullptr);
        }

        /**
         * @brief Registers a callable object, called as `(*handler)(ScriptCall&)`.
         *
         * The object must outlive the context it is registered on.
         */
        template <class Handler>
        wxl_result RegisterFunction(const char* name, Handler* handler) const
        {
            static_assert(!std::is_function_v<Handler>,
                          "pass a plain function as RegisterFunction<&Fn>(name)");
            if (!handler) return WXL_E_INVALID;
            return RegisterFunction(name, &HandlerThunk<Handler>, handler);
        }

        wxl_result RegisterEvent(const char* name) const
        {
            const wxl_script_api* api = Raw();
            if (!api) return WXL_E_UNSUPPORTED;
            return api->register_event(name);
        }

        wxl_result FireEvent(const char* name, const wxl_value* args, uint32_t count) const
        {
            const wxl_script_api* api = Raw();
            if (!api) return WXL_E_UNSUPPORTED;
            return api->fire_event(name, args, count);
        }

        /// `script.FireEvent("MY_EVENT", {wxl::Value::Number(1), wxl::Value::String("x")});`
        wxl_result FireEvent(const char* name, std::initializer_list<wxl_value> args) const
        {
            return FireEvent(name, args.begin(), static_cast<uint32_t>(args.size()));
        }

        /// Runs a chunk of script text in the live context; @p name labels it in errors.
        wxl_result Run(std::string_view chunk, const char* name) const
        {
            const wxl_script_api* api = Raw();
            if (!api) return WXL_E_UNSUPPORTED;
            return api->run(Str(chunk), name);
        }

    private:
        template <void (*Fn)(ScriptCall&)>
        static void WXL_CALL FunctionThunk(wxl_script_call* call, void*)
        {
            ScriptCall typed(Raw(), call);
            Fn(typed);
        }

        template <class Handler>
        static void WXL_CALL HandlerThunk(wxl_script_call* call, void* user)
        {
            ScriptCall typed(Raw(), call);
            (*static_cast<Handler*>(user))(typed);
        }
    };
} // namespace wxl

#endif // WXL_SERVICE_SCRIPT_HPP
