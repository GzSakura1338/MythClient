#pragma once

namespace nova::module::autoblock {
// Called on the client thread before output and after input/tick dispatch.
// Only Slinky/Hypixel output is coordinated; other owners are untouched.
void SynchronizePacketDelays(bool discard = false);
}
