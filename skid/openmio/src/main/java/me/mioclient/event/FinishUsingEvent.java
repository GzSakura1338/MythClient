package me.mioclient.event;

import net.minecraft.item.ItemStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/FinishUsingEvent.class */
public final class FinishUsingEvent extends me.mioclient.TickMovementEvent {
    public ItemStack itemStack;

    public FinishUsingEvent(ItemStack itemStack) {
        this.itemStack = itemStack;
    }

    public ItemStack getItemStack2549() {
        return this.itemStack;
    }
}
