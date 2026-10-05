package today.starain.event.impl;

import lombok.Getter;
import lombok.Setter;
import today.starain.event.api.Cancellable;
import today.starain.event.api.Event;
import today.starain.event.api.EventType;

@Getter
@Setter
public class MotionEvent implements Event, Cancellable {
    private boolean cancelled;
    private EventType type;
    private double x;
    private double y;
    private double z;
    private float yaw;
    private float pitch;
    private boolean onGround;

    public MotionEvent(EventType type, float yaw, float pitch) {
        this.type = type;
        this.yaw = yaw;
        this.pitch = pitch;
    }

    public MotionEvent(EventType type, double x, double y, double z, float yaw, float pitch, boolean onGround) {
        this.type = type;
        this.x = x;
        this.y = y;
        this.z = z;
        this.yaw = yaw;
        this.pitch = pitch;
        this.onGround = onGround;
    }
}
