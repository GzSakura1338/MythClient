#pragma once
#include "ui/backend/Dx11Overlay.h"

namespace nova::cs2::hooks {
bool Install(ui::Dx11Callbacks callbacks);
bool Ready();
bool Failed();
// Stops callbacks and detaches the window on a render frame; no force-unmap.
bool Stop(unsigned timeoutMs);
// Call only after Stop succeeds and execution has drained out of this image.
void Finalize();
}
