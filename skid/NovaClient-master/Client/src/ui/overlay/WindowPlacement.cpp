#include "ui/overlay/WindowPlacement.h"

#include <cmath>
#include <string>
#include <unordered_map>

#include "core/Instance.h"
#include "imgui.h"

// Where the client's floating windows sit, kept as a fraction of the display rather
// than as pixels so that it means the same thing on a screen of any size.
//
// The fraction is the one thing about a placement that is meant to survive a resize.
// ImGui keeps a window's position in absolute pixels and this only ever sets one
// once -- setting it every frame would fight the player dragging the window -- so a
// display that changes size under the overlays leaves every one of them where it was
// on the old display: a menu that was in the middle is off to one side, and a hud
// pinned to the right edge is somewhere in the middle. Reflow is what notices and
// works them out again.

namespace nova::ui {
namespace {

constexpr double kFlushDelay = 1.0;

struct Entry {
    float  fractionX = 0.0f;
    float  fractionY = 0.0f;
    bool   known = false;
    bool   placed = false;
    bool   tracked = false;

    // The player has put this window somewhere themselves. A menu they moved stays
    // where they put it across a resize; one they never touched goes back to the
    // middle of the display it is now on.
    bool   moved = false;

    // Placed by PlaceWindowCentered, with the size it was centred at.
    bool   centered = false;
    float  width = 0.0f;
    float  height = 0.0f;

