package me.mioclient.mixin;

import java.awt.Color;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.Ambience;
import net.minecraft.client.render.LightmapTextureManager;
import net.minecraft.client.texture.NativeImage;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({LightmapTextureManager.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinLightmapTextureManager.class */
public class MixinLightmapTextureManager {
    private static Ambience ambience = (Ambience) BaritoneHelper_3.baritoneHelper_4.getModule117(Ambience.class);

    @Shadow
    @Final
    private NativeImage field_4133;

    @Redirect(method = {"update"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/texture/NativeImage;setColor(III)V"))
    private void updateHook(NativeImage nativeImage, int i, int i2, int i3) {
        if (!ambience.isToggled() || ambience.brightness.getValue() != Ambience.MixinEntityRendererMode.SCREEN || (i == 15 && i2 == 15)) {
            nativeImage.setColor(i, i2, i3);
        } else {
            Color value = ambience.color.getValue();
            this.field_4133.setColor(i, i2, (-16777216) | (value.getBlue() << 16) | (value.getGreen() << 8) | value.getRed());
        }
    }

    @Inject(method = {"pack"}, at = {@At("HEAD")}, cancellable = true)
    private static void packHook(int i, int i2, CallbackInfoReturnable<Integer> callbackInfoReturnable) {
        if (ambience.isToggled() && ambience.brightness.getValue() == Ambience.MixinEntityRendererMode.SCREEN) {
            callbackInfoReturnable.setReturnValue(0);
            callbackInfoReturnable.cancel();
        }
    }
}
