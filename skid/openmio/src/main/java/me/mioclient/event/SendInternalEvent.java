package me.mioclient.event;

import me.mioclient.PacketEvent;
import net.minecraft.network.packet.Packet;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/SendInternalEvent.class */
public final class SendInternalEvent extends PacketEvent {
    public SendInternalEvent(Packet<?> packet) {
        super(packet);
    }
}
