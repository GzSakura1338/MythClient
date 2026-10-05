package net.minecraft.client.renderer.state;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.fabricmc.fabric.api.client.rendering.v1.FabricRenderState;

@Environment(EnvType.CLIENT)
public class WindowRenderState implements FabricRenderState {
	public int width;
	public int height;
	public int guiScale;
	public float appropriateLineWidth;
	public boolean isMinimized;
}
