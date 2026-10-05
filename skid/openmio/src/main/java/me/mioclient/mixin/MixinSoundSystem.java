package me.mioclient.mixin;

import me.mioclient.SearchHelper_4;
import me.mioclient.event.PlayEvent;
import net.minecraft.client.sound.SoundInstance;
import net.minecraft.client.sound.SoundSystem;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({SoundSystem.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinSoundSystem.class */
public class MixinSoundSystem {
    @Inject(method = {"play(Lnet/minecraft/client/sound/SoundInstance;)V"}, at = {@At("HEAD")}, cancellable = true)
    private void playHook(SoundInstance soundInstance, CallbackInfo callbackInfo) {
        PlayEvent playEvent = new PlayEvent(soundInstance);
        SearchHelper_4.baritoneHelper.getObject1794(playEvent);
        if (playEvent.is2403()) {
            callbackInfo.cancel();
        }
    }
}
