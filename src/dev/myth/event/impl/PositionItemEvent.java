package dev.myth.event.impl;

import dev.myth.event.api.CancellableEvent;
import net.minecraft.network.protocol.Packet;

public class PositionItemEvent extends CancellableEvent {
    private Packet<?> packet;

    public PositionItemEvent(Packet<?> packet) {
        this.packet = packet;
    }

    public Packet<?> getPacket() {
        return packet;
    }

    public void setPacket(Packet<?> packet) {
        this.packet = packet;
    }
}
