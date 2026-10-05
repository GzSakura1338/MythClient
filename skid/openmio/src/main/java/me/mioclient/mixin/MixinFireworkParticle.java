package me.mioclient.mixin;

import it.unimi.dsi.fastutil.ints.IntList;
import net.minecraft.client.particle.FireworksSparkParticle;
import net.minecraft.client.particle.ParticleManager;
import net.minecraft.particle.ParticleTypes;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({FireworksSparkParticle.FireworkParticle.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinFireworkParticle.class */
public class MixinFireworkParticle {

    @Shadow
    @Final
    private ParticleManager field_3805;

    @Inject(method = {"addExplosionParticle"}, at = {@At("HEAD")}, cancellable = true)
    private void addExplosionParticleHook(double d, double d2, double d3, double d4, double d5, double d6, IntList intList, IntList intList2, boolean z, boolean z2, CallbackInfo callbackInfo) {
        if (this.field_3805.addParticle(ParticleTypes.FIREWORK, d, d2, d3, d4, d5, d6) == null) {
            callbackInfo.cancel();
        }
    }
}
