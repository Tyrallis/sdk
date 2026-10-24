// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The typed facade over wxl.ui: a developer panel drawn over the game.

#ifndef WXL_SERVICE_UI_HPP
#define WXL_SERVICE_UI_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include "wxl/sdk.hpp"
#include "wxl/services/ui.h"

namespace wxl
{
    /**
     * @brief A registered panel, removed when this goes out of scope.
     *
     * Move-only. Release() leaves the panel registered for code that removes it by hand. On a
     * runtime whose table predates remove_panel the panel cannot be removed; UiService::CanRemovePanels
     * says which case this is.
     */
    class Panel
    {
    public:
        Panel() = default;
        Panel(const wxl_ui_api* api, std::string title) : api_(api), title_(std::move(title)), live_(true) {}

        Panel(Panel&& other) noexcept
            : api_(other.api_), title_(std::move(other.title_)), live_(other.Release())
        {
        }
        Panel& operator=(Panel&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                api_   = other.api_;
                title_ = std::move(other.title_);
                live_  = other.Release();
            }
            return *this;
        }

        Panel(const Panel&)            = delete;
        Panel& operator=(const Panel&) = delete;

        ~Panel() { Reset(); }

        explicit operator bool() const { return live_; }
        const std::string& Title() const { return title_; }

        /// Gives up ownership without removing the panel. True when there was one.
        bool Release()
        {
            const bool was = live_;
            live_          = false;
            return was;
        }

        /// Removes the panel now. WXL_E_UNSUPPORTED when the runtime cannot remove panels.
        wxl_result Reset()
        {
            if (!live_) return WXL_OK;
            live_ = false;
            if (!api_) return WXL_E_UNSUPPORTED;
            if (api_->struct_size < offsetof(wxl_ui_api, remove_panel) + sizeof(api_->remove_panel) ||
                !api_->remove_panel)
                return WXL_E_UNSUPPORTED;
            return api_->remove_panel(title_.c_str());
        }

    private:
        const wxl_ui_api* api_ = nullptr;
        std::string       title_;
        bool              live_ = false;
    };

    /**
     * @brief The typed facade over wxl.ui.
     *
     * The controls are valid only inside a panel callback and do nothing anywhere else. A
     * control bound to a value answers true on the frame it changed.
     */
    class UiService : public detail::Facade<UiService, wxl_ui_api>
    {
    public:
        static constexpr const char* kName    = WXL_UI_SERVICE;
        static constexpr uint32_t    kVersion = WXL_UI_SERVICE_VERSION;

        /// Whether the runtime's table carries remove_panel.
        bool CanRemovePanels() const
        {
            const wxl_ui_api* api = Raw();
            return api &&
                   api->struct_size >= offsetof(wxl_ui_api, remove_panel) + sizeof(api->remove_panel) &&
                   api->remove_panel;
        }

        /// Registers a raw panel body. The title is the panel's identity: unique and stable.
        Expected<Panel> AddPanel(const char* title, wxl_panel_fn fn, void* user) const
        {
            const wxl_ui_api* api = Raw();
            if (!api) return Unexpected{WXL_E_UNSUPPORTED};
            if (!title || !fn) return Unexpected{WXL_E_INVALID};
            const wxl_result result = api->add_panel(title, fn, user);
            if (result != WXL_OK) return Unexpected{result};
            return Panel(api, title);
        }

        /// Registers a plain function as the panel body: `ui.AddPanel<&DrawPanel>("Mine")`.
        template <void (*Fn)()>
        Expected<Panel> AddPanel(const char* title) const
        {
            return AddPanel(title, &FunctionThunk<Fn>, nullptr);
        }

        /// Registers a callable object, drawn as `(*handler)()`. It must outlive the Panel.
        template <class Handler>
        Expected<Panel> AddPanel(const char* title, Handler* handler) const
        {
            static_assert(!std::is_function_v<Handler>, "pass a plain function as AddPanel<&Fn>(title)");
            if (!handler) return Unexpected{WXL_E_INVALID};
            return AddPanel(title, &HandlerThunk<Handler>, handler);
        }

        bool IsOpen() const
        {
            const wxl_ui_api* api = Raw();
            return api && api->is_open() == WXL_TRUE;
        }

        // --- controls -------------------------------------------------------------------------

        void Text(std::string_view text) const
        {
            if (const wxl_ui_api* api = Raw()) api->text(Str(text));
        }

        void Separator() const
        {
            if (const wxl_ui_api* api = Raw()) api->separator();
        }

        void SameLine() const
        {
            if (const wxl_ui_api* api = Raw()) api->same_line();
        }

        bool Button(const char* label) const
        {
            const wxl_ui_api* api = Raw();
            return api && api->button(label) == WXL_TRUE;
        }

        bool Checkbox(const char* label, bool& value) const
        {
            const wxl_ui_api* api = Raw();
            if (!api) return false;
            wxl_bool   flag    = value ? WXL_TRUE : WXL_FALSE;
            const bool changed = api->checkbox(label, &flag) == WXL_TRUE;
            value              = flag != WXL_FALSE;
            return changed;
        }

        bool SliderFloat(const char* label, float& value, float min, float max) const
        {
            const wxl_ui_api* api = Raw();
            return api && api->slider_float(label, &value, min, max) == WXL_TRUE;
        }

        bool SliderInt(const char* label, int32_t& value, int32_t min, int32_t max) const
        {
            const wxl_ui_api* api = Raw();
            return api && api->slider_int(label, &value, min, max) == WXL_TRUE;
        }

        /// Four floats in 0..1, red first.
        bool ColorEdit(const char* label, float (&rgba)[4]) const
        {
            const wxl_ui_api* api = Raw();
            return api && api->color_edit(label, rgba) == WXL_TRUE;
        }

        bool Combo(const char* label, int32_t& index, const char* const* items, int32_t count) const
        {
            const wxl_ui_api* api = Raw();
            return api && api->combo(label, &index, items, count) == WXL_TRUE;
        }

        template <size_t N>
        bool Combo(const char* label, int32_t& index, const char* const (&items)[N]) const
        {
            return Combo(label, index, items, static_cast<int32_t>(N));
        }

        /// A section the reader can fold away. True while open, which is when to draw its body.
        bool CollapsingHeader(const char* label) const
        {
            const wxl_ui_api* api = Raw();
            return api && api->collapsing_header(label) == WXL_TRUE;
        }

        bool InputText(const char* label, char* buffer, uint32_t capacity) const
        {
            const wxl_ui_api* api = Raw();
            return api && api->input_text(label, buffer, capacity) == WXL_TRUE;
        }

        template <size_t N>
        bool InputText(const char* label, char (&buffer)[N]) const
        {
            return InputText(label, buffer, static_cast<uint32_t>(N));
        }

    private:
        /// Draws a panel. What it throws ends this frame's drawing of it and is logged.
        template <class F>
        static void Draw(F&& f)
        {
            detail::Thrown thrown;
            if (detail::Contain(std::forward<F>(f), thrown)) return;
            detail::LogThrown("a panel registered with wxl.ui", thrown,
                              "; this frame's drawing of it ended there");
        }

        template <void (*Fn)()>
        static void WXL_CALL FunctionThunk(void*)
        {
            Draw([] { Fn(); });
        }

        template <class Handler>
        static void WXL_CALL HandlerThunk(void* user)
        {
            Draw([user] { (*static_cast<Handler*>(user))(); });
        }
    };
} // namespace wxl

#endif // WXL_SERVICE_UI_HPP
