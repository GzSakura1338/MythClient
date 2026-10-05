#pragma once

#include "imgui.h"

// windows.h defines DrawText as a macro for its own A/W pair of text-drawing
// functions, and jni.h drags windows.h in behind it, so by the time this header is
// reached the name may already be a macro and the declarations below would be
// renamed to DrawTextA. Undefining it here fixes it once for every file that uses
// the helper, rather than making each of them order its includes around it.
#ifdef DrawText
#undef DrawText
#endif

namespace nova::text {

ImU32 Dimmed(ImU32 color);

ImU32 ColorByIndex(int index);

float TextWidth(const char* text);

float TextHeight();

float ShadowOffset();

void DrawTextRun(ImDrawList* list, const char* begin, const char* end,
                 float x, float y, ImU32 color, ImU32 shadowColor, bool bold);

void DrawText(ImDrawList* list, const char* text, float x, float y, ImU32 color, bool shadow);

ImVec2 CenterText(const char* text, float x, float y, bool horizontal, bool vertical);

}
