package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.misc.NoNarrator;
import net.minecraft.client.option.NarratorMode;
import net.minecraft.client.util.NarratorManager;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({NarratorManager.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinNarratorManager.class */
public class MixinNarratorManager {
    private static NoNarrator nonarrator = (NoNarrator) BaritoneHelper_3.baritoneHelper_4.getModule117(NoNarrator.class);

    @Inject(method = {"getNarratorMode"}, at = {@At("HEAD")}, cancellable = true)
    private void getNarrator(CallbackInfoReturnable<NarratorMode> callbackInfoReturnable) {
        if (nonarrator.isToggled()) {
            callbackInfoReturnable.setReturnValue(NarratorMode.OFF);
        }
    }

    @Inject(method = {"onModeChange"}, at = {@At("HEAD")}, cancellable = true)
    private void onModeChange(CallbackInfo callbackInfo) {
        if (nonarrator.isToggled()) {
            callbackInfo.cancel();
        }
    }
}
