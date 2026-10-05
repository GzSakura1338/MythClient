#pragma once
#include "smtc/MediaSnapshot.h"
#include "smtc/MediaCommand.h"
namespace nova::smtc {
void Start();
void SelectSource(std::string source); // Empty / Auto follows Windows current session.
void SetLyricsEnabled(bool enabled);
std::shared_ptr<const MediaSnapshot> ReadSnapshot();
bool Enqueue(MediaCommand command);
void RequestStop();
bool WaitStopped(unsigned timeoutMs);
// Permanent stop gate used before unmapping the DLL.
void BeginShutdown();
}
