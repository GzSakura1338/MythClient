#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "config/settings/Properties.h"
#include "utils/Json.h"

namespace nova::config {

void SetClientSettings(const std::vector<Properties::PropDesc>* rows);

[[nodiscard]] std::filesystem::path ConfigDirectory();
[[nodiscard]] std::string SanitizeConfigName(std::string_view name);

class Config {
public:
    Config(std::string name, std::filesystem::path file);

    [[nodiscard]] const std::string& name() const { return name_; }
    void setName(std::string name);
    [[nodiscard]] const std::filesystem::path& file() const { return file_; }

    void Save();
    void Load() const;

    bool Read();
    bool Write() const;
    // Write the same captured settings under another file/name without loading
    // it or sampling live module state a second time.
    bool WriteAs(std::string name, std::filesystem::path file) const;

private:
    std::string name_;
    std::filesystem::path file_;
    nova::json::Value data_ = nova::json::Value::Object();
};

}
