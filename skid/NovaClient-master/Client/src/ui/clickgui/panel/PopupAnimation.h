#pragma once
#include <memory>
#include <vector>
#include "imgui.h"

struct ImGuiWindow;
namespace nova::menu::panel {
enum class PopupMotion { Dropdown, Account };
// Retain geometry only while native popups close. Input remains owned by ImGui.
class PopupAnimations {
public:
    void reset();
    void capture(ImGuiWindow*, float reveal, PopupMotion);
    void drawClosing();
private:
    struct Entry {
        ImGuiID id{};
        int frame{};
        float reveal{};
        ImVec2 position{}, size{};
        PopupMotion motion{};
        std::vector<std::shared_ptr<ImDrawList>> lists;
    };
    std::vector<Entry> entries_;
};
void SetPopupAnimations(PopupAnimations*);
void CapturePopupAnimation(ImGuiWindow*, float, PopupMotion);
void TransformPopup(ImDrawList*, ImVec2 position, ImVec2 size, float reveal, PopupMotion);
}
