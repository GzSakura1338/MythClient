#pragma once

#include "utils/Json.h"

namespace nova::ui {

void PlaceWindow(const char* id, float fallbackX, float fallbackY);

void PlaceWindowCentered(const char* id, float width, float height);

void CaptureWindow(const char* id);

bool WindowPlacementsNeedFlush();

nova::json::Value SaveWindowPlacements();

void LoadWindowPlacements(const nova::json::Value& in);

}
