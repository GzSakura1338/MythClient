#pragma once
#include "ui/overlay/Portrait.h"

namespace nova::minecraft {
inline ui::Portrait PlayerPortrait(std::uint64_t texture) {
    ui::Portrait result;
    result.texture=texture;result.nearest=true;result.layerCount=2;
    result.layers[0]={{8.f/64,8.f/64},{16.f/64,16.f/64}};
    result.layers[1]={{40.f/64,8.f/64},{48.f/64,16.f/64}};
    return result;
}
}
