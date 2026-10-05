#include "utils/Keyboard.h"

#include <array>

namespace nova::input {
namespace {
std::array<bool, 256> keyDown{};
std::array<bool, 256> keyPrevious{};
std::array<bool, 5> mouseDown{};
std::array<bool, 5> mousePrevious{};
std::vector<int> pressedKeys;
POINT cursorPosition{};

bool ValidKey(int key) { return key >= 0 && key < static_cast<int>(keyDown.size()); }
bool ValidMouseButton(int button) { return button >= 0 && button < static_cast<int>(mouseDown.size()); }
}

void Update() {
    keyPrevious = keyDown;
    pressedKeys.clear();
    for (int key = 0; key < static_cast<int>(keyDown.size()); ++key) {
        keyDown[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
        if (keyDown[key] && !keyPrevious[key])
            pressedKeys.push_back(key);
    }

    mousePrevious = mouseDown;
    constexpr int buttons[] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
    for (int i = 0; i < static_cast<int>(mouseDown.size()); ++i)
        mouseDown[i] = (GetAsyncKeyState(buttons[i]) & 0x8000) != 0;

    GetCursorPos(&cursorPosition);
}

void Reset() {
    keyDown.fill(false);
    keyPrevious.fill(false);
    mouseDown.fill(false);
    mousePrevious.fill(false);
    pressedKeys.clear();
    cursorPosition = {};
}

bool IsKeyDown(int virtualKey) { return ValidKey(virtualKey) && keyDown[virtualKey]; }
bool WasKeyPressed(int virtualKey) { return IsKeyDown(virtualKey) && !keyPrevious[virtualKey]; }
bool WasKeyReleased(int virtualKey) { return ValidKey(virtualKey) && !keyDown[virtualKey] && keyPrevious[virtualKey]; }
const std::vector<int>& PressedKeys() { return pressedKeys; }

bool IsMouseButtonDown(MouseButton button) {
    const int index = static_cast<int>(button);
    return ValidMouseButton(index) && mouseDown[index];
}
bool WasMouseButtonPressed(MouseButton button) {
    const int index = static_cast<int>(button);
    return IsMouseButtonDown(button) && !mousePrevious[index];
}
bool WasMouseButtonReleased(MouseButton button) {
    const int index = static_cast<int>(button);
    return ValidMouseButton(index) && !mouseDown[index] && mousePrevious[index];
}
POINT MousePosition() { return cursorPosition; }

bool IsProcessForeground(DWORD processId) {
    const HWND foreground = GetForegroundWindow();
    if (!foreground) return false;
    DWORD foregroundProcess = 0;
    GetWindowThreadProcessId(foreground, &foregroundProcess);
    return foregroundProcess == processId;
}

}
