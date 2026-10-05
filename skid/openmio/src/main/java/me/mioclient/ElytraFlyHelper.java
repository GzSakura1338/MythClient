package me.mioclient;

import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.MoveEvent;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.event.TickEvent_2;
import me.mioclient.event.TickPostEvent;
import me.mioclient.feature.Event_3;
import me.mioclient.feature.MotionEvent;
import me.mioclient.module.movement.ElytraFly;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ElytraFlyHelper.class */
public abstract class ElytraFlyHelper implements SearchHelper_4 {
    public final ElytraFly elytraFly;

    public ElytraFlyHelper(ElytraFly elytraFly) {
        this.elytraFly = elytraFly;
    }

    public abstract void do27(TickEvent tickEvent);

    public abstract void do28(MoveEvent moveEvent);

    public abstract void do29(ChannelRead0Event channelRead0Event);

    public abstract void do30(SendImmediatelyEvent sendImmediatelyEvent);

    public abstract void do31(MotionEvent motionEvent);

    public abstract void do32(TickPostEvent tickPostEvent);

    public abstract void do33(Event_3 event_3);

    public void do329(TickEvent_2 tickEvent_2) {
    }

    public void onEnable() {
    }

    public void onDisable() {
    }
}
