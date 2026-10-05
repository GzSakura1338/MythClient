#pragma once

#include <string>

#include "ui/clickgui/panel/PanelComponent.h"

namespace nova::menu::panel {

class TopBar final : public PanelComponent {
public:
    explicit TopBar(PanelClickGui& gui);

    void render() override;

private:
    struct MenuAction {
        int renameIndex = -1;
        std::string targetName;
        bool overwrite = false;
        bool duplicate = false;
        bool remove = false;
        bool refresh = false;
        bool openFolder = false;
    };

    void renderTitle();
    void renderSelector();
    void renderSelectorPopup();
    void renderConfigList();
    void renderConfigRow(int index, MenuAction& action);
    void renderCreateRow();
    void renderEditRow();
    void renderStatus();
    void renderSearch();

    bool configInput(const char* id, const char* hint, char* buffer, std::size_t size,
                     float width);
    void beginRename(int index);
    void commitRename();
    void cancelEdit();

    char nameBuffer_[64] = {};
    char createBuffer_[64] = {};
    char searchBuffer_[64] = {};
    int searchFrame_ = -2;
    float searchReveal_ = 0.0f, searchHighlight_ = 0.0f, searchFocus_ = 0.0f;
    bool searchOpen_ = false;
    bool focusSearch_ = false;

    int renaming_ = -1;
    bool focusEdit_ = false;

    float barOriginX_ = 0.0f;
    float barOriginY_ = 0.0f;
    float barRight_ = 0.0f;
    float barCenterY_ = 0.0f;

    unsigned int seenStatus_ = 0;
    double statusAt_ = -1000.0;
};

}
