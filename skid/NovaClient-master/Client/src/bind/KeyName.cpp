#include "bind/KeyName.h"

#include <cstddef>
#include <iterator>

#include <windows.h>

namespace nova::bind {
namespace {

std::string Narrow(const wchar_t* text, int length) {
    if (length <= 0)
        return {};

    const int size = WideCharToMultiByte(CP_UTF8, 0, text, length, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return {};

    std::string out(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, length, out.data(), size, nullptr, nullptr);
    return out;
}

std::string VirtualKeyName(UINT virtualKey) {
    const UINT scan = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);
    if (scan == 0)
        return {};

    LONG param = static_cast<LONG>(scan << 16);
    switch (virtualKey) {
        case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
        case VK_PRIOR: case VK_NEXT: case VK_LEFT: case VK_RIGHT:
        case VK_UP: case VK_DOWN: case VK_NUMLOCK: case VK_DIVIDE:
        case VK_RCONTROL: case VK_RMENU:
            param |= 1 << 24;
            break;
        default:
            break;
    }

    wchar_t buffer[64]{};
    return Narrow(buffer, GetKeyNameTextW(param, buffer, static_cast<int>(std::size(buffer))));
}

}

std::string KeyName(int keyCode) {
    if (keyCode == 0)
        return "None";

    if (keyCode < 0) {
        const int index = -keyCode;
        if (index == 1)
            return "LMB";
        if (index == 2)
            return "RMB";
        return "M" + std::to_string(index);
    }

    if (keyCode > 255)
        return "Key " + std::to_string(keyCode);

    const std::string name = VirtualKeyName(static_cast<UINT>(keyCode));
    return name.empty() ? "Key " + std::to_string(keyCode) : name;
}

}
