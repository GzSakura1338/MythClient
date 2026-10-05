package me.mioclient.mixin;

import net.minecraft.util.crash.CrashReport;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({CrashReport.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinCrashReport.class */
public class MixinCrashReport {
    @Inject(method = {"addDetails"}, at = {@At("TAIL")})
    public void addStackTrace(StringBuilder sb, CallbackInfo callbackInfo) {
        sb.append("\n\n-- www.mioclient.me --\n");
        sb.append("Details:\n");
        sb.append("\tMagic: rizz\n\n");
    }
}
