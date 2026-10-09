package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.Camera;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.multiplayer.ClientLevel;
import net.minecraft.client.renderer.fog.FogData;
import net.minecraft.client.renderer.fog.FogRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/**
 * Mio pushes the fog end out by Range, terrain only unless Sky is on. 26.2 keeps those
 * distances in FogData instead of a shader uniform, so they get scaled on the way out.
 */
@Mixin(FogRenderer.class)
public abstract class MixinFogRenderer {
    @Inject(method = "setupFog", at = @At("RETURN"))
    private void myth$fog(Camera camera, int renderDistanceInChunks, DeltaTracker deltaTracker,
                          float darkenWorldAmount, ClientLevel level, CallbackInfoReturnable<FogData> cir) {
        NoRender noRender = NoRender.active();
        if (noRender == null || !noRender.fog.get()) return;
        FogData fog = cir.getReturnValue();
        float range = noRender.fogRange.get();
        fog.environmentalEnd *= range;
        fog.renderDistanceEnd *= range;
        if (noRender.fogSky.get()) { fog.skyEnd *= range; fog.cloudEnd *= range; }
    }
}
