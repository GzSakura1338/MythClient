package com.mojang.blaze3d.vulkan.glsl;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(EnvType.CLIENT)
record SpvSampler(String name, int bindingOffset, int dimensions) {
}
