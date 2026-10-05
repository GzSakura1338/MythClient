#pragma once
#include "cs2/sdk/chams/Chams.h"
namespace nova::cs2::hooks::chams {
bool Install(sdk::chams::Binding);
bool Available();
void Publish(sdk::chams::Settings);
bool Stop(unsigned timeoutMs);
void Finalize();
}
