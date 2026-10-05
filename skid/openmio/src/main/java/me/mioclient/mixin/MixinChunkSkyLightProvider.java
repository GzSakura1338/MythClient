package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.NoRender;
import net.minecraft.world.chunk.light.ChunkSkyLightProvider;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({ChunkSkyLightProvider.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinChunkSkyLightProvider.class */
public class MixinChunkSkyLightProvider {
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);

    @Inject(at = {@At("HEAD")}, method = {"method_51585"}, cancellable = true)
    private void recalculateLevelhook(int i, int i2, int i3, CallbackInfoReturnable<Integer> callbackInfoReturnable) {
        if (norender.isToggled() && norender.skyLight.getValue().booleanValue()) {
            callbackInfoReturnable.setReturnValue(15);
            callbackInfoReturnable.cancel();
        }
    }
}
