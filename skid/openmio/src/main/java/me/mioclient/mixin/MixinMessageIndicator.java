package me.mioclient.mixin;

import java.awt.Color;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.MixinMessageIndicatorHelper_2;
import net.minecraft.client.gui.hud.MessageIndicator;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({MessageIndicator.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinMessageIndicator.class */
public class MixinMessageIndicator {
    @Inject(method = {"indicatorColor"}, at = {@At("HEAD")}, cancellable = true)
    private void indicatorColorHook(CallbackInfoReturnable<Integer> callbackInfoReturnable) {
        if (((Object) this) == MixinMessageIndicatorHelper.messageIndicator) {
            Color color811 = MixinMessageIndicatorHelper_2.getColor811();
            callbackInfoReturnable.setReturnValue(Integer.valueOf(MixinMessageIndicatorHelper_2.getColor814(color811, color811.darker(), 3000.0d, 0.0d).hashCode()));
            callbackInfoReturnable.cancel();
        }
    }
}
