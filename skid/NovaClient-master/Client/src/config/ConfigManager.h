#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "config/Config.h"
#include "ui/notifications/Notifications.h"

namespace nova::config {

class ConfigManager {
public:
    static ConfigManager& Get();

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void Init();
    void Refresh();

    bool SaveCurrent(bool notify = true);
    bool SaveForUnload();
    bool SaveAs(std::string_view name);
    bool LoadByName(std::string_view name);
    bool Rename(std::string_view from, std::string_view to);
    bool Delete(std::string_view name);
    bool Duplicate(std::string_view name);

    void OpenFolder() const;

    [[nodiscard]] const std::vector<Config>& All() const { return configs_; }
    [[nodiscard]] const std::string& CurrentName() const { return current_; }
    [[nodiscard]] const std::filesystem::path& Directory() const { return dir_; }

    [[nodiscard]] const std::string& Status() const { return status_; }
    [[nodiscard]] unsigned int StatusSerial() const { return statusSerial_; }
    [[nodiscard]] unsigned int LoadSerial() const { return loadSerial_; }

private:
    ConfigManager() = default;

    [[nodiscard]] Config* Find(std::string_view name);
    [[nodiscard]] std::filesystem::path PathFor(std::string_view name) const;
    void Sort();
    void SetStatus(std::string text, notifications::Type type = notifications::Type::Success, bool notify = true);

    std::filesystem::path dir_;
    std::vector<Config> configs_;
    std::string current_ = "default";
    std::string status_;
    unsigned int statusSerial_ = 0;
    unsigned int loadSerial_ = 0;
    bool started_ = false;
};

}
