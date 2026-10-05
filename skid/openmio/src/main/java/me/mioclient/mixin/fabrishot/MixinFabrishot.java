package me.mioclient.mixin.fabrishot;

import java.io.File;
import java.io.FileInputStream;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.misc.ExtraScreenshot;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Pseudo;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Pseudo
@Mixin(targets = {"me.ramidzkh.fabrishot.Fabrishot"}, remap = false)
/* loaded from: mio-yarn.jar:me/mioclient/mixin/fabrishot/MixinFabrishot.class */
public class MixinFabrishot {
    private static ExtraScreenshot extrascreenshot = (ExtraScreenshot) BaritoneHelper_3.baritoneHelper_4.getModule117(ExtraScreenshot.class);

    @Inject(method = {"printFileLink"}, at = {@At("HEAD")}, cancellable = true, remap = false)
    private static void printFileLinkHook(File file, CallbackInfo callbackInfo) {
        if (extrascreenshot.isToggled()) {
            try {
                FileInputStream fileInputStream = new FileInputStream(file);
                byte[] readAllBytes = fileInputStream.readAllBytes();
                fileInputStream.close();
                extrascreenshot.do2187(readAllBytes);
            } catch (Exception e) {
            }
            if (extrascreenshot.is2190()) {
                try {
                    file.delete();
                } catch (Exception e2) {
                }
            }
            if (extrascreenshot.is2191()) {
                return;
            }
            callbackInfo.cancel();
        }
    }
}
