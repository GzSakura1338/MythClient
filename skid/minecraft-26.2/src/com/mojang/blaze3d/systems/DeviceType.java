package com.mojang.blaze3d.systems;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(EnvType.CLIENT)
public enum DeviceType {
	OTHER,
	INTEGRATED,
	DISCRETE,
	VIRTUAL,
	CPU;
}
