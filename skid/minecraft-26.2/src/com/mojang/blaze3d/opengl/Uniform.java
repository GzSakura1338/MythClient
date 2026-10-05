package com.mojang.blaze3d.opengl;

import com.mojang.blaze3d.GpuFormat;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(EnvType.CLIENT)
public sealed interface Uniform extends AutoCloseable permits Uniform.Sampler, Uniform.Ubo, Uniform.Utb {
	@Override
	default void close() {
	}

	@Environment(EnvType.CLIENT)
	record Sampler(int location, int samplerIndex) implements Uniform {
	}

	@Environment(EnvType.CLIENT)
	record Ubo(int blockBinding) implements Uniform {
	}

	@Environment(EnvType.CLIENT)
	record Utb(int location, int samplerIndex, GpuFormat format, int texture) implements Uniform {
		public Utb(final int location, final int samplerIndex, final GpuFormat format) {
			this(location, samplerIndex, format, GlStateManager._genTexture());
		}

		@Override
		public void close() {
			GlStateManager._deleteTexture(this.texture);
		}
	}
}
