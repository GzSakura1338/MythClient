#pragma once
#include "config/settings/Properties.h"

namespace nova::ui::nametags {
struct Appearance {
    Properties::ColorProp background{0xCC331914,true};
    Properties::ColorProp text{0xFFFFEAE5,false};
    Properties::BoolProperty shadow{false};

    std::vector<Properties::PropDesc> Rows(Properties::PropDesc::Visibility visible = {}) {
        std::vector<Properties::PropDesc> rows;
        for (auto* color : {&background,&text}) {
            for (const auto& row : color->descriptors()) {
                rows.emplace_back(row.getName(),row.getProp(),std::vector<Properties::PropDesc>{},
                    [row,visible] { return (!visible || visible()) && row.isVisible(); });
            }
        }
        rows.emplace_back("Text Shadow",&shadow,std::vector<Properties::PropDesc>{},std::move(visible));
        return rows;
    }

    std::vector<Properties::PropDesc> ConfigRows() {
        return {{"Background",&background},{"Text",&text},{"Text Shadow",&shadow}};
    }
};
}
