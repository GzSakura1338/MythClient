package me.mioclient.event;

import me.mioclient.KeyPearlMode;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/EndDrawingEvent.class */
public class EndDrawingEvent extends Event {
    public final KeyPearlMode keyPearlMode;

    public EndDrawingEvent(KeyPearlMode keyPearlMode) {
        this.keyPearlMode = keyPearlMode;
    }

    public KeyPearlMode getKeyPearlMode1472() {
        return this.keyPearlMode;
    }
}
