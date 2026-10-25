// An add-on that lives in the mod's memory, not on the disk. The provider below answers the
// client's questions about three names -- the add-on's folder, its .toc and its .lua -- and the
// client lists, loads and runs HelloProvider as if it were in Interface\AddOns. A transform then
// fills the mod's version into the add-on's Lua as the client reads it.

#include <cctype>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "wxl/sdk.hpp"
#include "wxl/services/storage.hpp"

namespace
{
    constexpr const char* kTag     = "asset-provider";
    constexpr const char* kVersion = "1.2.0";

    /// A name as the client compares it: any case, backslashes.
    std::string Key(std::string_view name)
    {
        std::string key(name);
        for (char& c : key) c = c == '/' ? '\\' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return key;
    }

    /// The add-on's files, by name.
    class HelloAddOn final : public wxl::AssetProvider
    {
    public:
        HelloAddOn()
        {
            files_[Key("Interface\\AddOns\\HelloProvider\\HelloProvider.toc")] =
                "## Interface: 30300\n## Title: HelloProvider\nHelloProvider.lua\n";
            files_[Key("Interface\\AddOns\\HelloProvider\\HelloProvider.lua")] =
                "DEFAULT_CHAT_FRAME:AddMessage(\"Hello from a provider, version $VERSION\")\n";
        }

        // Asked at every open and existence check: a lookup, nothing more.
        wxl::AssetState Lookup(std::string_view path) override
        {
            return files_.count(Key(path)) ? wxl::AssetState::Present : wxl::AssetState::Missing;
        }

        // The work: here, a copy of the text. A provider over an archive would read it here.
        std::unique_ptr<wxl::AssetFile> Open(std::string_view path) override
        {
            const auto found = files_.find(Key(path));
            if (found == files_.end()) return nullptr;
            const std::string& text = found->second;
            return std::make_unique<wxl::MemoryAssetFile>(std::vector<uint8_t>(text.begin(), text.end()));
        }

        // The client lists Interface\AddOns from the disk; what is told here joins that list.
        void List(std::string_view folder, const std::function<void(std::string_view, bool)>& entry) override
        {
            if (Key(folder) == "interface\\addons") entry("HelloProvider", true);
        }

    private:
        std::map<std::string, std::string> files_;
    };

    /// Replaces $VERSION in the add-on's Lua; every other Lua file goes by untouched.
    bool FillVersion(std::string_view path, const uint8_t* data, size_t size, const wxl::AssetOutput& out)
    {
        if (Key(path) != "interface\\addons\\helloprovider\\helloprovider.lua") return false;
        std::string text(reinterpret_cast<const char*>(data), size);
        const size_t at = text.find("$VERSION");
        if (at == std::string::npos) return false;
        text.replace(at, 8, kVersion);
        return out.Replace(text.data(), text.size());
    }

    class AssetProviderMod final : public wxl::Mod
    {
    public:
        wxl_result OnLoad() override
        {
            const wxl::StorageService storage;
            // Priority 1: above the client's own files, which are at 0.
            auto provider = storage.AddProvider(addOn_, 1);
            if (!provider)
            {
                wxl::Log(WXL_LOG_WARN, kTag, "no provider: {}", wxl::ResultName(provider.Error()));
                return WXL_OK;
            }
            provider_ = std::move(*provider);

            // Every .lua file the client reads passes through here, so the check is cheap.
            auto transform = storage.AddTransform("lua", 0, 0, &FillVersion);
            if (transform) transform_ = std::move(*transform);
            wxl::Log(WXL_LOG_INFO, kTag, "HelloProvider served; look for it in the add-on list");
            return WXL_OK;
        }

    private:
        HelloAddOn             addOn_;
        wxl::AssetRegistration provider_;
        wxl::AssetRegistration transform_;
    };
}

WXL_MOD(AssetProviderMod, "wxl.asset-provider", "Asset provider")
