#pragma once
#include "cs2/sdk/environment/Environment.h"
namespace nova::cs2::hooks::time {
bool Install(sdk::environment::Binding);
bool Available();
void Publish(sdk::environment::Settings, bool enabled);
bool Stop(unsigned timeoutMs);
void Finalize();
}
