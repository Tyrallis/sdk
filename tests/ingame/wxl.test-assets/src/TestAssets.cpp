// A C++ mod that serves files the client reads as its own. Its provider holds an add-on,
// WxlProvided, that exists nowhere on the disk: the client lists it among its add-ons, reads its
// .toc and runs its Lua. A transform of .lua files rewrites that add-on's file as the client reads
// it, and one of .wxltest files upper-cases them. wxl.selftest checks what the client got.

#include <cctype>
#include <map>
#include <memory>
#include <string>

#include "wxl/sdk.hpp"
#include "wxl/services/storage.hpp"

namespace
{
    constexpr const char* kTag = "test-assets";

    /// Names as the provider is asked them: lower case, backslashes.
    std::string Key(std::string_view name)
    {
        std::string key(name);
        for (char& c : key) c = c == '/' ? '\\' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return key;
    }

    /// The files of the add-on and a file to convert, in memory.
    class Provided final : public wxl::AssetProvider
    {
    public:
        Provided()
        {
            Add("Interface\\AddOns\\WxlProvided\\WxlProvided.toc",
                "## Interface: 30300\n## Title: WxlProvided\n## Notes: served by wxl.test-assets\nWxlProvided.lua\n");
            Add("Interface\\AddOns\\WxlProvided\\WxlProvided.lua", "WXL_PROVIDED_ADDON = \"loaded\"\n");
            Add("Interface\\WxlTest\\lower.wxltest", "read through a transform");
        }

        wxl::AssetState Lookup(std::string_view path) override
        {
            return files_.count(Key(path)) ? wxl::AssetState::Present : wxl::AssetState::Missing;
        }

        std::unique_ptr<wxl::AssetFile> Open(std::string_view path) override
        {
            const auto found = files_.find(Key(path));
            if (found == files_.end()) return nullptr;
            return std::make_unique<wxl::MemoryAssetFile>(std::vector<uint8_t>(found->second.begin(), found->second.end()));
        }

        void List(std::string_view folder, const std::function<void(std::string_view, bool)>& entry) override
        {
            const std::string key = Key(folder);
            if (key == "interface\\addons") entry("WxlProvided", true);
            if (key == "interface\\addons\\wxlprovided")
            {
                entry("WxlProvided.toc", false);
                entry("WxlProvided.lua", false);
            }
        }

    private:
        void Add(const std::string& name, const std::string& text) { files_[Key(name)] = text; }

        std::map<std::string, std::string> files_;
    };

    Provided                g_provided;
    wxl::AssetRegistration  g_provider;
    wxl::AssetRegistration  g_luaTransform;
    wxl::AssetRegistration  g_upper;

    /// Rewrites the provided add-on's Lua as the client reads it; every other .lua is left alone.
    bool MarkAddOn(std::string_view path, const uint8_t*, size_t, const wxl::AssetOutput& out)
    {
        if (Key(path) != "interface\\addons\\wxlprovided\\wxlprovided.lua") return false;
        static const std::string kText = "WXL_PROVIDED_ADDON = \"loaded, transformed\"\n";
        return out.Replace(kText.data(), kText.size());
    }

    bool Upper(std::string_view, const uint8_t* data, size_t size, const wxl::AssetOutput& out)
    {
        uint8_t* to = out.Replace(size);
        if (!to) return false;
        for (size_t i = 0; i < size; ++i) to[i] = static_cast<uint8_t>(std::toupper(data[i]));
        return true;
    }

    class TestAssets final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            const wxl::StorageService storage;
            auto provider = storage.AddProvider(g_provided, 5);
            auto lua      = storage.AddTransform("lua", 0, 0, &MarkAddOn);
            auto upper    = storage.AddTransform("wxltest", 0, 0, &Upper);
            if (provider) g_provider = std::move(*provider);
            if (lua) g_luaTransform = std::move(*lua);
            if (upper) g_upper = std::move(*upper);
            wxl::Log(WXL_LOG_INFO, kTag, "provider {}, transforms {} and {}", provider ? "added" : "refused",
                     lua ? "added" : "refused", upper ? "added" : "refused");
            return WXL_OK;
        }

        void OnShutdown() override
        {
            g_upper.Reset();
            g_luaTransform.Reset();
            g_provider.Reset();
        }
    };
}

WXL_MOD(TestAssets, "wxl.test-assets", "Test assets")
