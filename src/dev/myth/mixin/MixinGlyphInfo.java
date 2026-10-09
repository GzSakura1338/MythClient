package dev.myth.mixin;

import com.mojang.blaze3d.font.GlyphInfo;
import dev.myth.model.render.NoRender;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/**
 * Mio's TextShadow does not drop the shadow, it pulls it in: MixinGlyph returns 0.6 instead of
 * the vanilla 1.0. Providers that override the default keep their own offset, as in Mio.
 */
@Mixin(GlyphInfo.class)
public interface MixinGlyphInfo {
    @Inject(method = "getShadowOffset", at = @At("HEAD"), cancellable = true)
    default void myth$textShadow(CallbackInfoReturnable<Float> cir) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.textShadow.get()) cir.setReturnValue(.6f);
    }
}
