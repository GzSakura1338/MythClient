#pragma once
namespace nova::minecraft::accounts {
void Start();
void BeginShutdown();
bool WaitStopped(unsigned milliseconds);
}
