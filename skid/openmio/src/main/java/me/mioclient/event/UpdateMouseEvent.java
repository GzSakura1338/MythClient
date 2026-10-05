package me.mioclient.event;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/UpdateMouseEvent.class */
public class UpdateMouseEvent extends Event {
    public final double val;
    public final double val2;

    public UpdateMouseEvent(double d, double d2) {
        this.val = d;
        this.val2 = d2;
    }

    public double get1008() {
        return this.val;
    }

    public double get1009() {
        return this.val2;
    }
}
