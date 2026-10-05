#pragma once
#include <filesystem>
#include "utils/Json.h"

namespace nova::config {
struct HostConfig {
    std::filesystem::path directory;
    void (*save)(json::Value& root)=nullptr;
    void (*load)(const json::Value& root)=nullptr;
};
// Install on the startup thread, before ConfigManager::Init. The callbacks
// execute synchronously on the existing configuration caller's thread.
bool InstallHostConfig(HostConfig host);
}
