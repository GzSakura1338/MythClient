package me.mioclient;

import me.mioclient.event.Event;
import net.minecraft.network.packet.Packet;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PacketEvent.class */
public abstract class PacketEvent extends Event {
    public final Packet<?> packet;

    public PacketEvent(Packet<?> packet) {
        this.packet = packet;
    }

    public Packet<?> getPacket904() {
        return this.packet;
    }
}
