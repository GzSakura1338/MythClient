package com.mojang.blaze3d.vulkan.glsl;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(EnvType.CLIENT)
record SpvVariable(String name, int locationOffset) {
}
