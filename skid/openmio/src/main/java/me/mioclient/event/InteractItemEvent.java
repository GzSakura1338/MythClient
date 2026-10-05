package me.mioclient.event;

import net.minecraft.util.Hand;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/InteractItemEvent.class */
public class InteractItemEvent extends Event {
    public final Hand hand;

    public InteractItemEvent(Hand hand) {
        this.hand = hand;
    }

    public Hand getHand2084() {
        return this.hand;
    }
}
