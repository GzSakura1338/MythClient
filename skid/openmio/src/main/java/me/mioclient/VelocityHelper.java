package me.mioclient;

import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.ExplosionVelocityEvent;
import me.mioclient.event.InteractBlockEvent;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.feature.MotionEvent;
import me.mioclient.module.movement.Velocity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/VelocityHelper.class */
public abstract class VelocityHelper implements SearchHelper_4 {
    public final Velocity velocity;

    public VelocityHelper(Velocity velocity) {
        this.velocity = velocity;
    }

    public abstract void do29(ChannelRead0Event channelRead0Event);

    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
    }

    public void do711() {
    }

    public void do598(ExplosionVelocityEvent explosionVelocityEvent) {
    }

    public void do31(MotionEvent motionEvent) {
    }

    public void onInteractBlock(InteractBlockEvent interactBlockEvent) {
    }

    public void onDisable() {
    }
}
