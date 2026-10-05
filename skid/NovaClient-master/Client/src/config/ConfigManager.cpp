#include "config/ConfigManager.h"

#include <algorithm>
#include <cctype>
#include <system_error>
#include <utility>

#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include "core/Instance.h"

namespace nova::config {
namespace {

constexpr const char* kExtension = ".json";
constexpr const char* kDefaultName = "default";

bool LessNoCase(const std::string& left, const std::string& right) {
    const std::size_t shared = std::min(left.size(), right.size());
    for (std::size_t i = 0; i < shared; ++i) {
        const auto a = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(left[i])));
        const auto b = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(right[i])));
        if (a != b)
            return a < b;
    }
    return left.size() < right.size();
}

}

ConfigManager& ConfigManager::Get() {
    return Instance<ConfigManager>([] { return new ConfigManager(); });
}

void ConfigManager::Init() {
    if (started_)
        return;
    dir_ = ConfigDirectory();
    if(dir_.empty()) {
        SetStatus("Configuration host has not been installed",notifications::Type::Error);
        return;
    }
    started_ = true;
    std::error_code error;
    std::filesystem::create_directories(dir_, error);
    Refresh();

    if (Find(current_) == nullptr) {
        Config created(current_, PathFor(current_));
        created.Save();
        created.Write();
        configs_.push_back(std::move(created));
        Sort();
    }

    if (Config* config = Find(current_)) {
        if (config->Read())
            config->Load();
    }
    ++loadSerial_;
    SetStatus("Loaded " + current_);
}

void ConfigManager::Refresh() {
    configs_.clear();

    std::error_code error;
    std::filesystem::directory_iterator it(dir_, error);
    const std::filesystem::directory_iterator end;
    while (!error && it != end) {
        const std::filesystem::directory_entry& entry = *it;
        std::error_code probe;
        if (entry.is_regular_file(probe)) {
            const std::filesystem::path path = entry.path();
            if (path.extension().string() == kExtension)
                configs_.emplace_back(path.stem().string(), path);
        }
        it.increment(error);
    }
    Sort();
}

bool ConfigManager::SaveCurrent(bool notify) {
    Config* config = Find(current_);
    if (config == nullptr) {
        const std::string clean = SanitizeConfigName(current_);
        current_ = clean.empty() ? kDefaultName : clean;

        Config created(current_, PathFor(current_));
        created.Save();
        const bool written = created.Write();
        configs_.push_back(std::move(created));
        Sort();
        SetStatus(written ? "Saved " + current_ : std::string("Could not write the config file"),
            written ? notifications::Type::Success : notifications::Type::Error, notify || !written);
        return written;
    }

    config->Save();
    const bool written = config->Write();
    SetStatus(written ? "Saved " + current_ : std::string("Could not write the config file"),
        written ? notifications::Type::Success : notifications::Type::Error, notify || !written);
    return written;
}

bool ConfigManager::SaveForUnload() {
    if(!started_||dir_.empty()) {
        SetStatus("Unload save skipped: configuration is not initialized",notifications::Type::Error,false);
        return false;
    }
    // Capture before the host disables modules, once for both destinations.
    Config snapshot(current_,PathFor(current_));
    snapshot.Save();
    const bool currentWritten=snapshot.Write();
    auto lower=current_;
    for(char& c:lower)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    const bool isDefault=lower==kDefaultName; // Windows paths are case insensitive.
    const bool defaultWritten=isDefault?currentWritten:snapshot.WriteAs(kDefaultName,PathFor(kDefaultName));
    if(currentWritten&&defaultWritten) {
        SetStatus(isDefault?"Saved "+current_:"Saved "+current_+" and default",notifications::Type::Success,false);
    } else {
        std::string failed;
        if(!currentWritten)failed=current_;
        if(!defaultWritten&&!isDefault){if(!failed.empty())failed+=" and ";failed+=kDefaultName;}
        SetStatus("Could not save "+failed+" on unload",notifications::Type::Error,false);
    }
    return currentWritten&&defaultWritten;
}

bool ConfigManager::SaveAs(std::string_view name) {
    const std::string clean = SanitizeConfigName(name);
    if (clean.empty()) {
        SetStatus("That name cannot be used as a file", notifications::Type::Error);
        return false;
    }

    Config created(clean, PathFor(clean));
    created.Save();
    if (!created.Write()) {
        SetStatus("Could not write the config file", notifications::Type::Error);
        return false;
    }

    current_ = clean;
    Refresh();
    SetStatus("Saved " + current_);
    return true;
}

