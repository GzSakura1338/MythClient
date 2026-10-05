package me.mioclient.event;

import net.minecraft.client.sound.SoundInstance;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/PlayEvent.class */
public final class PlayEvent extends Event {
    public final SoundInstance soundInstance;

    public PlayEvent(SoundInstance soundInstance) {
        this.soundInstance = soundInstance;
    }

    public SoundInstance getSoundInstance1914() {
        return this.soundInstance;
    }
}
