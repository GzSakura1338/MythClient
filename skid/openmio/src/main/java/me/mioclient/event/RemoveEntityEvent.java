package me.mioclient.event;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/RemoveEntityEvent.class */
public class RemoveEntityEvent extends Event {
    public final int id;

    public RemoveEntityEvent(int i) {
        this.id = i;
    }

    public int getId() {
        return this.id;
    }
}
