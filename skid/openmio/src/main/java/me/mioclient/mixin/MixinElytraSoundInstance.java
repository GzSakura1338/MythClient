package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.movement.ElytraFly;
import net.minecraft.client.sound.ElytraSoundInstance;
import net.minecraft.client.sound.MovingSoundInstance;
import net.minecraft.sound.SoundCategory;
import net.minecraft.sound.SoundEvent;
import net.minecraft.util.math.random.Random;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({ElytraSoundInstance.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinElytraSoundInstance.class */
public abstract class MixinElytraSoundInstance extends MovingSoundInstance {
    private static ElytraFly elytrafly = (ElytraFly) BaritoneHelper_3.baritoneHelper_4.getModule117(ElytraFly.class);

    protected MixinElytraSoundInstance(SoundEvent soundEvent, SoundCategory soundCategory, Random random) {
        super(soundEvent, soundCategory, random);
    }

    @Inject(method = {"tick"}, at = {@At("HEAD")}, cancellable = true)
    private void tickHook(CallbackInfo callbackInfo) {
        if (elytrafly.isToggled() && elytrafly.muteElytra.getValue().booleanValue()) {
            this.volume = 0.0f;
            callbackInfo.cancel();
        }
    }
}
