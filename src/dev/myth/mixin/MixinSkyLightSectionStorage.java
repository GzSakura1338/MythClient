package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.world.level.lighting.SkyLightSectionStorage;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/**
 * Mio MixinChunkSkyLightProvider returns 15 from the sky light lookup. 26.2 moved that read into
 * the section storage. Like Mio, this also reaches an integrated server's own light engine.
 */
@Mixin(SkyLightSectionStorage.class)
public abstract class MixinSkyLightSectionStorage {
    @Inject(method = "getLightValue(J)I", at = @At("HEAD"), cancellable = true)
    private void myth$skyLight(long blockNode, CallbackInfoReturnable<Integer> cir) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.skyLight.get()) cir.setReturnValue(15);
    }
}
