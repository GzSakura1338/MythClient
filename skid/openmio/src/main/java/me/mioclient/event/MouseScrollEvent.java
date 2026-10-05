package me.mioclient.event;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/MouseScrollEvent.class */
public class MouseScrollEvent extends Event {
    public final double val;
    public final double val2;

    public MouseScrollEvent(double d, double d2) {
        this.val = d;
        this.val2 = d2;
    }

    public double get2801() {
        return this.val;
    }

    public double get2802() {
        return this.val2;
    }
}