bool ConfigManager::LoadByName(std::string_view name) {
    Config* config = Find(name);
    if (config == nullptr) {
        SetStatus("There is no config by that name", notifications::Type::Error);
        return false;
    }

    if (!config->Read()) {
        SetStatus(config->name() + " is not a readable config", notifications::Type::Error);
        return false;
    }

    config->Load();
    current_ = config->name();
    ++loadSerial_;
    SetStatus("Loaded " + current_);
    return true;
}

bool ConfigManager::Rename(std::string_view from, std::string_view to) {
    Config* config = Find(from);
    if (config == nullptr) {
        SetStatus("There is no config by that name", notifications::Type::Error);
        return false;
    }

    const std::string before = config->name();
    const std::string clean = SanitizeConfigName(to);
    if (clean.empty()) {
        SetStatus("That name cannot be used as a file", notifications::Type::Error);
        return false;
    }
    if (clean == before)
        return false;
    if (Find(clean) != nullptr) {
        SetStatus(clean + " already exists", notifications::Type::Error);
        return false;
    }

    const std::filesystem::path target = PathFor(clean);
    std::error_code error;
    std::filesystem::rename(config->file(), target, error);
    if (error) {
        SetStatus("Could not rename the config file", notifications::Type::Error);
        return false;
    }

    Config moved(clean, target);
    if (moved.Read()) {
        moved.setName(clean);
        moved.Write();
    }

    Refresh();
    if (current_ == before)
        current_ = clean;
    SetStatus("Renamed to " + clean);
    return true;
}

bool ConfigManager::Delete(std::string_view name) {
    Config* config = Find(name);
    if (config == nullptr) {
        SetStatus("There is no config by that name", notifications::Type::Error);
        return false;
    }

    const std::string removed = config->name();
    std::error_code error;
    std::filesystem::remove(config->file(), error);
    if (error) {
        SetStatus("Could not delete the config file", notifications::Type::Error);
        return false;
    }

    Refresh();
    if (removed == current_) {
        current_ = configs_.empty() ? kDefaultName : configs_.front().name();
        if (Config* next = Find(current_); next != nullptr && next->Read()) {
            next->Load();
            ++loadSerial_;
        }
    }
    SetStatus("Deleted " + removed);
    return true;
}

bool ConfigManager::Duplicate(std::string_view name) {
    Config* config = Find(name);
    if (config == nullptr) {
        SetStatus("There is no config by that name", notifications::Type::Error);
        return false;
    }

    const std::string source = config->name();
    std::string copy;
    for (int suffix = 1; suffix < 100 && copy.empty(); ++suffix) {
        std::string candidate = suffix == 1 ? source + " copy"
                                            : source + " copy " + std::to_string(suffix);
        if (Find(candidate) == nullptr)
            copy = std::move(candidate);
    }
    if (copy.empty()) {
        SetStatus("Too many copies of " + source, notifications::Type::Error);
        return false;
    }

    std::error_code error;
    std::filesystem::copy_file(config->file(), PathFor(copy), error);
    if (error) {
        SetStatus("Could not copy the config file", notifications::Type::Error);
        return false;
    }

    Config created(copy, PathFor(copy));
    if (created.Read()) {
        created.setName(copy);
        created.Write();
    }

    Refresh();
    SetStatus("Duplicated " + source + " as " + copy);
    return true;
}

void ConfigManager::OpenFolder() const {
    std::error_code error;
    std::filesystem::create_directories(dir_, error);
    const std::string path = dir_.string();
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

Config* ConfigManager::Find(std::string_view name) {
    for (Config& config : configs_) {
        if (config.name() == name)
            return &config;
    }
    return nullptr;
}

std::filesystem::path ConfigManager::PathFor(std::string_view name) const {
    return dir_ / (std::string(name) + kExtension);
}

void ConfigManager::Sort() {
    std::sort(configs_.begin(), configs_.end(), [](const Config& left, const Config& right) {
        return LessNoCase(left.name(), right.name());
    });
}

void ConfigManager::SetStatus(std::string text, notifications::Type type, bool notify) {
    status_ = std::move(text);
    ++statusSerial_;
    if (notify) notifications::Post(status_, type, notifications::Source::Config);
}

}
