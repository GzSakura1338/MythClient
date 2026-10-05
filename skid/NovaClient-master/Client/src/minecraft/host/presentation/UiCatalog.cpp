#include "minecraft/host/presentation/UiCatalog.h"

#include <stdexcept>
#include "minecraft/modules/ModuleManager.h"
#include "ui/clickgui/FeatureTree.h"
#include "ui/clickgui/ClickGui.h"

namespace nova::minecraft {
void InstallUiCatalog() {
    using namespace module;
    ui::FeatureCatalog catalog;
    constexpr const char* ids[] = {"combat", "player", "movement", "render", "misc"};
    constexpr const char* labels[] = {"Combat", "Player", "Movement", "Render", "Miscellaneous"};
    constexpr const char8_t* icons[] = {u8"\uf05b", u8"\uf007", u8"\uf8cc", u8"\uf03e", u8"\uf1de"};
    constexpr int order[] = {0, 2, 1, 3, 4};
    static_assert(kCategoryCount == std::size(ids));
    for (int i = 0; i < kCategoryCount; ++i) {
        const auto info = Describe(static_cast<Category>(i));
        catalog.categories.push_back({ids[i], std::string(info.name), info.icon,
            info.face == IconFace::Frost ? ui::IconFace::Frost : ui::IconFace::Fatal,
            labels[i], icons[i], order[i]});
    }
    for (const auto& module : ModuleManager::Get().All()) {
        ui::Feature feature;
        feature.id = "minecraft." + module->name();
        feature.name = module->name();
        feature.category = ids[static_cast<int>(module->category())];
        feature.page = module->page();
        feature.subpage = module->subpage();
        feature.hud = module->category() == Category::Render && module->name() != "ClickGUI" && module->name() != "FreeLook";
        feature.enabled = module->enabledProperty();
        feature.hiddenInArrayList = &module->hiddenInArrayListProperty();
        feature.settings = module->settings();
        catalog.features.push_back(std::move(feature));
    }
    if (!menu::InstallFeatureCatalog(std::move(catalog)))
        throw std::logic_error("Minecraft UI catalog could not be installed");
    clickgui::SetCloseHandler([] {
        if (auto* feature = ModuleManager::Get().Find("ClickGUI")) feature->setEnabled(false);
    });
    clickgui::SetOpenHandler([] {
        if(auto* feature=ModuleManager::Get().Find("ClickGUI")){
            for(const auto& row:feature->settings())if(std::string_view(row.getName())=="Mode")
                if(auto* mode=dynamic_cast<Properties::ModeProperty*>(row.getProp()))mode->set(0);
            feature->setEnabled(true);
        }
    });
}
}
