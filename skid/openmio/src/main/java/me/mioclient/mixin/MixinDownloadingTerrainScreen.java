package me.mioclient.mixin;

import net.minecraft.client.gui.screen.DownloadingTerrainScreen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({DownloadingTerrainScreen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinDownloadingTerrainScreen.class */
public class MixinDownloadingTerrainScreen {
    @Inject(method = {"shouldCloseOnEsc"}, at = {@At("HEAD")}, cancellable = true)
    private void shouldCloseOnEscHook(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        callbackInfoReturnable.setReturnValue(true);
    }
}
