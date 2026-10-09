package dev.myth.mixin.access;

import com.mojang.blaze3d.pipeline.RenderTarget;
import net.minecraft.client.renderer.LevelRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/** The silhouette the Shader ESP reads; vanilla keeps it private and only the glow effect uses it. */
@Mixin(LevelRenderer.class)
public interface LevelRendererAccess {
    @Accessor("entityOutlineTarget")
    RenderTarget myth$entityOutlineTarget();
}
