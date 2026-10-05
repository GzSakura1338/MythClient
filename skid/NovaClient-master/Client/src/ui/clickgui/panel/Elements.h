#pragma once

#include <memory>

#include "config/settings/Properties.h"

namespace nova::menu::panel {

class PanelClickGui;

class ElementRenderer {
public:
    explicit ElementRenderer(PanelClickGui& gui) : gui_(gui) {}
    virtual ~ElementRenderer() = default;

    ElementRenderer(const ElementRenderer&) = delete;
    ElementRenderer& operator=(const ElementRenderer&) = delete;

    [[nodiscard]] virtual float width(const Properties::AbstractProperty& prop) const = 0;
    [[nodiscard]] virtual float height(const Properties::AbstractProperty& prop) const;
    virtual void draw(Properties::AbstractProperty& prop) = 0;

protected:
    PanelClickGui& gui_;
};

[[nodiscard]] std::unique_ptr<ElementRenderer> MakeElement(PanelClickGui& gui,
                                                           Properties::AbstractProperty& prop);

}
