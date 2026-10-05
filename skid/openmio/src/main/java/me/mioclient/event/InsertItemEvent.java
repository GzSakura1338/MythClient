package me.mioclient.event;

import net.minecraft.screen.slot.Slot;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/InsertItemEvent.class */
public class InsertItemEvent extends Event {
    public final Slot slot;

    public InsertItemEvent(Slot slot) {
        this.slot = slot;
    }

    public Slot getSlot2783() {
        return this.slot;
    }
}
