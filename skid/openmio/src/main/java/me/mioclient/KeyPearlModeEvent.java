package me.mioclient;

import me.mioclient.event.Event;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/KeyPearlModeEvent.class */
public class KeyPearlModeEvent extends Event {
    public final KeyPearlMode keyPearlMode;

    public KeyPearlModeEvent(KeyPearlMode keyPearlMode) {
        this.keyPearlMode = keyPearlMode;
    }

    public KeyPearlMode getKeyPearlMode1472() {
        return this.keyPearlMode;
    }
}
