package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import dev.myth.Myth;
import dev.myth.model.render.FullBright;
import net.minecraft.client.renderer.LightmapRenderStateExtractor;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Slice;

/** Replace only the lightmap's gamma read, leaving the saved vanilla option intact. */
@Mixin(LightmapRenderStateExtractor.class)
public abstract class MixinLightmapRenderStateExtractor {
    @ModifyExpressionValue(method = "extract", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/OptionInstance;get()Ljava/lang/Object;"),
            slice = @Slice(from = @At(value = "INVOKE", target = "Lnet/minecraft/client/Options;gamma()Lnet/minecraft/client/OptionInstance;"),
                    to = @At(value = "INVOKE", target = "Lnet/minecraft/client/Options;darknessEffectScale()Lnet/minecraft/client/OptionInstance;")))
    private Object myth$fullBrightGamma(Object original) {
        FullBright fullBright = Myth.MODEL_MANAGER.getModel(FullBright.class).orElse(null);
        return fullBright != null && fullBright.isGammaMode() ? fullBright.getGamma() : original;
    }
}
