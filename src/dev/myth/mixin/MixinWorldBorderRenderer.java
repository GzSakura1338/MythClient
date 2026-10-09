package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.renderer.WorldBorderRenderer;
import net.minecraft.client.renderer.state.level.WorldBorderRenderState;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(WorldBorderRenderer.class)
public abstract class MixinWorldBorderRenderer {
    @Inject(method = "render", at = @At("HEAD"), cancellable = true)
    private void myth$worldBorder(WorldBorderRenderState state, Vec3 cameraPos, double renderDistance,
                                  double depthFar, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.worldBorder.get()) ci.cancel();
    }
}
