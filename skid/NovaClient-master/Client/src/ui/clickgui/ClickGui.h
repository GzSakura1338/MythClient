#pragma once

namespace nova::clickgui {
void OpenBrowser();
void OpenPanel();

enum class Mode : int {
    Panel = 0,
    Classic,
};

Mode GetMode();
void SetMode(Mode mode);

bool IsOpen();
bool CapturesInput(); // ClickGUI or an independently focused foreground window.
void SetOpen(bool open);
void Toggle();

void Close();
// The host owns the feature toggle. Register before rendering, clear after the
// last UI callback; a standalone UI does not require a close handler.
void SetCloseHandler(void (*handler)());
void SetOpenHandler(void (*handler)());

void Render();

}
