package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.Ambience;
import net.minecraft.util.math.random.Random;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({World.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinWorld.class */
public class MixinWorld {
    private static Ambience ambience = (Ambience) BaritoneHelper_3.baritoneHelper_4.getModule117(Ambience.class);

    @Inject(method = {"getRainGradient"}, at = {@At("HEAD")}, cancellable = true)
    private void getRainGradientHook(CallbackInfoReturnable<Float> callbackInfoReturnable) {
        if (ambience.worldWeather.getValue().booleanValue() && ambience.isToggled()) {
            callbackInfoReturnable.setReturnValue(Float.valueOf(ambience.weather.getValue() == Ambience.AmbiencePredicateMode.CLEAR ? 0.0f : 1.0f));
            callbackInfoReturnable.cancel();
        }
    }

    @ModifyExpressionValue(method = {"<init>"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/util/math/random/Random;create()Lnet/minecraft/util/math/random/Random;")}, require = 0)
    private Random initHook(Random random) {
        return Random.createThreadSafe();
    }
}
