package dev.myth.event.impl;

import dev.myth.event.api.CancellableEvent;
import dev.myth.event.api.EventType;
import net.minecraft.network.protocol.Packet;

public class PacketEvent extends CancellableEvent {
    private EventType type;
    private Packet<?> packet;

    public PacketEvent(EventType type, Packet<?> packet) {
        this.type = type;
        this.packet = packet;
    }

    public EventType getType() {
        return type;
    }

    public void setType(EventType type) {
        this.type = type;
    }

    public Packet<?> getPacket() {
        return packet;
    }

    public void setPacket(Packet<?> packet) {
        this.packet = packet;
    }
}
