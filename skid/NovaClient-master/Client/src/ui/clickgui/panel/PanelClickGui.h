#pragma once

#include <memory>
#include <string>

#include "config/settings/Properties.h"
#include "ui/clickgui/panel/PopupAnimation.h"
#include "ui/clickgui/panel/Theme.h"
#include "ui/clickgui/panel/TopBar.h"

namespace nova::menu::panel {

class PageLayout;

class PanelClickGui {
public:
    PanelClickGui();
    ~PanelClickGui();

    PanelClickGui(const PanelClickGui&) = delete;
    PanelClickGui& operator=(const PanelClickGui&) = delete;

    void render();
    void resumeFromWorkspace();

    [[nodiscard]] const ColorTheme& colors() const;
    [[nodiscard]] Properties::ModeProperty& themeSetting();

    [[nodiscard]] bool isSearching() const { return searching_; }
    void setSearching(bool value) { searching_ = value; if (!value) search_.clear(); }

    [[nodiscard]] const std::string& search() const { return search_; }
    void setSearch(std::string value) { search_ = std::move(value); }
    PageLayout& layout() { return *layout_; }
    void pageChanged();
    float pageReveal();


private:
    PopupAnimations popupAnimations_;
    float renderedScale_ = 0.f;
    float appliedScale_ = 0.0f;
    int lastFrame_ = -2;
    double revealStart_ = 0.0;
    float pageMix_ = 1.f;
    bool searching_ = false;
    std::string search_;

    TopBar topBar_;
    std::unique_ptr<PageLayout> layout_;
};

}
