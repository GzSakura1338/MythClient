#pragma once
#include <windows.h>

namespace nova::bind {
// BindProperty stores mouse buttons as -1..-5, keyboard keys as Win32 VKs.
constexpr int BindingVirtualKey(int binding) {
    constexpr int mouseKeys[]{VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
    if (binding >= -5 && binding < 0) return mouseKeys[-binding - 1];
    return binding > 0 && binding < 256 ? binding : 0;
}

inline bool IsPhysicalBindingDown(int binding) {
    const int key = BindingVirtualKey(binding);
    return key != 0 && (GetAsyncKeyState(key) & 0x8000) != 0;
}
}
