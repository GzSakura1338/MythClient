package com.mojang.blaze3d.platform;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(EnvType.CLIENT)
public enum BlendOp {
	ADD,
	SUBTRACT,
	REVERSE_SUBTRACT,
	MIN,
	MAX;
}
