#include "minecraft/java/api/game_chest.h"

#include <cmath>
#include <mutex>

namespace nova::java {
namespace {
std::mutex cameraMutex;
CameraView camera;
bool valid = false;
}

void InvalidateWorldCamera() {
    std::lock_guard lock(cameraMutex);
    valid = false;
}

void PublishWorldCamera(const CameraView& view) {
    std::lock_guard lock(cameraMutex);
    // A failed pass must never leave the preceding frame visible.
    valid = false;
    if (!view.hasModelView || !std::isfinite(view.x) || !std::isfinite(view.y) ||
        !std::isfinite(view.z) || view.windowWidth <= 0 || view.windowHeight <= 0)
        return;
    for (int i = 0; i < 16; ++i)
        if (!std::isfinite(view.projection[i]) || !std::isfinite(view.modelView[i])) return;
    if (view.projection[0] == 0 || view.projection[5] == 0) return;
    camera = view;
    valid = true;
}

bool ReadWorldCamera(CameraView& out) {
    std::lock_guard lock(cameraMutex);
    if (!valid) return false;
    out = camera;
    return true;
}

void JNICALL ResetWorldCameraHook(JNIEnv*, jclass) { InvalidateWorldCamera(); }
}
