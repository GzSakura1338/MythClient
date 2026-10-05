#pragma once
namespace nova::runtime {
bool AcquireInstance();
void ReleaseInstance();
// The host owns this Win32 event and clears it before closing the handle.
void SetUnloadEvent(void* event);
void SetLogPath(const char* path);
}
