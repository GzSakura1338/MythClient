#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "imgui.h"
#include <windows.h>
namespace nova::menu::panel { class PanelClickGui; }
namespace nova::ui::workspace {
enum class State { Embedded, Foreground, Shelved };
State Current();
bool Detached();
bool CapturesInput();
// UI-thread transitions; input-thread requests are drained by Draw().
void Detach();
void Restore();
void Shelve();
void Dock();
void Draw(menu::panel::PanelClickGui&);
void PrepareFrame();
void ShelvePanel();
void RestorePanel();
bool PanelHidden();
bool PanelShelved();
void CapturePanel();
bool WindowMessage(UINT message, WPARAM key, LPARAM flags);
void Reset();
void ReleaseTextures();
}
