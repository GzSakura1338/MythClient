package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.Ambience;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.biome.Biome;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({Biome.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBiome.class */
public class MixinBiome {
    private static Ambience ambience = (Ambience) BaritoneHelper_3.baritoneHelper_4.getModule117(Ambience.class);

    @Inject(method = {"getPrecipitation"}, at = {@At("HEAD")}, cancellable = true)
    private void getPrecipitationHook(BlockPos blockPos, CallbackInfoReturnable<Biome.Precipitation> callbackInfoReturnable) {
        if (ambience.isToggled() && ambience.worldWeather.getValue().booleanValue()) {
            callbackInfoReturnable.cancel();
            callbackInfoReturnable.setReturnValue(ambience.weather.getValue().getPrecipitation2196());
        }
    }
}
