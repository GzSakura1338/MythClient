package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.font.Glyph;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({Glyph.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinGlyph.class */
public interface MixinGlyph {
    @Inject(method = {"getShadowOffset"}, at = {@At("HEAD")}, cancellable = true)
    default void getShadowOffsetHook(CallbackInfoReturnable<Float> callbackInfoReturnable) {
        NoRender noRender = (NoRender) BaritoneHelper_3.keyPearlSearchHelper4.getEnumSettingHelper120(NoRender.class);
        if (noRender.isToggled() && noRender.textShadow.getValue().booleanValue()) {
            callbackInfoReturnable.setReturnValue(Float.valueOf(0.6f));
        } else {
            callbackInfoReturnable.setReturnValue(Float.valueOf(1.0f));
        }
    }
}
