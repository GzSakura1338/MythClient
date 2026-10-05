package me.mioclient.feature;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Event_3.class */
public final class Event_3 extends me.mioclient.event.Event {
    public float yaw;
    public float pitch;
    public boolean flag;

    public Event_3(float f, float f2, boolean z) {
        this.yaw = f;
        this.pitch = f2;
        this.flag = z;
    }

    public float get751() {
        return this.yaw;
    }

    public void setYaw(float f) {
        this.yaw = f;
    }

    public float get752() {
        return this.pitch;
    }

    public void setPitch(float f) {
        this.pitch = f;
    }

    public boolean is2925() {
        return this.flag;
    }
}
