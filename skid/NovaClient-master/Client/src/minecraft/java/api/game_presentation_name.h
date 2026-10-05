#pragma once
#include <string>
#include <string_view>
namespace nova::java {
// Presentation only; never use as an account/scoreboard/team/winner identity.
std::string PresentationName(std::string_view original);
}
