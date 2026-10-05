package me.mioclient.event;

import me.mioclient.KeyPearlMode;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/MoveEvent_2.class */
public class MoveEvent_2 {
    public final KeyPearlMode keyPearlMode;
    public float val;

    public MoveEvent_2(KeyPearlMode keyPearlMode, float f) {
        this.keyPearlMode = keyPearlMode;
        this.val = f;
    }

    public KeyPearlMode getKeyPearlMode1472() {
        return this.keyPearlMode;
    }

    public float get990() {
        return this.val;
    }

    public void do1473(float f) {
        this.val = f;
    }
}
