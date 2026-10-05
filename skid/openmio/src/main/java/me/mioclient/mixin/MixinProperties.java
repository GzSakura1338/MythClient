package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.SearchHelper_4;
import me.mioclient.module.render.SkyColor;
import net.minecraft.client.world.ClientWorld;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({ClientWorld.Properties.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinProperties.class */
public class MixinProperties {
    private static SkyColor skycolor = (SkyColor) BaritoneHelper_3.baritoneHelper_4.getModule117(SkyColor.class);

    @Inject(method = {"getHorizonShadingRatio"}, at = {@At("HEAD")}, cancellable = true)
    private void onGetSkyProperties(CallbackInfoReturnable<Float> callbackInfoReturnable) {
        if (SearchHelper_4.minecraftClient.world != null && skycolor.isToggled() && skycolor.is3136() && !SearchHelper_4.minecraftClient.world.getRegistryKey().getValue().getPath().contains("over")) {
            callbackInfoReturnable.setReturnValue(Float.valueOf(1.0f));
            callbackInfoReturnable.cancel();
        }
    }
}
