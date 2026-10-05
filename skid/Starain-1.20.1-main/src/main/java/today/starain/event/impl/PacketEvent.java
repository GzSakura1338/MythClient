package today.starain.event.impl;

import lombok.Getter;
import lombok.Setter;
import net.minecraft.network.protocol.Packet;
import today.starain.event.api.Cancellable;
import today.starain.event.api.Event;
import today.starain.event.api.EventType;

@Getter
@Setter
public class PacketEvent implements Event, Cancellable {

    private boolean cancelled;

    private EventType type;
    private Packet<?> packet;

    public PacketEvent(EventType type, Packet<?> packet) {
        this.type = type;
        this.packet = packet;
    }
}
