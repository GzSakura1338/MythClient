#pragma once
#include <filesystem>
#include "ui/backend/Dx11Overlay.h"

namespace nova::cs2::presentation {
// Called before hooks are enabled. Tests use an isolated temporary directory.
void Configure(std::filesystem::path directory, bool preview = false);
nova::ui::Dx11Callbacks Callbacks();
}
