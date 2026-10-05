#pragma once
#include <cstdint>

namespace nova {
struct HostContext {
    std::uint32_t profile=0;
    std::uint32_t loaderThread=0;
    bool reflective=false;
};
struct GameHost {
    const char* game;
    const char* label;
    unsigned long (*run)(HostContext context);
};
}
