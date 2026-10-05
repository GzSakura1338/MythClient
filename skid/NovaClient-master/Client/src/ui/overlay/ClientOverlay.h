#pragma once

#include "imgui.h"

namespace nova::overlay {

class ClientOverlay {
public:
    virtual ~ClientOverlay() = default;

    ClientOverlay(const ClientOverlay&) = delete;
    ClientOverlay& operator=(const ClientOverlay&) = delete;

    void Render(bool input);

protected:
    ClientOverlay() = default;

    struct Placement {
        ImVec2 pos;
        ImVec2 offset;

        // The overlay window itself. `pos` is the corner ClientOverlay picked for
        // the content to hang off, which is the window's left edge on one half of
        // the screen and its right edge on the other; a subclass that wants an
        // anchor of its own needs the edges rather than that corner.
        ImVec2 min;
        ImVec2 max;

        // Both vertical anchors, for the same reason: `pos.y` is one of these
        // depending on which half of the screen the window is on, and a subclass
        // that wants the other one should not have to rebuild the title height and
        // the gap to find it. Growing down starts at `downY`, growing up starts at
        // `upY` with the first row's bottom on it.
        float downY = 0.0f;
        float upY = 0.0f;
    };

    [[nodiscard]] virtual const char* OverlayId() const = 0;
    [[nodiscard]] virtual const char* OverlayName() const = 0;

    [[nodiscard]] virtual bool IsVisible() const { return true; }

    [[nodiscard]] virtual bool IsTitleAlwaysVisible() const { return false; }

    virtual void OnDraw(const Placement& placement, ImDrawList* draws) = 0;

    [[nodiscard]] virtual float ContentWidth() const { return 0.0f; }
    [[nodiscard]] virtual ImVec2 DefaultPosition() const { return ImVec2(0.94f, 0.03f); }

private:
    Placement PreRender(bool input);
    double lastTitleFrame_ = -1.0;
    float titleProgress_ = 0.0f;
};

}
