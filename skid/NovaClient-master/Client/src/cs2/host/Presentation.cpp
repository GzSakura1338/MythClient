#include "cs2/host/Presentation.h"
#include "cs2/sdk/Interfaces.h"
#include "cs2/modules/ModuleManager.h"
#include "core/Instance.h"
#include "core/RuntimeControl.h"
#include "config/HostConfig.h"
#include "config/ConfigManager.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/clickgui/FeatureTree.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/notifications/Notifications.h"

namespace nova::cs2::presentation {
namespace {
struct State {
    std::filesystem::path directory;
    bool preview = false;
};
State& Get() { return Instance<State>(); }
void Save(json::Value& root) { modules::ModuleManager::Get().Save(root); }
void Load(const json::Value& root) { modules::ModuleManager::Get().Load(root); }
void Closed() {
    if (auto* module = modules::ModuleManager::Get().Find("ClickGUI")) module->setEnabled(false);
}
bool Initialize() {
    auto& state = Get();
    modules::RegisterBuiltinModules(state.preview);
    if (!menu::InstallFeatureCatalog(modules::ModuleManager::Get().Catalog())) return false;
    if (!config::InstallHostConfig({state.directory, Save, Load})) return false;
    auto& settings = menu::panel::ClientSettings::Get();
    settings.SetBlurSupported(true);
    config::SetClientSettings(&settings.rows());
    config::ConfigManager::Get().Init();
    clickgui::SetCloseHandler(Closed);
    clickgui::SetOpenHandler([]{if(auto* module=modules::ModuleManager::Get().Find("ClickGUI")){if(auto* mode=module->modeProperty())mode->set(0);module->setEnabled(true);}});
    notifications::Post("CS2 UI ready", notifications::Type::Success);
    LogTo("CS2: module registry and shared ClickGUI/HUD initialized");
    return true;
}
void Draw() {
    auto& manager = modules::ModuleManager::Get();
    manager.Tick();
    manager.RenderOverlays();
}
void Stop() {
    clickgui::Close();
    auto& configs=config::ConfigManager::Get();
    configs.SaveForUnload();
    LogTo("CS2: unload config: %s",configs.Status().c_str());
    modules::ModuleManager::Get().Shutdown();
    clickgui::SetCloseHandler(nullptr);
    clickgui::SetOpenHandler(nullptr);
}
void Key(unsigned key) {
    // ClickGUI activation uses its configurable BindProperty.
    if (key == VK_ESCAPE && clickgui::IsOpen()) clickgui::Close();
}
void Unload() {
    LogTo("CS2: unload binding received on window thread; requesting full unload");
    RequestUnload();
}
}
void Configure(std::filesystem::path directory, bool preview) { Get().directory = std::move(directory); Get().preview = preview; }
ui::Dx11Callbacks Callbacks() { return {Initialize, Draw, Stop, Key, clickgui::CapturesInput, sdk::CaptureMenuMouse, Unload}; }
}
