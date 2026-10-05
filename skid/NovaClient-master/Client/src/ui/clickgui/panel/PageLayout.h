#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "imgui.h"
#include "contracts/UiCatalog.h"
#include "ui/clickgui/panel/PropertyComponent.h"

namespace nova::menu::panel {
class PanelClickGui;
// Category module groups in two columns, rendered with the current panel style.
class PageLayout {
public:
    using Page = int; // Catalog category index; ClientPage is the shared settings page.
    static constexpr Page ClientPage = -1;
    static constexpr Page BrowserPage = -2;
    static constexpr Page AccountsPage = -3;
    static constexpr Page ProxyPage = -4;
    explicit PageLayout(PanelClickGui&);
    void navigation(ImVec2 origin);
    void content(ImVec2 origin);
    void account(ImVec2 origin);
    Page page() const { return page_; }
    void changePage(Page);
    static Page route(const ui::Feature&);
private:
    void rebuild();
    struct Group {
        std::string name;
        int column = 0;
        std::vector<std::unique_ptr<PropertyComponent>> rows;
    };
    void card(Group&, float width);
    std::vector<const ui::Feature*> choices() const;
    PanelClickGui& gui_;
    Page page_ = 0;
    std::vector<const ui::Feature*> shown_;
    bool builtClient_ = false;
    std::vector<Group> groups_;
    std::string userName_;
};
}
