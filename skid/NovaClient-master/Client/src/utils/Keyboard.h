#pragma once

#include <vector>

#include <windows.h>

namespace nova::input {

enum class MouseButton : int {
    Left = 0,
    Right,
    Middle,
    X1,
    X2,
};

void Update();
void Reset();

bool IsKeyDown(int virtualKey);
bool WasKeyPressed(int virtualKey);
bool WasKeyReleased(int virtualKey);

const std::vector<int>& PressedKeys();

bool IsMouseButtonDown(MouseButton button);
bool WasMouseButtonPressed(MouseButton button);
bool WasMouseButtonReleased(MouseButton button);
POINT MousePosition();

bool IsProcessForeground(DWORD processId = GetCurrentProcessId());

}
