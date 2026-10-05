#pragma once

namespace nova::menu::panel {

class PanelClickGui;

class PanelComponent {
public:
    explicit PanelComponent(PanelClickGui& gui) : gui_(gui) {}
    virtual ~PanelComponent() = default;

    PanelComponent(const PanelComponent&) = delete;
    PanelComponent& operator=(const PanelComponent&) = delete;

    virtual void render() = 0;
    [[nodiscard]] virtual bool isVisible() const { return true; }

protected:
    PanelClickGui& gui_;
};

}
