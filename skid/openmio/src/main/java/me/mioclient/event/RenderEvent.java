package me.mioclient.event;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/RenderEvent.class */
public class RenderEvent extends Event {
    public final boolean flag;

    public RenderEvent(boolean z) {
        this.flag = z;
    }

    public boolean is168() {
        return this.flag;
    }
}
