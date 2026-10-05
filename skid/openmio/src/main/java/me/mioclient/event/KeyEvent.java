package me.mioclient.event;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/KeyEvent.class */
public final class KeyEvent extends Event {
    public final int num;
    public final boolean flag;

    public KeyEvent(int i, boolean z) {
        this.num = i;
        this.flag = z;
    }

    public int get2587() {
        return this.num;
    }

    public boolean is2588() {
        return this.flag;
    }
}
