#include "cs2/sdk/radar/Radar.h"
#include "cs2/sdk/players/Players.h"
#include <windows.h>
#include <cstring>

namespace nova::cs2::sdk::radar {
void* UpdateTarget() {
    if (!players::Available()) return nullptr;
    auto* client = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"client.dll"));
    if (!client) return nullptr;
    constexpr unsigned char entry[]{0x48,0x89,0x4C,0x24,0x08,0x55,0x41,0x54,0x48,0x8D,
        0x6C,0x24,0x88,0x48,0x81,0xEC,0x78,0x01,0x00,0x00};
    __try {
        return std::memcmp(client + updateRva, entry, sizeof(entry)) == 0 ? client + updateRva : nullptr;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
}
