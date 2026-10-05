#pragma once
#include "config/settings/Properties.h"
namespace nova::config {
json::Value SaveRows(const std::vector<Properties::PropDesc>& rows);
void LoadRows(const json::Value& in,const std::vector<Properties::PropDesc>& rows);
}
