#include <cassert>
#include <cstring>
#include <cstdio>
#include "ui/clickgui/Client.h"
#include "ui/clickgui/FeatureTree.h"

int main() {
    using namespace nova;
    Properties::BoolProperty enabled{false}, hidden{false}, secondEnabled{true};
    Properties::NumberProp volume{0,100,1,70};
    ui::FeatureCatalog source;
    source.categories.push_back({"media","MEDIA",u8"\ue620",ui::IconFace::Fatal,"Media",u8"\uf001",0});
    source.categories.push_back({"empty","EMPTY",u8"",ui::IconFace::Fatal,"Empty",u8"",1});
    source.features.push_back({"media.music","MusicHUD","media","General","Overlay",true,&enabled,&hidden,
        {{"Enabled",&enabled},{"Volume",&volume}}});
    source.features.push_back({"media.extra","Second","media","General","Overlay",true,&secondEnabled,nullptr,{}});

    auto invalid=source;invalid.features[0].category="unknown";
    assert(!menu::InstallFeatureCatalog(std::move(invalid)));
    invalid=source;invalid.features[0].enabled=nullptr;
    assert(!menu::InstallFeatureCatalog(std::move(invalid)));
    invalid=source;invalid.features[1].id=invalid.features[0].id;
    assert(!menu::InstallFeatureCatalog(std::move(invalid)));
    invalid=source;invalid.features[1].enabled=&enabled;
    assert(!menu::InstallFeatureCatalog(std::move(invalid)));
    invalid=source;invalid.categories.push_back(invalid.categories.front());
    assert(!menu::InstallFeatureCatalog(std::move(invalid)));
    assert(menu::Features().features.empty());
    assert(menu::InstallFeatureCatalog(std::move(source)));
    assert(!menu::InstallFeatureCatalog({})); // Existing rows must remain valid.
    const auto* feature=menu::FeatureForToggle(&enabled);
    assert(feature && feature->id=="media.music");
    assert(!menu::FeatureForToggle(nullptr) && !menu::FeatureForToggle(&volume));
    const auto row=menu::FeatureToggleRow(*feature);
    assert(std::strcmp(row.getName(),"MusicHUD")==0 && row.getProp()==&enabled);
    assert(row.getSubProps().size()==1 && row.getSubProps()[0].getProp()==&volume);
    feature->hiddenInArrayList->set(true);assert(hidden.get());
    assert(Client::FeatureCategories.size()==2);
    const auto& page=Client::FeatureCategories[0].pages[0];
    assert(std::strcmp(page.subpages[0].name,"HUD")==0);
    assert(page.subpages[0].groups[0].features.size()==2);
    assert(Client::FeatureCategories[1].pages[0].subpages[0].groups.empty());
    puts("PASS: native UI catalog, validation, HUD grouping, property bindings and stable session storage; no Minecraft/JNI");
}
