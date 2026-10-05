package today.starain.mixin;

import com.mojang.blaze3d.platform.Window;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.utils.render.shader.impl.KawaseBlur;
import today.starain.utils.render.skia.context.SkiaContext;

@Mixin(Window.class)
public class MixinWindow {
    @Shadow
    private int width;

    @Shadow
    private int height;

    @Inject(method = "onFramebufferResize", at = @At("HEAD"))
    private void onFramebufferResize(long window, int width, int height, CallbackInfo ci) {
        SkiaContext.createSurface(width, height);
        KawaseBlur.INSTANCE.resize();
    }
}