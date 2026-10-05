package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.NoRender;
import me.mioclient.module.render.SkyColor;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.hud.BossBarHud;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({BossBarHud.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBossBarHud.class */
public class MixinBossBarHud {
    private static final NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static final SkyColor skycolor = (SkyColor) BaritoneHelper_3.baritoneHelper_4.getModule117(SkyColor.class);

    @Inject(method = {"render"}, at = {@At("HEAD")}, cancellable = true)
    private void renderHook(DrawContext drawContext, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.bossBars.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"shouldDarkenSky"}, at = {@At("HEAD")}, cancellable = true)
    private void shouldDarkenSkyHook(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if ((norender.isToggled() && norender.bossBars.getValue().booleanValue()) || (skycolor.isToggled() && skycolor.is3136())) {
            callbackInfoReturnable.setReturnValue(false);
            callbackInfoReturnable.cancel();
        }
    }
}
