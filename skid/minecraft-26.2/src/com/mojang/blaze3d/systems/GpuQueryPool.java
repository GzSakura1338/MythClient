package com.mojang.blaze3d.systems;

import java.util.OptionalLong;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(EnvType.CLIENT)
public interface GpuQueryPool extends AutoCloseable {
	int size();

	OptionalLong getValue(int index);

	OptionalLong[] getValues(int index, int count);

	@Override
	void close();
}
