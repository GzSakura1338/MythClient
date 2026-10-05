#pragma once
#include "imgui.h"
namespace nova::menu::panel { class PanelClickGui; }
namespace nova::ui::accounts { void Draw(menu::panel::PanelClickGui&, ImVec2 origin); void Hide(); }
