#pragma once
#include "minecraft/host/input/ImeInput.h"

namespace nova::module::imefix {
// Called on the original HWND thread, before bind/GLFW dispatch; never replay
// IME composition messages later on the render thread.
bool WindowMessage(HWND window, UINT message, WPARAM wp, LPARAM lp);
}
