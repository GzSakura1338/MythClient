package me.mioclient.mixin;

import me.mioclient.ShaderSearchHelper4;
import net.minecraft.client.render.block.entity.BlockEntityRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({BlockEntityRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBlockEntityRenderer.class */
public interface MixinBlockEntityRenderer {
    @Inject(method = {"getRenderDistance"}, at = {@At("HEAD")}, cancellable = true)
    private void getRenderDistanceHook(CallbackInfoReturnable<Integer> callbackInfoReturnable) {
        if (ShaderSearchHelper4.flag) {
            callbackInfoReturnable.setReturnValue(1024);
        }
    }
}
