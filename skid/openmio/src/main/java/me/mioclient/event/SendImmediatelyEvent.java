package me.mioclient.event;

import me.mioclient.PacketEvent;
import net.minecraft.network.packet.Packet;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/SendImmediatelyEvent.class */
public final class SendImmediatelyEvent extends PacketEvent {
    public SendImmediatelyEvent(Packet<?> packet) {
        super(packet);
    }
}
