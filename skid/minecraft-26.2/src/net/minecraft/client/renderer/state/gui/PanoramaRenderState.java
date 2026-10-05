package net.minecraft.client.renderer.state.gui;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.fabricmc.fabric.api.client.rendering.v1.FabricRenderState;

@Environment(EnvType.CLIENT)
public record PanoramaRenderState(float spin) implements FabricRenderState {
}
