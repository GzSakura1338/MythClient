#pragma once

namespace nova::module::antikb {
bool IsEnabled();
bool IsDelaying();
int AttackQueue();
float Yaw();
bool OwnsIncomingDelayQueue();
// Read-only state; AutoBlock owns all output synchronization.
bool BlocksAutoBlockBlink();
bool WatchdogHoldingPackets();
bool WatchdogReplayingPackets();
bool BlocksBacktrack();
}
