package today.starain.event.impl;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import net.minecraft.world.entity.Entity;
import today.starain.event.api.Event;

@Getter
@Setter
@AllArgsConstructor
public class RotationAnimationEvent implements Event {
    public static Entity currentEntity;
    private float yaw;
    private float lastYaw;
    private float pitch;
    private float lastPitch;
}
