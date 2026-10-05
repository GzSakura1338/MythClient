#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "chrome/ipc/Protocol.h"
#include <memory>
#include <vector>
#include <windows.h>

namespace nova::chrome {
struct Frame { uint64_t sequence{}; int width{},height{}; std::vector<unsigned char> rgba; };
struct Snapshot { bool running{},starting{}; Status status{}; std::shared_ptr<const Frame> frame; std::string error; };
// Optional portable data root; runtime and profiles both stay below this path.
// Empty preserves the normal per-user installation location.
void Start(std::wstring dataRoot = {});
void Stop();                       // Non-blocking, may be used by UI.
void Shutdown();                   // Join off render thread before unloading DLL.
Snapshot Read();
void Send(Command command);
void Navigate(std::wstring url);
void Viewport(int width,int height);
void Visible(bool value);
void Focus(bool value);
bool Focused();
bool TakeAddressFocus();
// Called on the HWND owner thread, before normal key/bind routing. Mouse input
// is supplied by BrowserPage; only keyboard/IME are consumed here.
bool WindowMessage(HWND window,UINT message,WPARAM w,LPARAM l);
void InputRegion(float x,float y,float width,float height,int pixelsW,int pixelsH);
std::wstring Wide(std::string_view utf8);
std::string Utf8(std::wstring_view wide);
}
