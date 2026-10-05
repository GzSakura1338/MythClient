package me.mioclient.mixin.ducks;

import net.minecraft.client.sound.AbstractSoundInstance;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({AbstractSoundInstance.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckAbstractSoundInstance.class */
public interface DuckAbstractSoundInstance {
    @Accessor("volume")
    void setVolume(float f);
}
