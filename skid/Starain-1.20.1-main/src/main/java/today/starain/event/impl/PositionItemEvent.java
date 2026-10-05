package today.starain.event.impl;

import lombok.Getter;
import lombok.Setter;
import net.minecraft.network.protocol.Packet;
import today.starain.event.api.Cancellable;
import today.starain.event.api.Event;

@Getter
@Setter
public class PositionItemEvent implements Event, Cancellable {
    private boolean cancelled;
    private Packet<?> packet;

    public PositionItemEvent(Packet<?> packet) {
        this.packet = packet;
    }
}
