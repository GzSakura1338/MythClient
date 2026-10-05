#pragma once
#include "../../../shared/Bootstrap.h"
#include <cstdint>

struct UnwindProbeContext {
    NovaBootstrap boot{NOVA_BOOTSTRAP_MAGIC, sizeof(NovaBootstrap), NOVA_BOOTSTRAP_ABI, NovaClient_Cs2};
    std::uintptr_t image = 0, guardedRead = 0;
};