    ImVec2 lastPos = ImVec2(0.0f, 0.0f);
};

bool g_dirty = false;
double g_lastChange = 0.0;

// The display every placement was last worked out against.
ImVec2 g_appliedDisplay = ImVec2(-1.0f, -1.0f);

std::unordered_map<std::string, Entry>& Store() {
    return Instance<std::unordered_map<std::string, Entry>>();
}

float Clamp01(float value) {
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

bool SamePos(const ImVec2& a, const ImVec2& b) {
    return std::fabs(a.x - b.x) < 0.5f && std::fabs(a.y - b.y) < 0.5f;
}

void MarkDirty() {
    g_dirty = true;
    g_lastChange = ImGui::GetTime();
}

void Center(Entry& entry, const ImVec2& display) {
    if (display.x <= 0.0f || display.y <= 0.0f)
        return;
    entry.fractionX = Clamp01((display.x - entry.width) * 0.5f / display.x);
    entry.fractionY = Clamp01((display.y - entry.height) * 0.5f / display.y);
    entry.known = true;
}

// A display that changed size under the overlays. Every placement is worked out
// again from its fraction on the next Apply, which is the only thing about it that
// is meant to survive a resize.
void Reflow(const ImVec2& display) {
    if (display.x <= 0.0f || display.y <= 0.0f)
        return;
    if (display.x == g_appliedDisplay.x && display.y == g_appliedDisplay.y)
        return;
    g_appliedDisplay = display;

    for (auto& item : Store()) {
        Entry& entry = item.second;
        if (entry.centered && !entry.moved) {
            const float beforeX = entry.fractionX;
            const float beforeY = entry.fractionY;
            Center(entry, display);
            if (entry.fractionX != beforeX || entry.fractionY != beforeY)
                MarkDirty();
        }
        entry.placed = false;
    }
}

void Apply(Entry& entry) {
    if (entry.placed || !entry.known)
        return;

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    if (display.x <= 0.0f || display.y <= 0.0f)
        return;

    entry.placed = true;
    // The next capture follows an automatic placement, even if a slider is held.
    entry.tracked = false;
    ImGui::SetNextWindowPos(
        ImVec2(entry.fractionX * display.x, entry.fractionY * display.y));
}

}

void PlaceWindow(const char* id, float fallbackX, float fallbackY) {
    if (id == nullptr)
        return;

    Reflow(ImGui::GetIO().DisplaySize);

    Entry& entry = Store()[id];
    if (!entry.known) {
        entry.fractionX = Clamp01(fallbackX);
        entry.fractionY = Clamp01(fallbackY);
        entry.known = true;
    }
    Apply(entry);
}

void PlaceWindowCentered(const char* id, float width, float height) {
    if (id == nullptr)
        return;

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    Reflow(display);

    Entry& entry = Store()[id];
    const bool resized = entry.width != width || entry.height != height;
    entry.centered = true;
    entry.width = width;
    entry.height = height;
    if (!entry.known || (!entry.moved && (resized || !entry.placed))) {
        Center(entry, display);
        entry.placed = false;
    }
    if (resized && entry.moved && display.x > 0.0f && display.y > 0.0f) {
        // Keep a manually placed menu visible when its new size reaches an edge.
        const float maxX = display.x > width ? (display.x - width) / display.x : 0.0f;
        const float maxY = display.y > height ? (display.y - height) / display.y : 0.0f;
        if (entry.fractionX > maxX || entry.fractionY > maxY) {
            entry.fractionX = entry.fractionX > maxX ? maxX : entry.fractionX;
            entry.fractionY = entry.fractionY > maxY ? maxY : entry.fractionY;
            MarkDirty();
        }
        entry.placed = false;
    }
    Apply(entry);
}

void CaptureWindow(const char* id) {
    if (id == nullptr)
        return;

    Entry& entry = Store()[id];
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    if (display.x <= 0.0f || display.y <= 0.0f)
        return;

    const ImVec2 pos = ImGui::GetWindowPos();
    if (!entry.tracked) {
        entry.tracked = true;
        entry.lastPos = pos;
        return;
    }
    if (SamePos(pos, entry.lastPos))
        return;
    entry.lastPos = pos;

    // A window that moved with the mouse up moved for a reason that was not the
    // player's: the display changed size under it, or ImGui pulled it back into
    // view. Recording that as a new resting place is what made a placement walk a
    // little further towards a corner on every resize, because each one saved the
    // position it had been pushed to as if it had been chosen.
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        return;

    entry.fractionX = Clamp01(pos.x / display.x);
    entry.fractionY = Clamp01(pos.y / display.y);
    entry.moved = true;
    entry.known = true;
    MarkDirty();
}

bool WindowPlacementsNeedFlush() {
    if (!g_dirty || ImGui::GetCurrentContext() == nullptr)
        return false;
    if (ImGui::GetTime() - g_lastChange < kFlushDelay)
        return false;
    g_dirty = false;
    return true;
}

nova::json::Value SaveWindowPlacements() {
    nova::json::Value out = nova::json::Value::Array();
    for (const auto& item : Store()) {
        if (!item.second.known)
            continue;
        nova::json::Value entry = nova::json::Value::Object();
        entry["id"] = item.first;
        entry["x"] = static_cast<double>(item.second.fractionX);
        entry["y"] = static_cast<double>(item.second.fractionY);
        entry["moved"] = item.second.moved;
        out.Push(std::move(entry));
    }
    return out;
}

void LoadWindowPlacements(const nova::json::Value& in) {
    if (!in.isArray())
        return;

    for (const nova::json::Value& item : in.Items()) {
        if (!item.isObject())
            continue;
        const nova::json::Value* id = item.Find("id");
        const nova::json::Value* x = item.Find("x");
        const nova::json::Value* y = item.Find("y");
        if (id == nullptr || x == nullptr || y == nullptr)
            continue;

        Entry& entry = Store()[id->asString()];
        entry.fractionX = Clamp01(static_cast<float>(x->asNumber(0.0)));
        entry.fractionY = Clamp01(static_cast<float>(y->asNumber(0.0)));
        // Absent in a file written before the flag existed, and a placement that was
        // never moved is the right reading of that: a menu comes back to the middle
        // of whatever display it finds itself on.
        const nova::json::Value* moved = item.Find("moved");
        entry.moved = moved != nullptr && moved->asBool(false);
        entry.known = true;
        entry.placed = false;
        entry.tracked = false;
    }
}

}
