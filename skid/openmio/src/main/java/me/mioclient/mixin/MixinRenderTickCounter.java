package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import net.minecraft.client.render.RenderTickCounter;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({RenderTickCounter.Dynamic.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinRenderTickCounter.class */
public class MixinRenderTickCounter {

    @Shadow
    private float field_51958;

    @Inject(method = {"beginRenderTick(J)I"}, at = {@At(value = "FIELD", target = "Lnet/minecraft/client/render/RenderTickCounter$Dynamic;prevTimeMillis:J")})
    public void beginRenderTick(long j, CallbackInfoReturnable<Integer> callbackInfoReturnable) {
        if (BaritoneHelper_3.holeSnapSearchHelper4_2 == null) {
            return;
        }
        this.field_51958 *= BaritoneHelper_3.holeSnapSearchHelper4_2.get2019();
    }
}
