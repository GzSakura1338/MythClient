#pragma once

#include <windows.h>

namespace nova::selfunload {

bool ImageIsClear(HMODULE image, DWORD selfThreadId);

bool UnmapAndExit(HMODULE image);

}
