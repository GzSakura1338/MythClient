#pragma once
#include <string>
namespace nova::minecraft::accounts {
std::string ProtectCredentials(const std::string&);
std::string UnprotectCredentials(const std::string&);
}
