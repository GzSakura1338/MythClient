#pragma once
#include "minecraft/java/api/base.h"
#include <string>
#include <vector>

namespace nova::java {
// Reads the server's replay prompt without cancelling or retaining its packet.
bool ReadAutoPlayCommands(jobject packet, std::vector<std::string>& commands);
bool IsChatSessionStart(jobject packet);
// Game-thread only. Uses the normal outbound path, including packet listeners.
bool SendPlayCommand(const std::string& command);
}
