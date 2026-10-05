#include "ui/clickgui/panel/ClientSettings.h"

#include "core/Instance.h"
#include "bind/UnloadKey.h"

namespace nova::menu::panel {

ClientSettings& ClientSettings::Get() {
    return Instance<ClientSettings>([] { return new ClientSettings(); });
}

std::string ClientSettings::clientName() const {
    if(!customClientName_.get())return "NOVA";
    auto name=clientName_.get();
    for(char& c:name)if(static_cast<unsigned char>(c)<32)c=' ';
    const auto first=name.find_first_not_of(" \t\r\n");
    if(first==std::string::npos)return "NOVA";
    return name.substr(first,name.find_last_not_of(" \t\r\n")-first+1);
}

ClientSettings::ClientSettings()
    : theme_(static_cast<int>(Theme::Dark), {"Light", "Dark"})
    , guiSize_(60.0, 150.0, 5.0, 100.0, "{:.0f}%") {
    bind::unload::property.store(&unloadKey_);
    unloadKey_.setKeyChangeConsumer([](int key) { bind::unload::SetKey(key); });
    rows_.emplace_back("Theme", &theme_);
    rows_.emplace_back("Size", &guiSize_);
    rows_.emplace_back("Stage Manager", &stageManager_);
    rows_.emplace_back("Unload Key", &unloadKey_);
    rows_.emplace_back("Custom Client Name", &customClientName_);
    rows_.emplace_back("Client Name", &clientName_, std::vector<Properties::PropDesc>{},
                       [this] { return customClientName_.get(); });
    rows_.emplace_back("HUD Background Alpha", &hudBackgroundAlpha_);
    rows_.emplace_back("Blur", &blur_, std::vector<Properties::PropDesc>{}, [this] { return blurSupported_; });
    rows_.emplace_back("Blur Strength", &blurStrength_, std::vector<Properties::PropDesc>{},
                       [this] { return blurSupported_ && blur_.get(); });
    rows_.emplace_back("Blur FPS", &blurFps_, std::vector<Properties::PropDesc>{},
                       [this] { return blurSupported_ && blur_.get(); });
}

}
