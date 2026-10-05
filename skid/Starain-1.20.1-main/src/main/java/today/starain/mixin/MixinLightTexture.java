package today.starain.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import net.minecraft.client.renderer.LightTexture;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import today.starain.Starain;
import today.starain.module.impl.render.FullBright;

@Mixin(LightTexture.class)
public abstract class MixinLightTexture {

    // ordinal 1 是 options.gamma().get()，ordinal 0 是 options.darknessEffectScale().get()。
    @ModifyExpressionValue(
            method = "updateLightTexture",
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/OptionInstance;get()Ljava/lang/Object;",
                    ordinal = 1
            )
    )
    private Object starain$hookFullBrightGamma(Object original) {
        if (Starain.getInstance() == null || Starain.getInstance().getModuleManager() == null) {
            return original;
        }

        FullBright fullBright = Starain.getInstance().getModuleManager().getModule(FullBright.class);
        if (fullBright == null || !fullBright.isGammaMode()) {
            return original;
        }

        return fullBright.getGamma();
    }
}
