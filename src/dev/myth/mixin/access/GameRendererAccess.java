package dev.myth.mixin.access;

import net.minecraft.client.renderer.GameRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/**
 * renderLevel folds the portal / nausea skew into the projection from these two private fields,
 * so anything projecting world positions itself has to read them to land on the same pixels.
 */
@Mixin(GameRenderer.class)
public interface GameRendererAccess {
    @Accessor("spinningEffectTime")
    float myth$spinningEffectTime();

    @Accessor("spinningEffectSpeed")
    float myth$spinningEffectSpeed();
}
