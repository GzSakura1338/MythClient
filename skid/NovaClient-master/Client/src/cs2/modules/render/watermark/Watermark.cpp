#include "cs2/modules/render/watermark/Watermark.h"
#include <windows.h>
#include <cmath>
#include <format>

namespace nova::cs2::modules {
Watermark::Watermark() : Module("Watermark", "render", "HUD", true, true) {
    addSettings(hud_.Options().Rows());
}
void Watermark::onEnable() { data_ = {}; ticks_.Reset(); nextSample_ = 0; }
void Watermark::onUpdate() {
    const double now = GetTickCount64() / 1000.;
    if (now < nextSample_) return;
    nextSample_ = now + .25;
    data_ = {}; data_.version = "CS2"; data_.hideUnavailable = true; data_.speedUnit = "u/s";
    sdk::hud::Snapshot snapshot;
    if (!sdk::hud::Capture(snapshot)) { ticks_.Reset(); return; }
    data_.player = snapshot.player; data_.server = snapshot.server; data_.ping = snapshot.ping;
    data_.tps = ticks_.Update(snapshot, now); data_.speed = snapshot.speed;
    if (snapshot.hasPosition) data_.coordinates = std::format("{:.0f} / {:.0f} / {:.0f}",
        std::floor(snapshot.position.x), std::floor(snapshot.position.y), std::floor(snapshot.position.z));
}
void Watermark::onRender() { hud_.Draw(isEnabled(), data_); }
}
