#pragma once

#include <string>
#include <vector>
#include "config/settings/Properties.h"

namespace nova::ui {

enum class IconFace { Fatal, Frost };

struct FeatureCategory {
    std::string id;
    std::string name;
    std::u8string icon;
    IconFace face = IconFace::Fatal;
    std::string panelName;
    std::u8string panelIcon;
    int panelOrder = 0;
};

// Registered before rendering starts and immutable for the UI session. The host
// owns properties and descriptor labels until all UI consumers have stopped.
// Property access stays on the existing settings thread during this migration;
// this is not permission to read/write game objects from a rendering callback.
struct Feature {
    std::string id;
    std::string name;
    std::string category;
    std::string page;
    std::string subpage;
    bool hud = false;
    Properties::AbstractProperty* enabled = nullptr;
    Properties::BoolProperty* hiddenInArrayList = nullptr;
    std::vector<Properties::PropDesc> settings;
};

struct FeatureCatalog {
    std::vector<FeatureCategory> categories;
    std::vector<Feature> features;
};

}
