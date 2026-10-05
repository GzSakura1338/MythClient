#pragma once
#include "imgui.h"
#include "smtc/MediaSnapshot.h"
#include "ui/hud/musichud/SourceProperty.h"

namespace nova::ui::music {
// Called in the controls window, after OpenPopup("music-source").
void DrawSourcePicker(ImVec2 anchor, float scale, std::string& source,
                      const std::vector<smtc::Source>& sources);
// The settings-row selector shares the exact same menu with the HUD.
void DrawSourceSelector(SourceProperty& source, float scale);
}
