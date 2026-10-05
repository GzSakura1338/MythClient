package me.mioclient.mixin;

import me.mioclient.ParticlesHelper;
import net.minecraft.client.particle.Particle;
import net.minecraft.client.world.ClientWorld;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyVariable;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({Particle.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinParticle.class */
public class MixinParticle implements ParticlesHelper {

    @Shadow
    protected float field_3841;

    @Unique
    private float mio$initialAlpha;

    @Inject(method = {"<init>(Lnet/minecraft/client/world/ClientWorld;DDD)V"}, at = {@At("TAIL")})
    private void init(ClientWorld clientWorld, double d, double d2, double d3, CallbackInfo callbackInfo) {
        this.mio$initialAlpha = this.field_3841;
    }

    @ModifyVariable(method = {"setAlpha"}, at = @At("HEAD"), ordinal = 0, argsOnly = true)
    private float setAlpha(float f) {
        return f * this.mio$initialAlpha;
    }

    @Override // me.mioclient.ParticlesHelper
    public void mio$setInitialAlpha(float f) {
        this.mio$initialAlpha = f;
        this.field_3841 = f;
    }

    @Override // me.mioclient.ParticlesHelper
    public float mio$getInitialAlpha() {
        return this.mio$initialAlpha;
    }
}
