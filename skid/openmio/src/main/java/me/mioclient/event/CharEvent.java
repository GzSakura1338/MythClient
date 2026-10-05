package me.mioclient.event;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/CharEvent.class */
public final class CharEvent extends Event {
    public final int num;

    public CharEvent(int i) {
        this.num = i;
    }

    public String getString1445() {
        return Character.toString(this.num);
    }

    public char get1446() {
        return getString1445().charAt(0);
    }
}
