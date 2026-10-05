#pragma once

#include <memory>
#include <vector>

#include "config/settings/Properties.h"
#include "ui/clickgui/panel/Elements.h"

namespace nova::menu::panel {

class PanelClickGui;

class PropertyComponent {
public:
    PropertyComponent(PanelClickGui& gui, const Properties::PropDesc& descriptor);
    ~PropertyComponent();

    PropertyComponent(const PropertyComponent&) = delete;
    PropertyComponent& operator=(const PropertyComponent&) = delete;

    void render();

    [[nodiscard]] bool isVisible() const { return descriptor_.isVisible(); }

private:
    void renderGroupPopupButton();
    void renderGroupPopup(ImVec2 anchorMin, ImVec2 anchorMax);
    void renderModulePopup();

    PanelClickGui& gui_;
    Properties::PropDesc descriptor_;
    Properties::AbstractProperty* property_ = nullptr;
    std::unique_ptr<ElementRenderer> element_;
    std::vector<std::unique_ptr<PropertyComponent>> children_;
    std::unique_ptr<PropertyComponent> moduleMenuRow_;
};

}
