#pragma once
#include <algorithm>
#include <cmath>

namespace nova::module::freelook {
struct Settings {
    bool enabled = false, hold = true, invertPitch = false, lockPitch = true, customFov = false;
    int key = 0xA4; // Nova bindings use Win32 VK_LMENU, Raven uses LWJGL 56.
    float fov = 90;
};
struct Input {
    bool down = false, ready = false, focused = false, screen = false;
};
struct State {
    bool active = false, lastKey = false, changedFov = false;
    int previousView = 0, key = 0xA4;
    float yaw = 0, pitch = 0, savedFov = 70;

    template<class IO> void Stop(IO& io) {
        if (active) {
            io.SetView(previousView);
            if (changedFov) io.SetFov(savedFov);
        }
        active = changedFov = false;
    }
    template<class IO> void Tick(const Settings& s, Input in, IO& io) {
        if (s.key != key) { Stop(io); key = s.key; lastKey = in.down; }
        if (!s.enabled || !in.ready) { Stop(io); lastKey = in.down; return; }
        if (in.screen || !in.focused) {
            // Hold cannot become stuck when the release occurs in a menu.
            if (s.hold || !in.focused) Stop(io);
            lastKey = in.down;
            return;
        }
        if (in.down && !lastKey) {
            if (active) Stop(io);
            else if (io.Capture(previousView, savedFov, yaw, pitch)) active = io.SetView(1);
        }
        if (s.hold && !in.down) Stop(io);
        lastKey = in.down;
        if (active) {
            if (s.customFov) {
                if (io.SetFov(std::clamp(s.fov, 10.f, 150.f))) changedFov = true;
            } else if (changedFov && io.SetFov(savedFov)) changedFov = false;
        }
    }
    // dx/dy already contain sensitivity scaling; positive dy follows Raven's
    // MouseHelper.deltaY convention, independent of Minecraft's invert option.
    void Turn(const Settings& s, float dx, float dy) {
        if (!active || !std::isfinite(dx) || !std::isfinite(dy)) return;
        yaw += dx * .15f;
        pitch += (s.invertPitch ? -dy : dy) * .15f;
        if (s.lockPitch) pitch = std::clamp(pitch, -90.f, 90.f);
    }
};
}
