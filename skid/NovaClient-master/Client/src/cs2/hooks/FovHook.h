#pragma once
#include "cs2/modules/render/fovcontrol/FovSettings.h"
#include "cs2/sdk/camera/Camera.h"

namespace nova::cs2::hooks::fov {
// MinHook lifetime is owned by Dx11Hooks. Production bindings are version gated.
bool Install(sdk::camera::Binding);
bool Available();
void Publish(cs2::fov::Settings, bool enabled);
bool Stop(unsigned timeoutMs);
void Finalize();
}
